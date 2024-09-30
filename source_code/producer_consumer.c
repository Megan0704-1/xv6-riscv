#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Megan Kuo");

// global parameters
static int prod=0;
module_param(prod, int, 0);
MODULE_PARM_DESC(prod, "Number of producer threads");

static int cons=0;
module_param(cons, int, 0);
MODULE_PARM_DESC(cons, "Number of consumer threads");

static int size=0;
module_param(size, int, 0);
MODULE_PARM_DESC(size, "Size of semaphores");

struct task_struct** producer_threads=NULL;
struct task_struct** consumer_threads=NULL;

struct semaphore empty, full;

// check if threads are created successfully
// if kthread_run fails, it returns an error pointer (ERR_PTR)
bool assert_threadrun_error(struct task_struct* id, const char *type, int idx) {
    if(IS_ERR(id)) {
        printk(KERN_ERR "Failed to create %s thread number %d\n", type, idx);
        return true;
    }
    return false;
}

int kthread_producer(void* arg) {
    const char* thread_name = current->comm;

    while(!kthread_should_stop()) {
        down_interruptible(&empty);
        printk("An item has been produced by %s\n", thread_name);
        up(&full);
        msleep(100);
    }

    return 0;
}

int kthread_consumer(void* arg) {
    const char* thread_name = current->comm;

    while(!kthread_should_stop()) {
        down_interruptible(&full);
        printk("An item has been consumed by %s\n", thread_name);
        up(&empty);
        msleep(100);
    }

    return 0;
}

static int __init producer_consumer_init(void) {
    struct task_struct *producer, *consumer;


    // initialize semaphore
    // empty = size
    // full = 0
    sema_init(&empty, size);
    sema_init(&full, 0);
    
    producer_threads = kmalloc_array(prod, sizeof(struct task_struct *), GFP_KERNEL);
    if(!producer_threads) {
        printk(KERN_ERR "Failed to allocate array for producer threads.\n");
        return -ENOMEM;
    }
    consumer_threads = kmalloc_array(cons, sizeof(struct task_struct *), GFP_KERNEL);
    if(!consumer_threads) {
        printk(KERN_ERR "Failed to allocate array for consumer threads.\n");
        kfree(producer_threads);
        return -ENOMEM;
    }
    for(int i=0; i<prod; ++i) {
        producer = kthread_run(kthread_producer, NULL, "Producer-%d", i);
        if(assert_threadrun_error(producer, "producer", i)) {
            for(int j=0; j<i; ++j) kthread_stop(producer_threads[j]);
            kfree(producer_threads);
            kfree(consumer_threads);
            return PTR_ERR(producer);
        }
        producer_threads[i] = producer;
    }
    for(int i=0; i<cons; ++i) {
        consumer = kthread_run(kthread_consumer, NULL, "Consumer-%d", i);
        if(assert_threadrun_error(consumer, "consumer", i)) {
            for(int j=0; j<i; ++j) kthread_stop(consumer_threads[j]);
            kfree(consumer_threads);
            kfree(producer_threads);
            return PTR_ERR(consumer);
        } 
        consumer_threads[i] = consumer;
    }

    printk(KERN_INFO "Producer consumer module initialized\n");
    return 0;
}

static void __exit producer_consumer_exit(void) {
    // free threads
    for(int i=0; i<prod; ++i) {
        if(producer_threads[i]) {
            int ret = kthread_stop(producer_threads[i]);
            printk(KERN_INFO "Stopped producer %d with return value: %d\n", i, ret);
        }
    }
    for(int i=0; i<cons; ++i) {
        if(consumer_threads[i]) {
            int ret = kthread_stop(consumer_threads[i]);
            printk(KERN_INFO "Stopped consumer %d with return value: %d\n", i, ret);
        }
    }
    printk(KERN_INFO "Exiting producer consumer module.\n");
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);

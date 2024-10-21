#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#define BUFFERSIZE 10

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Megan");
MODULE_DESCRIPTION("Simple producer consumer module");
MODULE_VERSION("1.0");

static int buffer[BUFFERSIZE];
static int in=0, out=0, count=0;

static DEFINE_MUTEX(buffer_mutex);
static DECLARE_WAIT_QUEUE_HEAD(producer_wait);
static DECLARE_WAIT_QUEUE_HEAD(consumer_wait);

static struct task_struct *producer_thread;
static struct task_struct *consumer_thread;

static int producer_func(void *data) {
    int item=0;

    while(!kthread_should_stop()) {
        msleep(500);
        item++;

        mutex_lock(&buffer_mutex);

        while(count == BUFFERSIZE) {
            mutex_unlock(&buffer_mutex);
            if(wait_event_interruptible(producer_wait, count<BUFFERSIZE)) return -ERESTARTSYS;
            mutex_lock(&buffer_mutex);
        }

        buffer[in] = item;
        in = (in+1) % BUFFERSIZE;
        count++;
        printk(KERN_INFO "Producer: Produced item %d, buffer count: %d\n", item, count);

        wake_up_interruptible(&consumer_wait);

        mutex_unlock(&buffer_mutex);
    }

    printk(KERN_INFO "Producer thread stopping...\n");
    return 0;
}

static int consumer_func(void* data) {
    int item=0;

    while(!kthread_should_stop) {
        mutex_lock(&buffer_mutex);

        while(count==0) {
            mutex_unlock(&buffer_mutex);

            if(wait_event_interruptible(consumer_wait, count>0)) return -ERESTARTSYS;
            
            mutex_lock(&buffer_mutex);
        }

        item = buffer[out];
        out = (out+1) % BUFFERSIZE;
        count--;
        printk(KERN_INFO "Consumer: consumed item %d, buffer count: %d\n", item, count);

        wake_up_interruptible(&producer_wait);

        mutex_unlock(&buffer_mutex);

        msleep(1000);
    }

    printk(KERN_INFO "Consumer thread stopping\n");
    return 0;
}

staic int __init thread_init(void) {
    printk(KERN_INFO "Producer Consumer Moudle loading...\n");

    in = out = count = 0;

    producer_thread = kthread_run(producer_func, NULL, "Producer_thread");
    consumer_thread = kthread_run(consumer_func, NULL, "Consumer thread");
    return 0;
}

static void __exit thread_exit(void) {
    printk(KERN_INFO "Producer Consumer Module Unloading...\n");

    if(producer_thread) {
        kthread_stop(producer_thread);
    }
    if(consumer_thread) {
        kthread_stop(consumer_thread);
    }

}

module_init(thread_init);
module_exit(thread_exit);

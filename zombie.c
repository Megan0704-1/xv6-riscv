#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#define MAX_BUFFER_SIZE 65536
#define DEFAULT_BUFFER_SIZE 1024
#define DEFAULT_PROD_SIZE 1
#define DEFAULT_CONS_SIZE 2
#define DEFAULT_UID 1000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Megan Kuo");
MODULE_DESCRIPTION("Zombie Killer Module");

static int prod=DEFAULT_PROD_SIZE;
module_param(prod, int, 0);
MODULE_PARM_DESC(prod, "Number of producer threads.");

static int cons=DEFAULT_CONS_SIZE;
module_param(cons, int, 0);
MODULE_PARM_DESC(cons, "Number of consumer threads.");

static int size=DEFAULT_BUFFER_SIZE;
module_param(size, int, 0);
MODULE_PARM_DESC(size, "Size of the shared buffer.");

static int uid=DEFAULT_UID;
module_param(uid, int, 0);
MODULE_PARM_DESC(uid, "The UID of the zombie spreader.");

typedef struct {
    struct task_struct **buffer;
    int capacity;
    int zombie_cnt;
    int prod_idx;
    int cons_idx;

    struct mutex lock;
    wait_queue_head_t producer_queue;
    wait_queue_head_t consumer_queue;
} bounded_buffer;

bounded_buffer bb;
static struct task_struct **producers;
static struct task_struct **consumers;

void buffer_init(int s) {
    bb.capacity = s;
    bb.zombie_cnt=0;
    bb.prod_idx=0;
    bb.cons_idx=0;
    bb.buffer = kmalloc(sizeof(struct task_struct*) * s, GFP_KERNEL);

    mutex_init(&bb.lock);
    init_waitqueue_head(&bb.producer_queue);
    init_waitqueue_head(&bb.consumer_queue);
}

static int zombie_generator(void* data) {
    struct task_struct* p;

    while(!kthread_should_stop()) {
        rcu_read_lock(); // pre-thread lock
        for_each_process(p) {
            if((p->real_cred->uid.val != uid) || !(p->exit_state & EXIT_ZOMBIE)) continue;

            mutex_lock(&bb.lock);

            if(bb.zombie_cnt == bb.capacity) {
                mutex_unlock(&bb.lock); // release the lock
                wait_event_interruptible(bb.producer_queue, bb.zombie_cnt < bb.capacity || kthread_should_stop());

                if(kthread_should_stop()) {
                    rcu_read_unlock();
                    return 0;
                }

                mutex_lock(&bb.lock); // reacquire the lock after waking up
            }

            get_task_struct(p); // increment p reference count
            bb.buffer[bb.prod_idx] = p;
            bb.prod_idx = (bb.prod_idx+1 % bb.capacity);
            bb.zombie_cnt++;

            mutex_unlock(&bb.lock);
            wake_up_interruptible(&bb.consumer_queue);
        }
        rcu_read_unlock();
        msleep(250);
    }
    return 0;
}


static int __init zombie_module(void) {
    buffer_init(size);

    producers = kmalloc(sizeof(struct task_struct*) * prod, GFP_KERNEL);
    consumers = kmalloc(sizeof(struct task_struct*) * cons, GFP_KERNEL);

    for(int i=0; i<prod; ++i) {
        producers[i] = kthread_run(zombie_generator, NULL, "Producer-%d", i);
    }
    // for(int i=0; i<cons; ++i) {
        // consumers[i] = kthread_run(zombie_killer, NULL, "Consumer-%d", i);
    // }
// 
    pr_info("Zombie Module loaded.\n");
    return 0;
}

static void __exit zombie_exit(void) {
    for(int i=0; i<prod; ++i) {
        kthread_stop(producers[i]);
    }
    for(int i=0; i<cons; ++i) {
        kthread_stop(consumers[i]);
    }
    if(bb.buffer) {
        kfree(bb.buffer);
    }

    pr_info("Leaving zombie module.\n");
}

module_init(zombie_module);
module_exit(zombie_exit);

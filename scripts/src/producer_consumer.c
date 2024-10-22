#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/sched.h>

#define MAX_BUFFER_SIZE 65536
#define DEFAULT_BUFFER_SIZE 1024
#define DEFAULT_PROD_SIZE 1
#define DEFAULT_CONS_SIZE 2
#define DEFAULT_UID 1000
#define THREAD_STOP_CHECK if(kthread_should_stop()) {rcu_read_unlock(); return 0;}

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

// function prototype
void buffer_init(int);
void kill_zombie(struct task_struct*);

// custom bounded buffer struct
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

// global variable
bounded_buffer bb;
static struct task_struct **producers;
static struct task_struct **consumers;

// helper function for init bounded buffer struct
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

void kill_zombie(struct task_struct* zombie) {
    struct task_struct* parent;

    rcu_read_lock();
    parent = rcu_dereference(zombie->real_parent);
    get_task_struct(parent);
    rcu_read_unlock();

    // SIGCHLD notifies the parent that its child has exited, the parent should call wait to clean up the zombie
    send_sig_info(SIGCHLD, SEND_SIG_NOINFO, parent);
    put_task_struct(parent);
}

static int zombie_generator(void* data) {
    struct task_struct* p;

    while(!kthread_should_stop()) {
        rcu_read_lock(); // pre-thread lock
        for_each_process(p) {
            if((p->real_cred->uid.val != uid) || !(p->exit_state & EXIT_ZOMBIE)) continue;

            mutex_lock(&bb.lock);

            while(bb.zombie_cnt == bb.capacity) {
                mutex_unlock(&bb.lock); // release the lock
                THREAD_STOP_CHECK;

                wait_event_interruptible(bb.producer_queue, bb.zombie_cnt < bb.capacity || kthread_should_stop());
                THREAD_STOP_CHECK;

                mutex_lock(&bb.lock); // reacquire the lock after waking up

                continue;
            }

            get_task_struct(p); // increment task reference count
            bb.buffer[bb.prod_idx] = p;
            bb.prod_idx = ((bb.prod_idx+1) % bb.capacity);
            bb.zombie_cnt++;

            mutex_unlock(&bb.lock);
            wake_up_interruptible(&bb.consumer_queue);
        }
        rcu_read_unlock();
        msleep(250);
    }
    return 0;
}

static int zombie_killer(void* data) {
    struct task_struct *zombie;

    while(!kthread_should_stop()) {
        mutex_lock(&bb.lock);

        while(bb.zombie_cnt == 0) {
            mutex_unlock(&bb.lock);
            THREAD_STOP_CHECK;

            wait_event_interruptible(bb.consumer_queue, bb.zombie_cnt>0 || kthread_should_stop());
            THREAD_STOP_CHECK;

            mutex_lock(&bb.lock);

            continue;
        }

        zombie = bb.buffer[bb.cons_idx];
        bb.cons_idx = ((bb.cons_idx+1) % bb.capacity);
        bb.zombie_cnt--;

        mutex_unlock(&bb.lock);
        wake_up_interruptible(&bb.producer_queue);

        kill_zombie(zombie);
        put_task_struct(zombie); // decrement task reference count
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
    for(int i=0; i<cons; ++i) {
        consumers[i] = kthread_run(zombie_killer, NULL, "Consumer-%d", i);
    }

    pr_info("Zombie Module loaded.\n");
    return 0;
}

static void __exit zombie_exit(void) {
    for(int i=0; i<prod; ++i) {
        if(producers[i]) kthread_stop(producers[i]);
    }
    for(int i=0; i<cons; ++i) {
        if(consumers[i]) kthread_stop(consumers[i]);
    }

    mutex_lock(&bb.lock);
    while(bb.zombie_cnt>0) {
        struct task_struct *zombie = bb.buffer[bb.cons_idx];
        bb.cons_idx = (bb.cons_idx+1) % bb.capacity;
        bb.zombie_cnt--;
        put_task_struct(zombie);
    }
    mutex_unlock(&bb.lock);

    if(bb.buffer) kfree(bb.buffer);
    if(producers) kfree(producers);
    if(consumers) kfree(consumers);

    pr_info("Leaving zombie module.\n");
}

module_init(zombie_module);
module_exit(zombie_exit);

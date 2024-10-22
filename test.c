#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>
#include <linux/kthread.h>

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


// bounded buffer struct
typedef struct {
    // dynamic allocated shared buffer
    struct task_struct **buffer;

    // number of items in the buffer
    int zombie_count;
    int prod_idx;
    int cons_idx;

    // lock to protect buffer.
    struct mutex mutex;

    DECLARE_WAIT_QUEUE_HEAD(producer_wait);
    DECLARE_WAIT_QUEUE_HEAD(consumer_wait);
} bb;

// global bounded buffer
static bb shared_buffer;

static int zombie_producer_thread(void* data) {
    struct task_struct *p;

    const char* producer_name = current->comm;
   for_each_process(p) {
        // check if the process belongs to param uid
        if(p->cred->uid.val != uid) continue;

        if(p->exit_state & EXIT_ZOMBIE) {
            // found one zombie process
            // two conditions
            // 1. buffer size full -> put producer to wait queue
            // 2. buffer size empty -> increment zombie count

            if(mutex_lock_interruptible(&shared_buffer.mutex)) {
                // acquire lock failed
                pr_info("Process interrupted while waiting for mutex.\n");
                return -ERESTARTSYS;
            }

            while(shared_buffer.zombie_count == size) {
                pr_info("Buffer full. Put process to wait");

                mutex_unlock(&shared_buffer.mutex);

                // sleep until condition is true
                if(wait_event_interruptible(shared_buffer.producer_wait, shared_buffer.zombie_count < size)) {
                    pr_info("Process interrupted while waiting for buffer space.\n");
                    return -ERESTARTSYS;
                }

                // reacquire mutex when process is awake
                if(mutex_lock_interruptible(&shared_buffer.mutex)) {
                    pr_info("Process interrrupted while re-acquiring mutex lock.\n");
                    return -ERESTARTSYS;
                }
            }

            struct task_struct *task = current;
            int pid = task->pid;
            int ppid = task->real_parent->pid;
            pr_info("[%s] has produced a zombie process with pid %d and parent pid %d", producer_name, pid, ppid);

            shared_buffer.buffer[shared_buffer.prod_idx] = task;
            shared_buffer.prod_idx = (shared_buffer.prod_idx+1) % size;
            shared_buffer.zombie_count++;

            wake_up(&shared_buffer.consumer_wait);
            mutex_unlock(&shared_buffer.mutex);
        }
    }

}

static int __init count_process_init(void) {

    DEFINE_MUTEX(mutex);

    pr_info("Initializing module with buffer size: %u\n", size);

    // buffer size check
    if(size <=0 || size > MAX_BUFFER_SIZE) {
        pr_err("Invalid buffer size: %u. Must be between 1 and %u for kmalloc allocation function", size, MAX_BUFFER_SIZE);
        return -EINVAL; // invalid
    }

    shared_buffer.buffer = kmalloc(size * sizeof(struct task_struct*), GFP_KERNEL);
    if(!shared_buffer.buffer) {
        pr_err("Failed to allocate buffer of size %u.\n", size);
        return -ENOMEM;
    }

    memset(shared_buffer.buffer, 0, size);

    mutex_init(shared_buffer.mutex);

    shared_buffer.zombie_count=0; shared_buffer.prod_idx=0; shared_buffer.cons_idx=0;

    struct task_struct *producer_thread = kthread_run(zombie_producer_thread, NULL, "Producer-1");
    
    zombie_producer_thread();
    return 0;
}

static void __exit count_process_exit(void) {

    if(shared_buffer.buffer) {
        kfree(shared_buffer.buffer);
        pr_info("Buffer freed.\n");
    }

    pr_info("Module exiting\n");
}


module_init(count_process_init);
module_exit(count_process_exit);

// [New] sysipc.c

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysipc.h"
#include "sysipc_alloc.h"

extern struct proc proc[NPROC];
extern struct spinlock ipc_lock;

// send(pid, msg)
// send msg to a pid
// copies msg from user addr to 
// 1. reciever buffer if avail
// 2. sender buffer if 1. not success
uint64
sys_send(void) {
  int dest_pid;
  uint64 user_addr;
  struct proc* p = myproc();

  // expecting 2 arguments
  argint(0, &dest_pid);
  argaddr(1, &user_addr);

  // lookup destination pid
  struct proc* dest_p = 0;
  acquire(&ipc_lock);
  for(struct proc* q = proc; q < &proc[NPROC]; ++q) {
    if((q->pid == dest_pid) && (q->state != UNUSED) && (q->state != ZOMBIE)) {
      dest_p = q;
      break;
    }
  }

  if((dest_p == 0) || (dest_p == p)) {
    release(&ipc_lock);
    return -1;
  }

  // check if queue is full
  if(dest_p->ipc_queue_count >= IPC_QUEUE_LIMIT) {
    release(&ipc_lock);
    return -1;
  }

  // copy sender process IPC head to kernel space
  int kernel_head_buffer[3];
  if(copyin(p->pagetable, (char*)kernel_head_buffer, user_addr, sizeof(kernel_head_buffer)) < 0) {
    release(&ipc_lock);
    return -1;
  }

  int msgid = kernel_head_buffer[0];
  int msgtype = kernel_head_buffer[1];
  int msglen = kernel_head_buffer[2];

  // enforce msg len to be within max payload size
  if((msglen < 0) || (msglen > IPC_MAX_PAYLOAD)) {
    release(&ipc_lock);
    return -1;
  }

  // dynamic allocate for ipc msg node
  // TODO: change kalloc to more fine grained memory control
  struct ipc_msg_node *new_msg = alloc_ipc_msg_node();
  if(!new_msg) {
    release(&ipc_lock);
    return -1;
  }

  // define the new node
  new_msg->msgid = msgid;
  new_msg->msgtype = msgtype;
  new_msg->msglen = msglen;
  new_msg->msg_sender_pid = p->pid;
  new_msg->payload = 0;
  new_msg->next = 0;

  // write the msg
  if(msglen > 0) {
    new_msg->payload = (char*)kalloc();

    // ask memory for payload fail
    if(!new_msg->payload) {
      free_ipc_msg_node(new_msg);
      release(&ipc_lock);
      return -1;
    }

    // copy payload from user space
    uint64 payload_addr = user_addr + sizeof(kernel_head_buffer);
    if(copyin(p->pagetable, new_msg->payload, payload_addr, msglen) < 0) {
      kfree(new_msg->payload);
      free_ipc_msg_node(new_msg);
      release(&ipc_lock);
      return -1;
    }
  }

  // append p to dest_p's queue
  if(dest_p->ipc_queue_tail) {
    dest_p->ipc_queue_tail->next = new_msg;
    dest_p->ipc_queue_tail = new_msg;
  } else {
    // no node in queue
    dest_p->ipc_queue_head = new_msg;
    dest_p->ipc_queue_tail = new_msg;
  }
  dest_p->ipc_queue_count ++;

  // check dest_p ipc status
  if(dest_p->ipc_flags & IPC_WAITING) {
    if((dest_p->expected_src == IPC_ANY_SENDER) || (dest_p->expected_src == p->pid)) {
      dest_p->ipc_flags &= ~IPC_WAITING;
      wakeup(dest_p);
    }
  }

  // once the sender's msg is enqueued, its task complete

  release(&ipc_lock);
  return 0;
}

// recv(pid, msg)
// A user process calls sys_recv when it wants to read msg from the message queue
uint64 sys_recv(void) {
  int from, flags;
  uint64 user_addr;
  struct proc* p = myproc();

  // expecting 3 args
  // (expected sender pid, where to store the msg, mode)
  // mode 0 for blocking, 1 for non blocking
  argint(0, &from);
  argaddr(1, &user_addr);
  argint(2, &flags);

  acquire(&ipc_lock);
  struct ipc_msg_node *sender_node = 0;
  struct ipc_msg_node *prev_node = 0;

  // polling
  while(1) {
    // Scan the queue for a msg_sender_pid == from or IPC_ANY_SENDER
    struct ipc_msg_node *cur_node = p->ipc_queue_head;
    while(cur_node) {
      if((from == IPC_ANY_SENDER) || (cur_node->msg_sender_pid == from)) {
        sender_node = cur_node;
        break;
      }
      prev_node = cur_node;
      cur_node = cur_node->next;
    }
    
    // found sender
    if(sender_node) break;

    // no matching msg
    if(flags & IPC_NONBLOCK) {
      release(&ipc_lock);
      return -1;
    }

    // block current process until msg arrives
    p->ipc_flags = IPC_WAITING;
    p->expected_src = from;
    sleep(p, &ipc_lock);
  }

  // sender msg exist in recver msg queue
  if(prev_node) {
    prev_node->next = sender_node->next;
  } else {
    p->ipc_queue_head = sender_node->next;
  }
  if(sender_node == p->ipc_queue_tail) {
    p->ipc_queue_tail = prev_node;
  }

  p->ipc_queue_count--;
  release(&ipc_lock);

  // prepare to copy out (from kernel to user)
  int kernel_head_buffer[3];
  kernel_head_buffer[0] = sender_node->msgid;
  kernel_head_buffer[1] = sender_node->msgtype;
  kernel_head_buffer[2] = sender_node->msglen;

  // copy out header
  if(copyout(p->pagetable, user_addr, (char*)kernel_head_buffer, sizeof(kernel_head_buffer)) < 0) {
    if(sender_node->payload) {
      kfree(sender_node->payload);
    }
    free_ipc_msg_node(sender_node);
    return -1;
  }

  // copy out payload
  if(sender_node->msglen > 0) {
    if(copyout(p->pagetable, user_addr + sizeof(kernel_head_buffer), (char*)sender_node->payload, sender_node->msglen) < 0) {
      if(sender_node->payload) {
        kfree(sender_node->payload);
      }
      free_ipc_msg_node(sender_node);
      return -1;
    }
    kfree(sender_node->payload);
  }

  free_ipc_msg_node(sender_node);
  return 0;
}

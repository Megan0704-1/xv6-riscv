// [New] ipc.c

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysipc_alloc.h"
#include "server_protocol.h"

extern struct proc proc[NPROC];
extern struct spinlock ipc_lock;

// sys_send helper
uint64
ipc_send(int dest_pid, uint64 user_addr) {
  printf("ipc send: %d request to send to %d\n", myproc()->pid, dest_pid);
  struct proc* p = myproc();

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
    printf("ipc_send: sending to invalid dest (self / 0)\n");
    release(&ipc_lock);
    return -1;
  }

  // check if queue is full
  if(dest_p->ipc_queue_count >= IPC_QUEUE_LIMIT) {
    release(&ipc_lock);
    return -1;
  }

  // copy sender payload to kernl
  IPCHeader kernel_header;
  if(copyin(p->pagetable, (char*)&kernel_header, user_addr, sizeof(kernel_header)) < 0) {
    release(&ipc_lock);
    return -1;
  }

  int msgid = kernel_header.msgid;
  int msgtype = kernel_header.msgtype;
  int msglen = kernel_header.msglen;

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
    uint64 payload_addr = user_addr + sizeof(kernel_header);
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

// sys_recv helper
uint64 ipc_recv(int from, uint64 user_addr, int flags) {
  printf("ipc recv: %d wants to read from %d\n", myproc()->pid, from);
  printf("requested user address to put msg in: %lx\n", user_addr);
  struct proc* p = myproc();

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
    printf("sleeps %d\n", p->pid);
    sleep(p, &ipc_lock);
  }

  printf("wakes %d\n", p->pid);
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
  IPCHeader kernel_header;
  kernel_header.msgid = sender_node->msgid;
  kernel_header.msgtype = sender_node->msgtype;
  kernel_header.msglen = sender_node->msglen;

  // copy out header
  if(copyout(p->pagetable, user_addr, (char*)&kernel_header, sizeof(kernel_header)) < 0) {
    if(sender_node->payload) {
      kfree(sender_node->payload);
    }
    free_ipc_msg_node(sender_node);
    return -1;
  }

  // copy out payload
  if(sender_node->msglen > 0) {
    if(copyout(p->pagetable, user_addr + sizeof(kernel_header), (char*)sender_node->payload, sender_node->msglen) < 0) {
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

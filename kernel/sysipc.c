// [New] sysipc.c

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysipc.h"

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

  argint(0, &dest_pid);
  argaddr(1, &user_addr);

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

  // send success if dest_p is sleeping and waiting for
  // msg from (or not from) the sender
  if((dest_p->state == SLEEPING) && 
     (dest_p->ipc_status == RECV_BLOCKING) &&
      ((dest_p->expected_src == -1) ||
       (dest_p->expected_src == p->pid))) {
    
    // copy messge to dest ipc buffer from user space with 32 bytes
    if(copyin(p->pagetable, dest_p->ipc_msg.data, user_addr, MSGSIZE) < 0) {
      release(&ipc_lock);
      return -1;
    }

    dest_p->ipc_status =NOIPC;
    dest_p->expected_src = -1;

    // wake reciever
    if(dest_p->state == SLEEPING) {
      wakeup(dest_p);
    }

    release(&ipc_lock);
    return 0;
  }

  // in other case, the reciever is not waiting
  // block the sender
  p->ipc_status = SEND_BLOCKING;
  p->pending_dest = dest_pid;

  // copy message to sender buffer from user space
  if(copyin(p->pagetable, p->ipc_msg.data, user_addr, MSGSIZE) < 0) {
    p->ipc_status = NOIPC;
    release(&ipc_lock);
    return -1;
  }

  // put sender to sleep
  sleep(p, &ipc_lock);

  // if waken by reciever, we knoe msg is delievered
  p->ipc_status = NOIPC;
  release(&ipc_lock);
  return 0;
}

// recv(pid, msg)
// recieve msg from any sender (-1) or specific pid
// copies from sender to reciever
// then copies to user addr
uint64 sys_recv(void) {
  int from;
  uint64 user_addr;
  struct proc* p = myproc();

  argint(0, &from);
  argaddr(1, &user_addr);

  acquire(&ipc_lock);

  struct proc* sender = 0;
  for(struct proc* q = proc; q < &proc[NPROC]; ++q) {
    // search for any sender [from==-1]
    // or specific sender [q->pid]
    if((q->state == SLEEPING) && 
       (q->ipc_status == SEND_BLOCKING) && 
       (q->pending_dest == p->pid)) {
      if((from == -1) || (from == q->pid)) {
        sender = q;
        break;
      }
    }
  }

  if(sender) {
    // kernel mem copy
    memmove(p->ipc_msg.data, sender->ipc_msg.data, MSGSIZE);

    // unblock sender
    sender->ipc_status = NOIPC;
    if(sender->state == SLEEPING) {
      wakeup(sender);
    }
  } else {
    // no sender is waiting
    // block reciever
    p->ipc_status = RECV_BLOCKING;
    p->expected_src = from < 0 ? -1 : from;

    // put receiver to sleep
    sleep(p, &ipc_lock);

    // woken by sender
    p->ipc_status = NOIPC;
  }

  if(copyout(p->pagetable, user_addr, p->ipc_msg.data, MSGSIZE) < 0) {
    release(&ipc_lock);
    return -1;
  }

  release(&ipc_lock);
  return 0;
}

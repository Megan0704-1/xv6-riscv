// [New] sysipc.c

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

#include "ipc.h"
#include "sysipc.h"
#include "sysipc_alloc.h"

// send(pid, msg)
// send msg to a pid
// copies msg from user addr to 
// 1. reciever buffer if avail
// 2. sender buffer if 1. not success
uint64
sys_send(void) {
  int dest_pid;
  uint64 user_addr;

  // expecting 2 arguments
  argint(0, &dest_pid);
  argaddr(1, &user_addr);

  int err = ipc_send(dest_pid, user_addr);
  return err;
}

// recv(pid, msg)
// A user process calls sys_recv when it wants to read msg from the message queue
uint64 sys_recv(void) {
  int from, flags;
  uint64 user_addr;

  // expecting 3 args
  // (expected sender pid, where to store the msg, mode)
  // mode 0 for blocking, 1 for non blocking
  argint(0, &from);
  argaddr(1, &user_addr);
  argint(2, &flags);

  int err = ipc_recv(from, user_addr, flags);
  return err;
}

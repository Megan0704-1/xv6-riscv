#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysipc_alloc.h"

// global free list for ipc_msg_node
struct ipc_msg_node *ipc_msg_free_list = 0;

struct spinlock ipc_alloc_lock;

void sysipc_alloc_init(void) {
  initlock(&ipc_alloc_lock, "sysipc_alloc");
}

void refill_freelist(void) {
  char *page = (char*) kalloc();
  if(!page) {
    release(&ipc_alloc_lock);
    panic("refill_freelist for ipc_msg_node: kalloc failed\n");
    return;
  }

  // partition kalloc page into ipc_msg_node object
  for(int i=0; i<IPC_NODES_PER_PAGE; ++i) {
    struct ipc_msg_node *node = (struct ipc_msg_node*)(page + i*IPC_NODE_SIZE);

    node->next = ipc_msg_free_list;
    ipc_msg_free_list = node;
  }
}

struct ipc_msg_node *alloc_ipc_msg_node(void) {
  acquire(&ipc_alloc_lock);
  if(!ipc_msg_free_list) {
    refill_freelist();
  }
  if(!ipc_msg_free_list) {
    release(&ipc_alloc_lock);
    return 0;
  }

  struct ipc_msg_node *node = ipc_msg_free_list;
  ipc_msg_free_list = ipc_msg_free_list->next;

  release(&ipc_alloc_lock);
  return node;
}

void free_ipc_msg_node(struct ipc_msg_node *node) {
  if(!node) return;

  acquire(&ipc_alloc_lock);

  node->next = ipc_msg_free_list;
  ipc_msg_free_list = node;

  release(&ipc_alloc_lock);
}

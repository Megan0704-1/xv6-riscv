// sysipc_alloc.h
#ifndef SYSIPC_ALLOC_H
#define SYSIPC_ALLOC_H

#define IPC_NODE_SIZE (sizeof(struct ipc_msg_node))
#define IPC_NODES_PER_PAGE (PGSIZE / IPC_NODE_SIZE)

void refill_freelist(void);
struct ipc_msg_node *alloc_ipc_msg_node(void);
void free_ipc_msg_node(struct ipc_msg_node *node);

#endif // SYSIPC_ALLOC_H

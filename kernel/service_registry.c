// [New] service_registry.c

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "service_registry.h"

struct spinlock service_registry_lock;
struct service_entry service_registry[MAX_SERVICES];

void service_registry_init(void) {
  initlock(&service_registry_lock, "service_registry");

  for(int i=0; i<MAX_SERVICES; ++i) {
    service_registry[i].name[0] = '\0';
    service_registry[i].pid = -1;
  }
}

int register_service(const char *name, int pid) {
  acquire(&service_registry_lock);

  for(int i=0; i<MAX_SERVICES; ++i) {
    if((strncmp(service_registry[i].name, name, SERVICE_NAME_LEN) != 0) || (service_registry[i].pid == pid)) {
      release(&service_registry_lock);
      return -1;
    }

    if(service_registry[i].pid == -1) {
      strncpy(service_registry[i].name, name, SERVICE_NAME_LEN-1);
      service_registry[i].name[SERVICE_NAME_LEN-1] += '\0';
      service_registry[i].pid = pid;

      release(&service_registry_lock);
      return 0;
    }
  }

  release(&service_registry_lock);
  return -1;
}

int lookup_service(const char *name) {
  int pid = -1;
  acquire(&service_registry_lock);

  for(int i=0; i<MAX_SERVICES; ++i) {
    if(strncmp(service_registry[i].name, name, SERVICE_NAME_LEN) == 0) {
      pid = service_registry[i].pid;
      break;
    }
  }

  release(&service_registry_lock);
  return pid;
}

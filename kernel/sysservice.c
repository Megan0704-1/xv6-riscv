// [New] sysservice.c

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "service_registry.h"

// [New] register server process
uint64
sys_register_service(void)
{
  char name[SERVICE_NAME_LEN];
  int pid;

  argstr(0, name, SERVICE_NAME_LEN);
  argint(1, &pid);
  
  return register_service(name, pid);
}

// [New] lookup server syscall
uint64
sys_lookup_service(void)
{
  char name[SERVICE_NAME_LEN];
  argstr(0, name, SERVICE_NAME_LEN);
  return lookup_service(name); 
}


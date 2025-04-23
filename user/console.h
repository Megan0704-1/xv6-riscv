#ifndef UCONSOLE_H
#define UCONSOLE_H

#include "kernel/syscall.h"
#include "kernel/types.h"

static int 
console_read(int fd, void *buf, int n) {
  register int a0 asm("a0") = fd;
  register uint a1 asm("a1") = (uint64)buf;
  register int a2 asm("a2") = n;
  register int a7 asm("a7") = SYS_read;
  asm volatile("ecall"
      : "+r"(a0)
      : "r"(a1), "r"(a2), "r"(a7)
      : "memory");
  return a0;
}

static int 
console_write(int fd, const void *buf, int n) {
  register int a0 asm("a0") = fd;
  register uint a1 asm("a1") = (uint64)buf;
  register int a2 asm("a2") = n;
  register int a7 asm("a7") = SYS_read;
  asm volatile("ecall"
      : "+r"(a0)
      : "r"(a1), "r"(a2), "r"(a7)
      : "memory");
  return a0;
}

#endif 

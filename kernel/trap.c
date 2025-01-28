#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define ll long long

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

// helper func
static uint64 get_reg(struct trapframe *tf, int reg);
static void set_reg(struct trapframe *tf, int reg, uint64 val); 

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    // system call

    if(killed(p))
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
  }

  // Secure Microkernel Design Course - Your Solution Here
  else if(r_scause() == 2) {
    int bit_field_5 = 0x1f;

    uint64 instr = r_stval();
    const uint32 opcode = instr & 0x7f; // mask bit 6:0 for opcode
    const uint32 funct5 = (instr >> 27) & bit_field_5; // madk bit 31:27
    if(opcode == 0x0b && funct5 == 0x0) {
      // mac instruction
      int rd = (instr >> 7) & bit_field_5;
      int rs1 = (instr >> 12) & bit_field_5;
      int rs2 = (instr >> 17) & bit_field_5;
      int rs3 = (instr >> 22) & bit_field_5;

      uint64 a = get_reg(p->trapframe, rs1);
      uint64 b = get_reg(p->trapframe, rs2);
      uint64 c = get_reg(p->trapframe, rs3);

      ll result = (ll)a * (ll)b + (ll)c;

      // store
      set_reg(p->trapframe, rd, (int)result);

      // update pc
      p->trapframe->epc += 4;
    } else {
      printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
      printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
      setkilled(p);
    }
  }

  else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if(killed(p))
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    // interrupt or trap from an unknown source
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  if(cpuid() == 0){
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }

  // ask for the next timer interrupt. this also clears
  // the interrupt request. 1000000 is about a tenth
  // of a second.
  w_stimecmp(r_time() + 1000000);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if(scause == 0x8000000000000009L){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000005L){
    // timer interrupt.
    clockintr();
    return 2;
  } else {
    return 0;
  }
}

static uint64
get_reg(struct trapframe *tf, int reg) {
  // x0 is always 0
  if(reg == 0) return 0;

  switch(reg) {
    case 1: return tf->ra;
    case 2: return tf->sp;
    case 3: return tf->gp;
    case 4: return tf->tp;
    case 5: return tf->t0;
    case 6: return tf->t1;
    case 7: return tf->t2;
    case 8: return tf->s0;
    case 9: return tf->s1;
    case 10: return tf->a0;
    case 11: return tf->a1;
    case 12: return tf->a2;
    case 13: return tf->a3;
    case 14: return tf->a4;
    case 15: return tf->a5;
    case 16: return tf->a6;
    case 17: return tf->a7;
    case 18: return tf->s2;
    case 19: return tf->s3;
    case 20: return tf->s4;
    case 21: return tf->s5;
    case 22: return tf->s6;
    case 23: return tf->s7;
    case 24: return tf->s8;
    case 25: return tf->s9;
    case 26: return tf->s10;
    case 27: return tf->s11;
    case 28: return tf->t3;
    case 29: return tf->t4;
    case 30: return tf->t5;
    case 31: return tf->t6;

    default:
      panic("get_reg: invalid register");
      return 0;
  }
}

static void
set_reg(struct trapframe *tf, int reg, uint64 val) {
  // x0 is always 0
  if(reg == 0) return;

  switch(reg) {
    case 1:  tf->ra = val; break;
    case 2:  tf->sp = val; break;
    case 3:  tf->gp = val; break; 
    case 4:  tf->tp = val; break;
    case 5:  tf->t0 = val; break;
    case 6:  tf->t1 = val; break;
    case 7:  tf->t2 = val; break;
    case 8:  tf->s0 = val; break;
    case 9:  tf->s1 = val; break;
    case 10:  tf->a0 = val; break;
    case 11:  tf->a1 = val; break;
    case 12:  tf->a2 = val; break;
    case 13:  tf->a3 = val; break;
    case 14:  tf->a4 = val; break;
    case 15:  tf->a5 = val; break;
    case 16:  tf->a6 = val; break;
    case 17:  tf->a7 = val; break;
    case 18:  tf->s2 = val; break;
    case 19:  tf->s3 = val; break;
    case 20:  tf->s4 = val; break;
    case 21:  tf->s5 = val; break;
    case 22:  tf->s6 = val; break;
    case 23:  tf->s7 = val; break;
    case 24:  tf->s8 = val; break;
    case 25:  tf->s9 = val; break;
    case 26:  tf->s10 = val; break;
    case 27:  tf->s11 = val; break;
    case 28:  tf->t3 = val; break;
    case 29:  tf->t4 = val; break;
    case 30:  tf->t5 = val; break;
    case 31:  tf->t6 = val; break;

    default:
      panic("set_reg: invalid register");
  }
}

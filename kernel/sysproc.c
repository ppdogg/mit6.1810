#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  backtrace();
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void) {
  int interval;
  uint64 addr;

  argint(0, &interval);
  argaddr(1, &addr);

  myproc()->interval = interval;
  myproc()->alarmhandler = (void (*)()) addr;
  return 0;
}

uint64
sys_sigreturn(void) {
  struct proc *p = myproc();
  // restore register
  p->trapframe->ra = p->savedstate->ra;
  p->trapframe->sp = p->savedstate->sp;
  p->trapframe->t1 = p->savedstate->t1;
  p->trapframe->s0 = p->savedstate->s0;
  p->trapframe->s1 = p->savedstate->s1;
  p->trapframe->a1 = p->savedstate->a1;
  p->trapframe->a4 = p->savedstate->a1;
  p->trapframe->a4 = p->savedstate->a4;
  p->trapframe->a5 = p->savedstate->a5;
  p->trapframe->s2 = p->savedstate->s2;
  p->trapframe->s3 = p->savedstate->s3;
  p->trapframe->s4 = p->savedstate->s4;
  p->trapframe->s5 = p->savedstate->s5;

  p->trapframe->epc = p->savedstate->epc;

  p->interrupted = 0;

  return p->savedstate->a0; 
}

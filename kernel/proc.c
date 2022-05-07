#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define SUCCESS 1
#define FAIL 0
#define TRUE 1
#define FALSE 0

extern uint64 cas(volatile void* addr, int expected, int newval);

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;

//todo : pointer for the lists?
struct proc_list* unused_list;
struct proc_list* sleeping_list;
struct proc_list* zombie_list;

struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;






//-------------------------------------------------------proc_list-------------------------------------------------------------
//todo: check when the set_cpu failed
int
is_valid_cpu(int cpu_num)
{
  printf("in is_valid_cpu func\n");
  return cpu_num>=0 && cpu_num<NCPU;
}

//todo need cas for get and set?
//todo need to return -1?
int
set_cpu(int cpu_num)
{
    printf("in set_cpu func\n");

  if(!is_valid_cpu(cpu_num))
  {
      return -1;
  }
  struct proc* p = myproc();
  acquire(&p->node_lock); 
  p->cpu_num = cpu_num; 
  release(&p->node_lock);
  yield();
  return cpu_num;
}

struct proc_list*
get_ready_list(int cpu_num)
{
  printf("in get_ready_list func\n");

  struct cpu* c = get_cpu_by_cpu_num(cpu_num);
  if(c == 0)
  {
    return FAIL;
  }
  return c->ready_list;
}

struct cpu*
get_cpu_by_cpu_num(int cpu_num)
{
  printf("in get_cpu_by_cpu_num func\n");
  if(!is_valid_cpu(cpu_num))
  {
      return FAIL;
  }
  return &cpus[cpu_num];
}
//todo check set/get cpu 3.1.5
int
get_cpu(void)
{
          printf("in get_cpu func\n");

  struct proc* p = myproc();
  acquire(&p->node_lock); 
  int curr_cpu_num = p->cpu_num;
  release(&p->node_lock);
  return curr_cpu_num;
}


//todo check the changes in the assinment in part 1
//todo cas we need to change 0 to 1?

//todo need lock on list? the lock in the function or before and after functions?
//todo imp is_valid_proc_index
int
is_valid_proc_index(int index)
{
  printf("in is_valid_proc_index func\n");
  return (index >= 0 && index <NPROC);
}

//important: before calling is_empty() acquire proc_list->lock
//important: is_empty() not realease proc_list->lock
int
is_empty(struct proc_list* proc_list)
{
  printf("in is_empty func\n");
  return proc_list->head==-1 || (!is_valid_proc_index(proc_list->head));
}
//todo: what id the proc is unused?
struct proc* 
get_proc_by_index(int index)
{
  printf("in get_proc_by_index func\n");

  if(is_valid_proc_index(index))
  {
    struct proc* p  = &proc[index];
    return p;
  }
  return FAIL;
}

//important: before calling is_empty() dont hold: proc_list->lock and tail_proc->node_lock
int
add_proc_to_tail(int p_index, struct proc_list* proc_list)
{
  printf("in add_proc_to_tail func\n");

  if(!is_valid_proc_index(p_index))
  {
    return FAIL;
  }
  acquire(&proc_list->lock);
  if(is_empty(proc_list))
  { 
    proc_list->head=p_index;
    proc_list->tail=p_index;
    release(&proc_list->lock);
    return SUCCESS;
  }
  struct proc* tail_proc = get_proc_by_index(proc_list->tail);
  if(tail_proc == 0)
  {
    release(&proc_list->lock);
    return FAIL;
  }
  acquire(&tail_proc->node_lock);
  tail_proc->next_proc_index = p_index; 
  proc_list->tail=p_index;
  release(&tail_proc->node_lock);
  release(&proc_list->lock);
  return SUCCESS;
}

//important: before calling get_head() acquire proc_list->lock
//important: get_head() not realease proc_list->lock
struct proc*
get_head(struct proc_list* proc_list)
{
  printf("in get_head func\n");
  return get_proc_by_index(proc_list->head);
}

//important: before calling is_tail() acquire proc_list->lock
//important: after calling is_tail() release proc_list->lock
int
is_tail(struct proc* p, struct proc_list* proc_list)
{
  return p->index == proc_list->tail; 
}


//important: before calling is_remove_head() acquire proc_list->lock
//important: after calling is_remove_head() release proc_list->lock
int
is_remove_head(int p_index, struct proc_list* proc_list)
{
  printf("in is_remove_head func\n");

  return (p_index==proc_list->head) && (p_index!=-1);
}


//important: before calling has_next() acquire p->node_lock
//important: after calling has_next() release p->node_lock
int
has_next(struct proc *p)
{
  return p->next_proc_index != -1;
}

//important: before calling remove_head() acquire proc_list->lock
//important: before calling remove_head() dont hold: head_proc->node_lock
//important: after calling remove_head() release proc_list->lock

struct proc*
remove_head(struct proc_list* proc_list)
{
  printf("in remove_head func\n");

  struct proc *p = get_head(proc_list);
  if(p != 0)
  {
    acquire(&p->node_lock);
    proc_list->head = p->next_proc_index;
    if(!has_next(p)) 
    {
      proc_list->tail = -1;
    }
    p->next_proc_index = -1;
    release(&p->node_lock);
  }
  return p;
}

//important: before calling remove_head() dont hold: head_proc->node_lock and proc_list->lock and node_lock

int
remove_proc(int p_index, struct proc_list* proc_list)
{
                            printf("in remove_proc func\n");

  struct proc *pred, *curr;
  acquire(&proc_list->lock);
  if(is_empty(proc_list))
  {
    release(&proc_list->lock);
    return FAIL;
  }

  if(is_remove_head(p_index, proc_list))
  {
    int result = FAIL;
    if(remove_head(proc_list))
    {
      result = SUCCESS;
    }
    release(&proc_list->lock);
    return result;
  }

  //todo imp
  pred = get_proc_by_index(proc_list->head);
  if(pred == 0)
  {
    release(&proc_list->lock);
    return FAIL;
  }

  acquire(&pred->node_lock);
  release(&proc_list->lock);
  
  if(!has_next(pred)) 
  {
    release(&pred->node_lock);
    return FAIL;
  }
  //todo change to function
  while (has_next(pred)) 
  {
    curr = get_proc_by_index(pred->next_proc_index);
    if(curr == 0)
    {
      release(&pred->node_lock);
      return FAIL;
    }
    acquire(&curr->node_lock);
    if (p_index == curr->index)
    {
      acquire(&proc_list->lock);
      if(is_tail(curr, proc_list))
      {
        proc_list->tail = pred->index;
      }
      release(&proc_list->lock);
      pred->next_proc_index = curr->next_proc_index;
      curr->next_proc_index = -1;
      release(&curr->node_lock);
      release(&pred->node_lock);
      return SUCCESS;
    }
    release(&pred->node_lock);
    pred = curr;
  }
  release(&curr->node_lock);
  return FAIL;
} 

//-------------------------------------------------------proc_list------------------------------------------------------------





// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl) {
                              printf("in proc_mapstacks func\n");

  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

void
init_list(struct proc_list* list, char* name)
{
                                printf("in init_list func\n");

  initlock(&list->lock, name);
  list->head = -1;
  list->tail = -1;
}

void
init_process_lists(void)
{
                                  printf("in init_process_lists func\n");

  init_list(unused_list,"unused_list_lock");
  init_list(sleeping_list, "sleeping_list_lock");
  init_list(zombie_list, "zombie_list_lock");
}


// initialize the proc table at boot time.
void
procinit(void)
{
                                    printf("in procinit func\n");

  init_process_lists();
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  int i = 0;
  for(p = proc; p < &proc[NPROC]; i++, p++) {
    initlock(&p->lock, "proc");
    initlock(&p->node_lock, "node_lock");
    p->kstack = KSTACK((int) (p - proc));
    p->index = i;
    p->next_proc_index = -1;
    add_proc_to_tail(i, unused_list);
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
                                      printf("in cpuid func\n");

  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void) {
                                        printf("in mycpu func\n");

  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void) {
                                          printf("in myproc func\n");

  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid() {
                                            printf("in allocpid func\n");

  int pid;
  do
  {
    pid = nextpid;
  }while(cas(&nextpid, pid, pid+1));

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.

static struct proc*
allocproc(void)
{
                                              printf("in allocproc func\n");
  acquire(&unused_list->lock);
  struct proc *p = remove_head(unused_list);
  release(&unused_list->lock);
  if(p == 0)
  {
    return FAIL;
  }
  acquire(&p->lock);
  goto found;

found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  p->next_proc_index = -1;
  
  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.

//todo: imp -> git
static void
freeproc(struct proc *p)
{
                                                printf("in freeproc func\n");

  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  remove_proc(p->index, zombie_list);
  add_proc_to_tail(p->index, unused_list);
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
                                                  printf("in proc_pagetable func\n");

  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
                                                    printf("in proc_freepagetable func\n");

  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
//todo imp -> git
void
userinit(void)
{
                                                      printf("in userinit func\n");

  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&p->lock);
  int first_cpu_num = 0;
  add_proc_to_tail(p->index, get_ready_list(first_cpu_num));
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
                                                        printf("in growproc func\n");

  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
//todo imp -> git
int
fork(void)
{
                                                          printf("in fork func\n");

  int i, pid;
  struct proc *np;
  struct proc *p = myproc();
  acquire(&p->node_lock);
  int father_cpu_num = p->cpu_num;
  release(&p->node_lock);

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;

  release(&np->lock);
  acquire(&np->node_lock);
  np->cpu_num = father_cpu_num;
  np->next_proc_index = -1;
  release(&np->node_lock);

  add_proc_to_tail(np->index, get_ready_list(father_cpu_num));

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
                                                            printf("in reparent func\n");

  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
//todo imp -> git
void
exit(int status)
{
                                                              printf("in exit func\n");

  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  //todo need?
  /*
  acquire(&p->lock);
  p->cpu_num=0;
  release(&p->lock);
  */
  acquire(&p->node_lock);
  p->cpu_num = 0;
  release(&p->node_lock);

  add_proc_to_tail(p->index, zombie_list);

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
                                                              printf("in wait func\n");
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      if(np->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if(np->state == ZOMBIE){
          // Found one.
          pid = np->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
//todo imp -> git
void
scheduler(void)
{
                                                                printf("in scheduler func\n");

  struct proc *p;
  struct cpu *c = mycpu();
  init_list(c->ready_list, "ready_list");

  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
    struct proc_list* ready_list = c->ready_list;
    acquire(&ready_list->lock);
    p = remove_head(ready_list);
    release(&ready_list->lock);
    if(p != 0)
    {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
          // Switch to chosen process.  It is the process's job
          // to release its lock and then reacquire it
          // before jumping back to us.
          p->state = RUNNING;
          c->proc = p;
          swtch(&c->context, &p->context);

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;       
      }
      release(&p->lock);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
                                                                  printf("in sched func\n");

  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
//todo inp -> git
void
yield(void)
{
                                                                    printf("in yield func\n");

  struct proc *p = myproc();
  acquire(&p->lock);  
  p->state = RUNNABLE;
  acquire(&p->node_lock);
  int cpu_num = p->cpu_num;
  release(&p->node_lock);
  add_proc_to_tail(p->index,get_ready_list(cpu_num));
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
                                                                      printf("in forkret func\n");

  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
//todo: imp-> git
void
sleep(void *chan, struct spinlock *lk)
{
                                                                        printf("in sleep func\n");

  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;
  add_proc_to_tail(p->index,sleeping_list);

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
//todo:  imp -> git
//todo locks
//todo delete all printf
void
wakeup(void *chan)
{
  printf("in wakeup func\n");
  struct proc *pred, *curr;
  int found_proc_to_wakeup = FALSE;
  while(TRUE)
  {
    acquire(&sleeping_list->lock);
    if(is_empty(sleeping_list))
    {
      release(&sleeping_list->lock);
      return;
    }
    pred = get_proc_by_index(sleeping_list->head);
    if(pred==0)
    {
      release(&sleeping_list->lock);
      return;
    }

    acquire(&pred->node_lock);
    release(&sleeping_list->lock);
    acquire(&pred->lock);
  
    if(pred->chan == chan)
    {
      pred->state = RUNNABLE;
      //todo: check if we need two locks for one proc?
      int cpu_num = pred->cpu_num;
      release(&pred->node_lock);
      remove_proc(pred->index,sleeping_list);
      add_proc_to_tail(pred->index, get_ready_list(cpu_num));
      release(&pred->lock);
      continue;
    }
    release(&pred->lock);
    found_proc_to_wakeup = FALSE;
    while(has_next(pred))
    {
      curr = get_proc_by_index(pred->next_proc_index);

      if(curr==0)
      {
        release(&pred->lock);
        return;
      }
      acquire(&curr->node_lock);
      release(&pred->node_lock);
      pred = curr;
      acquire(&pred->lock);

      if(pred->chan == chan) {
        pred->state = RUNNABLE;
        int cpu_num = pred->cpu_num;
        release(&pred->node_lock);
        remove_proc(pred->index,sleeping_list);
        add_proc_to_tail(pred->index, get_ready_list(cpu_num));
        release(&pred->lock);
        found_proc_to_wakeup = TRUE;
        break;
      }
      release(&pred->lock);
    }
    if(!found_proc_to_wakeup)
    {
      release(&pred->node_lock);
      return;
    }
  }
}



//todo delete
/*
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
        remove_proc(p->index,sleeping_list);
        add_proc_to_tail(p->index, get_ready_list(p->cpu_num));
      }
      release(&p->lock);
    }
  }
}
*/

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
//todo: imp -> git
int
kill(int pid)
{
                                                                          printf("in kill func\n");

  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
                                                                            printf("in either_copyout func\n");

  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
                                                                              printf("in either_copyin func\n");

  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
                                                                                printf("in procdump func\n");

  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

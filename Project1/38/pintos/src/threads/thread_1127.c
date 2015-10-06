#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "../devices/timer.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif



/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list ready_list;

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

//#ifndef USERPROG
//project 3
bool thread_prior_aging;
//#endif

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);

/* ********************USER CODE ******************** */


void initmutexUser(int *lock){
	//lock_init(lock);
	*lock = FALSE;
}

void acquireUser(int *lock){

	while(1){
			if( *lock == TRUE)
					break;
			/*
			for(i=0;i<100;i++){
				result[i] = a[i] *a[i] * b[i] * b[i];
			}
			*/
			thread_yield();
			//Sleep(1);
			//sleep(1);
	}
	*lock = FALSE;
		
}

void releaseUser(int *lock){
	*lock = TRUE;
}


/* ************************************************** */

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
void
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();
	/* project3 */
	load_avg = 0;
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void) 
{
  /* Create the idle thread. */
  struct semaphore start_idle;
  sema_init (&start_idle, 0);
  thread_create ("idle", PRI_MIN, idle, &start_idle);

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&start_idle);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void) 
{
  struct thread *t = thread_current ();
	int avg;
  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  /* Enforce preemption. */
	/* it is stopped because round-robbin is changed by priority */

	if (++thread_ticks >= TIME_SLICE){
		intr_yield_on_return ();
	}

  //project 3333333
	if(thread_mlfqs && (timer_ticks()%TIMER_FREQ==0) )
	{
			thread_get_load_avg_update();
			thread_foreach(thread_get_recent_cpu_update,NULL);
	}

	if(thread_mlfqs && (thread_current()!=idle_thread))
			(thread_current()->recent_cpu) += 1*F;
	
	if(thread_mlfqs && (timer_ticks() % TIME_SLICE==0))
	{
			thread_foreach(thread_priority_update, NULL);
	}


 /*	
	if(thread_mlfqs && (thread_current()!=idle_thread))
//			thread_current()->recent_cpu+=F;

	if( thread_mlfqs && (timer_ticks() % TIMER_FREQ == 0)){
		thread_get_load_avg_update();
		//thread_get_recent_cpu_update();
	//	thread_foreach(thread_get_recent_cpu_update,NULL);
	}
	
	if( thread_mlfqs && (timer_ticks() % TIME_SLICE == 0)){
	//	thread_foreach(thread_priority_update,NULL);
	}	
 	*/
  if(thread_prior_aging==true)
	  thread_aging();

}

/* Prints thread statistics. */
void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;
  enum intr_level old_level;

	//printf("%s CREATE \n",name);
	

  ASSERT (function != NULL);

  if(function == NULL){
	//printf(" CREATE FUNC ERROR \n");
	return TID_ERROR;
  }

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  /* Prepare thread for first run by initializing its stack.
     Do this atomically so intermediate values for the 'stack' 
     member cannot be observed. */
  old_level = intr_disable ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  //성철 parent, child 설정
  struct thread* cur=thread_current();
  t->parent= cur;	//새로 생성한 thread의 parent는 현재 thread
  list_push_back(&(cur->childList),&(t->childElem));		//현재 쓰레드의 child는 새로 생성한 thread
  intr_set_level (old_level);
	
  /* Add to run queue. */
  thread_unblock (t);
  //project 3
  //생성하려는 thread의 priority가 현재 thread의 priority 보다 크면 스케쥴링을 해준다.
  if(priority > cur->priority)
 	  thread_yield();
	/* load_avg,recent_cpu,Nice of child is inherited by parent */
	t->nice = cur->nice;
	t->recent_cpu = cur->recent_cpu;

	//printf("%d\n", realToInt(1));
  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
    
   This is an error if T is not blocked.  (Use thread_yield() to
	 	make the running thread ready.)

   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;
 	struct thread *cur = thread_current();
 	ASSERT (is_thread(t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
	list_push_back (&ready_list, &t->elem);
//	list_insert_ordered(&ready_list, &t->elem,priority_less,NULL);
  t->status = THREAD_READY;

  intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char *
thread_name (void) 
{
  return thread_current ()->name; 
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();
  
  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void) 
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void) 
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
 	//	list_insert_ordered(&ready_list, &cur->elem, priority_less,NULL);
	  list_push_back (&ready_list, &cur->elem);
	//list_push_priority(&ready_list, &cur->elem);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}

				/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority) 
{
  thread_current ()->priority = new_priority;
  //project 3
  //priority가 변경되면 새로 스케쥴링을 해준다.
  thread_yield();

}

/* Returns the current thread's priority. */
int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice ) 
{
  /* Not yet implemented. */
	if(nice >20)
		thread_current ()-> nice = 20;
	else if(nice <-20)
		thread_current ()-> nice = -20;
	else
		thread_current ()-> nice = nice;

	//thread_foreach(thread_priority_update,NULL);
//	thread_yield();
	// priority의 재계산이 위치하여야 함
	// running thread가 더이상 높은 pri가 아니면 yield();
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
  /* Not yet implemented. */
  return thread_current() -> nice;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
	/* It is only used to get not calculate load_avg          */
  /* load_avg is implemented as fixed-point real arithmetic */

  //return 100*((load_avg+F/2)/F);
	return (100*(load_avg))/F;
}
void
thread_get_load_avg_update (void){

		int a, b;
		int size=0;
		int idle_flag=0;

		enum intr_level old_level = intr_disable();
		if(thread_current()!=idle_thread)
			idle_flag++;
		size=list_size(&ready_list)+idle_flag;
		a= (((int64_t)(59*F)/60)*load_avg)/F;
		b= (((int64_t)((1*F)/60))*(size*F))/F;
		load_avg=a+b;
		intr_set_level(old_level);
			
}

int ready_size(void){
	return list_size(&ready_list);
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
	/* It is only used to get not calculate recent_cpu          */
  /* recent_cpu is implemented as fixed-point real arithmetic */
	//printf("THREAD RECENT CPU %d \n", thread_current() -> recent_cpu);
  return 100 * (thread_current() -> recent_cpu) / F;
}



void thread_get_recent_cpu_update( struct thread *t, void *aux){
			int a,b,c,d;

				if( t->status != THREAD_DYING && t!=idle_thread ){
		    	//t->recent_cpu = real_divide(real_multi(load_avg * 2,(t->recent_cpu)),(load_avg *2 + intToReal(1))) + intToReal(t-> nice);
				a=2*load_avg;
				b=(2*load_avg)+F;
				c=(((int64_t)a)*(t->recent_cpu))/F;
				d= (((int64_t)c)*F)/b;
				t->recent_cpu = d+((t->nice)*F);	
				}
}

/*
void
thread_get_recent_cpu_update(void)
{
		struct thread *t;
		struct list_elem *e;

		int a,b,c,d;

		for(e = list_begin(&all_list) ; e != list_end(&all_list) ; e=list_next(e))
		{
				t = list_entry(e,struct thread,elem);
				if( t->status != THREAD_DYING && t!=idle_thread )
		    	//t->recent_cpu = real_divide(real_multi(load_avg * 2,(t->recent_cpu)),(load_avg *2 + intToReal(1))) + intToReal(t-> nice);
				a=2*load_avg;
				b=(2*load_avg)+F;
				c=(((int64_t)a)*(t->recent_cpu))/F;
				d= (((int64_t)c)*F)/b;
				t->recent_cpu = d+((t->nice)*F);	
		}
}
*/

void thread_priority_update( struct thread *t, void *aux){
		    int a, b, c;

				a=(t->recent_cpu/4);
				b=a/F;

				t->priority = PRI_MAX - b - (t->nice * 2);
}
/*
void
thread_priority_update(void){
		
		struct thread *t;
		struct list_elem *e;

		for(e = list_begin(&all_list) ; e != list_end(&all_list) ; e=list_next(e)){
				t = list_entry(e,struct thread,elem);
		    t->priority = PRI_MAX - realToInt(100 * t->recent_cpu) / 4 - (t->nice * 2);
				//printf(" PRIORITY %d \n", t->recent_cpu);
		}
}
*/
int intToReal (int integer){
	return integer * F;
}

int realToInt (int real){
	return (real>=0)?(real + F/2) / F : (real-F/2) /F;
}

int real_multi (int a, int b){
	/* a by b */
	int tmp;
	tmp = ((int64_t)a)*b/F;
	return tmp; 
}
int real_divide (int a, int b){
	/* a over b */
	return ((int64_t)a)*F/b;
}
/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;) 
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

      /* Re-enable interrupts and wait for the next one.

         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.

         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void) 
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->priority = priority;
  t->magic = THREAD_MAGIC;
  list_push_back (&all_list, &t->allelem);

  //성철 2014 10 25 parent, child, exit_status 초기화
  t->parent=NULL;
  list_init(&t->childList);
  t->exit_status=0;

  //마지막날 수정 
  initmutexUser(&t->lockUser);
  initmutexUser(&t->lock2User); 	  
  
  initmutexUser(&t->lockUser_missingfile);

  //project 2
  list_init(&t->list_fdSet);
  t->fdCnt=0;
  //project 3
  t->wakeUpTime = 1000000;
	t->nice = t->recent_cpu = 0;
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void) 
{
 	
   	if (list_empty (&ready_list))
	    return idle_thread;
	else
	{
		list_sort(&ready_list, priority_less, NULL);
	 	return list_entry (list_pop_front (&ready_list), struct thread, elem);
		
	//	struct list_elem *e=list_max(&ready_list, priority_for_max, NULL);
	//	return list_entry(e, struct thread, elem);
		//return list_entry (list_max(&ready_list,priority_for_max,NULL), struct thread, elem);
}
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.

   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.

   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);

bool priority_less(const struct list_elem *a, const struct list_elem *b, void *aux)
{
	struct thread *t1;
	struct thread *t2;

	t1=list_entry(a, struct thread, elem);
	t2=list_entry(b, struct thread, elem);

	if(t1->priority > t2->priority )
		return true;
	else if(t1->priority == t2->priority){
		if(!strcmp(t1->name,"main"))
						return true;
		else
						return false;
	}
	else
		return false;
}


bool priority_for_max(const struct list_elem *a, const struct list_elem *b, void *aux)
{
	struct thread *t1;
	struct thread *t2;

	t1=list_entry(a, struct thread, elem);
	t2=list_entry(b, struct thread, elem);

	if(t1->priority < t2->priority)
		return true;
	else
		return false;
}


bool wakeUpTime_less(const struct list_elem *a, const struct list_elem *b, void *aux)
{
	struct thread *t1;
	struct thread *t2;

	t1=list_entry(a, struct thread, elem);
	t2=list_entry(b, struct thread, elem);

	if(t1->wakeUpTime < t2->wakeUpTime)
		return true;
	else
		return false;
}

void list_push_priority(struct list *list, struct list_elem *new)
{
	struct list_elem *e;
	struct thread *t;
	struct thread *newThread = list_entry(new, struct thread, elem);
	
	int end_flag=1;

	for(e=list_begin(list) ; e!=list_end(list) ; e=list_next(e))
	{
		t=list_entry(e, struct thread, elem);
		if(newThread->priority > t->priority)
		{
			end_flag=0;
			break;
		}	
	}
	if(end_flag)		//newThread의 priority가 가장 낮은경우
	{
		list_push_back(&ready_list, &newThread->elem);
	}
	else
	{
		list_insert(&t->elem,&newThread->elem);
	}
}

void thread_aging()
{
	struct thread *t;
	struct list_elem *e;

	for(e=list_begin(&ready_list) ; e!=list_end(&ready_list) ; e=list_next(e))
	{
		t=list_entry(e, struct thread, elem);
		t->priority++;


	}
}

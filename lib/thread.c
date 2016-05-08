// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>


struct jthread_thread_info_t {
	bool in_use;
	// the top of the stack
	uint32_t stack_top;
	// thread id
	jthread_t tid;
	// index of the next thread in the pool
	// used for blocked thread list and free thread list
	int next;
};

#define MAX_THREADS 1000

static struct jthread_thread_info_t thread_pool[MAX_THREADS];

static int head;

// the destination code for the newly created thread
static void (*dst_code)(void *);
// arguments for the new thread
static void *args;

// the big thread pool lock;
static struct jthread_spinlock_t lock;

// initialize all the data structures
void jthread_lib_init_thread_pool() {
	int i;
	
	// the stack must below the main thread's stack
	// and an empty page is left there for protection
	uint32_t stack_top = USTACKTOP - 2 * PGSIZE;
	// initialize the thread pool
	for(i = 0; i < MAX_THREADS; i++) {
		thread_pool[i].in_use = false;
		thread_pool[i].stack_top = stack_top;
		// each thread has a stack of one page size
		// and one empty page for detecting stack overflow
		stack_top -= 2 * PGSIZE;
		thread_pool[i].tid = -1;
		thread_pool[i].next = i + 1;
	}
	// terminating the free list
	thread_pool[MAX_THREADS - 1].next = -1;

	// initialize the free blocked thread list
	head = 0;

	// initialize the big lock
	jthread_spinlock_init(&lock, 0);
};


static void __attribute__ ((noinline))thread_entry(void (*func)(void *), void *args);

jthread_t
jthread_create(void (*func)(void *), void *arg)
{
	// the big thread pool lock
	jthread_spinlock_lock(&lock);

	int slot = -1;
	if(head != -1) {
		slot = head;
		head = thread_pool[head].next;
		thread_pool[slot].in_use = true;
		thread_pool[slot].next = -1;
	}
	else
		panic("jthread_create: thread_pool full!\n");

	envid_t envid;
	int r;
	// Allocate a new child environment.
	envid = sys_exovfork(thread_pool[slot].stack_top);
	if (envid < 0)
		panic("sys_exovfork: %e", envid);

	if (envid == 0) {
		// when the child start running
		// it's stack has been switched
		// so we only need to make a function call
		// to change it's eip
		thread_entry(dst_code, args);
		panic("jthread_create: should not reach here!\n");
		return 0;
	}
	else {
		// in order to transfer these arguments to the new thread
		// we can put it on the child's stack
		// but I choose the simplest way.
		// use the global variable to do this
		// but in order to avoid race conditions
		// I use a spin lock
		thread_pool[slot].tid = envid;

		dst_code = func;
		args = arg;
		// Start the child environment running
		if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
			panic("sys_env_set_status: %e", r);
		}
		return envid;
	}
}

// this function should not be inlined
// we do not inline this, so that a copy of correct dst_code
// will stay in the new thread's stack
// only then we can safely unlock the lock
static void __attribute__ ((noinline))thread_entry(void (*func)(void *), void *args) {
	// unlock the lock
	jthread_spinlock_unlock(&lock);
	func(args);
	// thread exited, free it
	jthread_exit();
	panic("thread_entry: should not reach here!\n");
}

// temporarily give up CPU
void jthread_yield() {
	sys_yield();
}

static int find_slot(void);

// does not free the thread stack
// since I do not how to do it
void jthread_exit() {
	// acquire the lock
	jthread_spinlock_lock(&lock);
	// return the slot back to the pool
	int slot = find_slot();
	thread_pool[slot].next = head;
	thread_pool[slot].in_use = false;
	thread_pool[slot].tid = -1;
	
	head = slot;

	// release the lock
	jthread_spinlock_unlock(&lock);

	exit();

	panic("jthread_exit: should not reach here!\n");
}

static int find_slot(void) {
	// first we need to get the eip
	// and from the esp, we can guess
	// which slot it stays
	uint32_t esp;
	asm volatile("movl %%esp, %0" : 
			"=m"(esp) : 
			:);
	esp = ROUNDUP(esp, PGSIZE);
	if(esp >= USTACKTOP)
		return -1;
	int index = (USTACKTOP - esp) / (2 * PGSIZE) - 1;
	cprintf("index: %d\n", index);
	return index;
}

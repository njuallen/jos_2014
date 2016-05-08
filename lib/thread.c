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

static int thread_head;

// the destination code for the newly created thread
static void (*dst_code)(void *);
// arguments for the new thread
static void *args;

// the big thread pool lock;
static struct jthread_spinlock_t thread_pool_lock;

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
	thread_head = 0;

	// initialize the big lock
	jthread_spinlock_init(&thread_pool_lock, 0);
};


static void __attribute__ ((noinline))thread_entry(void (*func)(void *), void *args);

jthread_t
jthread_create(void (*func)(void *), void *arg)
{
	// the big thread pool lock
	jthread_spinlock_lock(&thread_pool_lock);

	int slot = -1;
	if(thread_head != -1) {
		slot = thread_head;
		thread_head = thread_pool[thread_head].next;
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
	jthread_spinlock_unlock(&thread_pool_lock);
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
	jthread_spinlock_lock(&thread_pool_lock);
	// return the slot back to the pool
	int slot = find_slot();
	thread_pool[slot].next = thread_head;
	thread_pool[slot].in_use = false;
	thread_pool[slot].tid = -1;
	
	thread_head = slot;

	// release the lock
	jthread_spinlock_unlock(&thread_pool_lock);

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
	return index;
}

// a user space semaphore

#define MAX_SEMA 1000

struct jthread_sema_info_t {
	// the lock is used to protect val
	struct jthread_spinlock_t lock;
	// the value of the sema
	unsigned int val;
	// the index of the blocked thread in the thread pool
	int blocked_thread;
	// free items is orgnized as a singly linked list
	int next;
};

static int sema_head;

// lock used to protect sema_head
static struct jthread_spinlock_t sema_pool_lock;

static struct jthread_sema_info_t sema_pool[MAX_SEMA];

// initialize the semaphore pool
void jthread_lib_init_sema_pool() {
	int i;

	for(i = 0; i < MAX_SEMA; i++) {
		sema_pool[i].val = 0;
		sema_pool[i].blocked_thread = -1;
		sema_pool[i].next = i + 1;
	}
	// terminating the free list
	sema_pool[MAX_SEMA - 1].next = -1;

	// initialize the free list sema_head
	sema_head = 0;

	// initialize the sema pool lock
	jthread_spinlock_init(&sema_pool_lock, 0);
};

int jthread_sema_init(struct jthread_sema_t *m, int val) {
	int slot = -1;
	jthread_spinlock_lock(&sema_pool_lock);
	// no more free slot
	// simply panic
	if(sema_head == -1)
		panic("jthread_sema_init: no semaphore available\n");
	else {
		slot = sema_head;
		sema_head = sema_pool[sema_head].next;
	}
	jthread_spinlock_unlock(&sema_pool_lock);
	m->sema_id = slot;
	sema_pool[slot].val = val;
	sema_pool[slot].next = -1;
	sema_pool[slot].blocked_thread = -1;
	jthread_spinlock_init(&sema_pool[slot].lock, 0);
	return 0;
};

// v
void jthread_sema_post(struct jthread_sema_t *m) {
	int slot = m->sema_id;

	// acquire exclusive access to the semaphore
	jthread_spinlock_lock(&sema_pool[slot].lock);

	int thread = sema_pool[slot].blocked_thread;
	
	// someone is blocked, so we need to wake it up
	if(thread != -1) {
		// now we need to modify the thread_pool
		jthread_spinlock_lock(&thread_pool_lock);
		
		// remove this one out of the sleeping queue
		sema_pool[slot].blocked_thread = thread_pool[thread].next;
		thread_pool[thread].next = -1;
		envid_t envid = thread_pool[thread].tid;
		jthread_spinlock_unlock(&thread_pool_lock);
		jthread_spinlock_unlock(&sema_pool[slot].lock);

		// try wake it up
		// you must send a nonzero value
		// since in ipc_recv we use 0 to indicate error
		ipc_send(envid, 1, NULL, 0);
	}
	// no one is blocked, we simply imcrement val
	else {
		sema_pool[slot].val++;
		jthread_spinlock_unlock(&sema_pool[slot].lock);
	}
};

int jthread_sema_trylock(struct jthread_sema_t *m) {
	return 0;
};

// P
void jthread_sema_wait(struct jthread_sema_t *m) {
	int slot = m->sema_id;
	// acquire exclusive access to the semaphore
	jthread_spinlock_lock(&sema_pool[slot].lock);

	// do not need to be blocked
	if(sema_pool[slot].val > 0) {
		sema_pool[slot].val--;
		jthread_spinlock_unlock(&sema_pool[slot].lock);
	}
	else {
		// add ourself to the blocked queue
		
		// which slot in thread_pool does current thread reside?
		int curr = find_slot(); 
		// now we need to modify the thread_pool
		jthread_spinlock_lock(&thread_pool_lock);
		thread_pool[curr].next = sema_pool[slot].blocked_thread;
		sema_pool[slot].blocked_thread = curr;

		jthread_spinlock_unlock(&thread_pool_lock);
		jthread_spinlock_unlock(&sema_pool[slot].lock);

		// use this to block ourselves
		// well, we can not use the while loop below to implement this
		// since ipc_recv uses the global variable thisenv to get the transmitted value
		// but the thisenv problem remains unsolved
		// so in most cases it will return 0
		// so we can not use 0 to distinguish error between transmitted value
		// so just assume that ipc_recv will never error!
		//while(ipc_recv(NULL, NULL, NULL) == 0)
			//jthread_yield();
		ipc_recv(NULL, NULL, NULL);
	};
};

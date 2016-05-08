#include <inc/string.h>
#include <inc/lib.h>

// a user space lock

tid_t Jthread_create();
tid_t Jthread_yield();
tid_t Jthread_exit();

// user should only see this
struct Jthread_mutex_t {
	// the id of the mutex
	unsigned int mutex_id;
};


#define MAX_MUTEX 1000

struct Jthread_lib_mutex_pool_t {
	bool in_use;
	// the value of the mutex
	unsigned int val;
	// the corresponding sleeping queue
	// index into the sleeping queue pool
	int block_queue;
	// free items is orgnized as a singly linked list
	int next;
};

static int mutex_pool_free_head;

static Jthread_lib_mutex_pool_t mutex_pool[MAX_MUTEX];

#define MAX_BLOCKED_THREAD 10000

static int thread_pool_free_head;

// the pool for blocked threads
struct Jthread_lib_thread_pool_t {
	bool in_use;
	tid_t tid;
	int next;
};

static Jthread_lib_thread_pool_t thread_pool[MAX_BLOCKED_THREAD];

// initialize all the data structures
void Jthread_lib_init_first() {
	int i;
	// initialize mutex pool
	for(i = 0; i < MAX_MUTEX; i++) {
		mutex_pool[i].in_use = false;
		mutex_pool[i].val = 0;
		mutex_pool[i].block_queue = -1;
		mutex_pool[i].next = -1;
	}
	// initialize the free mutex list
	mutex_pool_free_head = -1;

	// initialize the blocked thread pool
	for(i = 0; i < MAX_BLOCKED_THREAD; i++) {
		thread_pool[i].in_use = false;
		thread_pool[i].tid = -1;
		thread_pool[i].next = -1;
	}

	// initialize the free blocked thread list
	thread_pool_free_head = -1;

};

int Jthread_mutex_init(struct Jthread_mutex_t &m) {
};

int Jthread_mutex_lock(struct Jthread_mutex_t &m) {
};

int Jthread_mutex_trylock(struct Jthread_mutex_t &m) {
};

int Jthread_mutex_unlock(struct Jthread_mutex_t &m) {
};

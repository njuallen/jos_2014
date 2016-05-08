// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>
#include <inc/x86.h>


void jthread_spinlock_init(struct jthread_spinlock_t *lock, uint32_t value) {
	lock->val = value;
}

void jthread_spinlock_lock(struct jthread_spinlock_t *lock) {
	while(xchg(&lock->val, 1) != 0)
		// well, wait for a while
		sys_yield();
}

void jthread_spinlock_unlock(struct jthread_spinlock_t *lock) {
	xchg(&lock->val, 0);
}

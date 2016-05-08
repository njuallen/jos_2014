// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two with fork.

#include <inc/lib.h>

struct jthread_sema_t s1;
struct jthread_sema_t s2;

void ping(void *arg);
void pong(void *arg);

void
umain(int argc, char **argv)
{
	jthread_sema_init(&s1, 1);
	jthread_sema_init(&s2, 0);
	jthread_create(ping, NULL);
	jthread_create(pong, NULL);
	while(1);
}

void ping(void *arg) {
	while(1) {
		jthread_sema_wait(&s1);
		cprintf("ping\n");
		jthread_sema_post(&s2);
	}
}

void pong(void *arg) {
	while(1) {
		jthread_sema_wait(&s2);
		cprintf("pong\n");
		jthread_sema_post(&s1);
	}
}

// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two with fork.

#include <inc/lib.h>

int s1;
int s2;

void ping(void *arg);
void pong(void *arg);

void
umain(int argc, char **argv)
{
	s1 = sys_sem_open(1);
	s2 = sys_sem_open(0);
	jthread_create(ping, NULL);
	jthread_create(pong, NULL);
	while(1);
}

void ping(void *arg) {
	while(1) {
		sys_sem_wait(s1);
		cprintf("ping\n");
		sys_sem_post(s2);
	}
}

void pong(void *arg) {
	while(1) {
		sys_sem_wait(s2);
		cprintf("pong\n");
		sys_sem_post(s1);
	}
}

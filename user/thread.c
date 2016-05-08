// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two, crudely.

#include <inc/string.h>
#include <inc/lib.h>

void thread(void *arg);

	void
umain(int argc, char **argv)
{

	int message = 1;
	jthread_create(thread, &message);

	int message2 = 2;
	jthread_create(thread, &message2);

	sys_sleep(200);

	int message3 = 3;
	jthread_create(thread, &message3);

}

void thread(void *arg) {
	int message = *((int *)arg);
	cprintf("%d started\n", message);
	jthread_yield();
	cprintf("%d is exiting\n", message);
}

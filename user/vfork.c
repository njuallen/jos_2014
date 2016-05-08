// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two, crudely.

#include <inc/string.h>
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t who;
	int i;

	// fork a child process
	who = sys_exovfork(0xa0000000);
	if(who == 0) {
		int a = 0;
		int j;
		for(j = 0; j < 9; j++)
			cprintf("child: %d\n", a);
	}
	else {
		int a = 1;
		int j;
		for(j = 0; j < 9; j++)
			cprintf("father: %d\n", a);
	}
}


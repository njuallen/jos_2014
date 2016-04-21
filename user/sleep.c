// used to test the sys_sleep syscall, it print an message every 2 seconds 

#include <inc/lib.h>


void
umain(int argc, char **argv)
{
	if(fork() == 0) {
		while(1) {
			cprintf("miao miao miao!\n");
			sys_sleep(200);
		}
	}
	while(1);
}


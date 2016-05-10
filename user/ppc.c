#include <inc/lib.h>

#define N 10
// the number of producers
#define NP 5
// the number of consumers
#define NC 5

int sem_mutex;
int sem_empty;
int sem_full;

envid_t Fork();

void
umain(int argc, char **argv)
{
	if((sem_mutex = sys_sem_open(1)) < 0)
		panic("sem_mutex: can not open semaphore!\n");

	if((sem_empty = sys_sem_open(N)) < 0)
		panic("sem_empty: can not open semaphore!\n");

	if((sem_full = sys_sem_open(0)) < 0)
		panic("sem_full: can not open semaphore!\n");

	int i;
	
	// create producers
	for(i = 0; i < NP; i++)
		if(Fork() == 0)
			while(1) {
				sys_sem_wait(sem_empty);
				cprintf("produce\n");
				sys_sem_post(sem_full);
			}
		
	// create consumers
	for(i = 0; i < NC; i++)
		if(Fork() == 0)
			while(1) {
				sys_sem_wait(sem_full);
				cprintf("consume\n");
				sys_sem_post(sem_empty);
			}
}

envid_t Fork() {
	envid_t envid = fork();
	if(envid < 0)
		panic("Fork: can not fork!\n");
	return envid;
}



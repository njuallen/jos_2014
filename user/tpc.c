#include <inc/lib.h>

#define N 10
// the number of producers
#define NP 5
// the number of consumers
#define NC 5

int sem_mutex;
int sem_empty;
int sem_full;

void producer(void *arg);
void consumer(void *arg);

int buffer[N + 1];

int head = 0;

int tail = 0;

void insert(int product) {
	buffer[tail++] = product;
	tail %= N + 1; 
}

int remove() {
	int product = buffer[head++];
	head %= N + 1;
	return product;
}


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

	int producer_arg[NP];
	int consumer_arg[NC];

	// create producers
	for(i = 0; i < NP; i++) {
		producer_arg[i] = i;
		jthread_create(producer, &producer_arg[i]);
	}

	// create consumers
	for(i = 0; i < NC; i++) {
		consumer_arg[i] = i;
		jthread_create(consumer, &consumer_arg[i]);
	}

	while(1)
		;
}

void producer(void *arg) {
	int id = *((int *)arg);
	while(1) {
		sys_sem_wait(sem_empty);
		cprintf("produce\n");
		insert(-id);
		cprintf("producer %d produced product %d\n", id, -id);
		sys_sem_post(sem_full);
	}
}

void consumer(void *arg) {
	int id = *((int *)arg);
	while(1) {
		sys_sem_wait(sem_full);
		cprintf("consume\n");
		int product = remove();
		cprintf("consumer %d consumed product %d\n", id, product);
		sys_sem_post(sem_empty);
	}
}

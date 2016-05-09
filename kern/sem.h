/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_SEM_H
#define JOS_KERN_SEM_H

#define NSEM 1000

enum {
	SEM_FREE = 0,
	SEM_USING
};

struct Sem {
	uint32_t sem_status;
	// the value of the sem
	uint32_t sem_value;
	// the pointer to the blocked env in the env pool
	struct Env *sem_env;
	// free items is orgnized as a singly linked list
	int32_t sem_link;
};

extern struct Sem *sems;

void sem_init(void);
int sem_open(int val);
int sem_close(int sem_id);
int sem_post(int sem_id);
int sem_wait(int sem_id);

#endif // !JOS_KERN_SEM_H

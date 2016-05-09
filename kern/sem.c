#include <inc/x86.h>
#include <inc/error.h>

#include <kern/sem.h>
#include <kern/env.h>
#include <kern/sched.h>



static int32_t sem_free_list;


struct Sem *sems = NULL;

// initialize the semphore pool
void sem_init(void) {
	int i;

	for(i = 0; i < NSEM; i++) {
		sems[i].sem_status = SEM_FREE;
		sems[i].sem_value = 0;
		sems[i].sem_env = NULL;
		sems[i].sem_link = i + 1;
	}
	// terminating the list
	sems[NSEM - 1].sem_link = -1;

	// initialize the free list
	sem_free_list = 0;
};

int sem_open(int val) {
	int sem_id = -1;
	// no free sem
	if(sem_free_list == -1)
		return -E_NO_FREE_SEM;
	else {
		sem_id = sem_free_list;
		sem_free_list = sems[sem_free_list].sem_link;
		sems[sem_id].sem_status = SEM_USING;
		sems[sem_id].sem_value = val;
		sems[sem_id].sem_link = -1;
		sems[sem_id].sem_env = NULL;
		return sem_id;
	}
};

int sem_close(int sem_id) {

	// check whether sem_id is in range
	if(sem_id < 0 || sem_id >= NSEM)
		return -E_INVAL;
	// check whether the semaphore is in use
	if(sems[sem_id].sem_status != SEM_USING)
		return -E_INVAL;

	sems[sem_id].sem_status = SEM_FREE;
	sems[sem_id].sem_value = 0;
	sems[sem_id].sem_link = sem_free_list;
	sems[sem_id].sem_env = NULL;
	sem_free_list = sem_id;
	return 0;
};

// v
int sem_post(int sem_id) {

	// check whether sem_id is in range
	if(sem_id < 0 || sem_id >= NSEM)
		return -E_INVAL;
	// check whether the semaphore is in use
	if(sems[sem_id].sem_status != SEM_USING)
		return -E_INVAL;

	struct Env *env = sems[sem_id].sem_env;

	// someone is blocked, so we need to wake it up
	if(env) {
		// remove this one out of the sleeping queue
		sems[sem_id].sem_env = env->env_link;
		env->env_link = NULL;

		// try wake it up
		env->env_status = ENV_RUNNABLE;
	}
	// no one is blocked, we simply imcrement val
	else
		sems[sem_id].sem_value++;
	return 0;
};

int sem_trylock(int sem_id) {
	return 0;
};

// P
int sem_wait(int sem_id) {

	// check whether sem_id is in range
	if(sem_id < 0 || sem_id >= NSEM)
		return -E_INVAL;
	// check whether the semaphore is in use
	if(sems[sem_id].sem_status != SEM_USING)
		return -E_INVAL;

	// do not need to be blocked
	if(sems[sem_id].sem_value > 0)
		sems[sem_id].sem_value--;
	else {
		// add ourself to the blocked queue
		curenv->env_link = sems[sem_id].sem_env;
		sems[sem_id].sem_env = curenv;
		// mark as not runnable
		curenv->env_status = ENV_NOT_RUNNABLE;
		// do not forget to prepare the syscall return value
		curenv->env_tf.tf_regs.reg_eax = 0;
		// give up cpu
		sched_yield();
	};
	return 0;
};

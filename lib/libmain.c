// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, uvpd, and uvpt.

#include <inc/lib.h>

extern void umain(int argc, char **argv);

const volatile struct Env *thisenv;
const char *binaryname = "<unknown>";

#define debug 0

void
libmain(int argc, char **argv, char *pwd)
{
	// set thisenv to point at our Env structure in envs[].
	// LAB 3: Your code here.
	envid_t envid = sys_getenvid();

	// update working directory
	extern char working_directory[MAXPATHLEN];
	if(pwd) {
		if(debug)
			printf("libmain pwd:%s\n", pwd);
		strncpy(working_directory, pwd, MAXPATHLEN);
	}

	thisenv = envs;
	thisenv += ENVX(envid); 

	// save the name of the program so that panic() can use it
	if (argc > 0)
		binaryname = argv[0];

	jthread_lib_init_thread_pool();
	jthread_lib_init_sema_pool();
	// call user main routine
	umain(argc, argv);

	// exit gracefully
	exit();
}

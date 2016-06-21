#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int f, i;

	binaryname = "mkdir";
	for (i = 1; i < argc; i++) {
		f = open(argv[i], O_CREAT | O_MKDIR);
		if (f < 0)
			printf("can't create %s\n", argv[i]);
	}
}

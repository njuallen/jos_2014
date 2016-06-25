#include <inc/lib.h>

int *p[100];

void
umain(int argc, char **argv)
{
	int i;
	for(i = 0; i < 100; i++) {
		p[i] = (int *)malloc((i + 1) * sizeof(int));
		int j;
		for(j = 0; j < i + 1; j++)
		   p[i][j] = j;	
	}
	for(i = 0; i < 100; i++) {
		int j;
		for(j = 0; j < i + 1; j++)
		   if(p[i][j] != j)
			   panic("test failed!\n");
	}
	for(i = 0; i < 100; i++) {
		free(p[i]);
	}
}

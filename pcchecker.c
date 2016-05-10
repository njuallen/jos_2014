#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE 1024
int main(int argc, char **argv) {
	int n;
	if((argc < 2) || 
			(sscanf(argv[1], "%d", &n) != 1)) {	
		fprintf(stderr, "check expect a number which means the size of the buffer\n");
		exit(1);
	}
	int product = 0;
	char buf[MAX_LINE];
	while(fgets(buf, MAX_LINE, stdin) != NULL) {
		// remove the newline
		buf[strlen(buf) - 1] = '\0';
		// it's the producer
		if(strcmp(buf, "produce") == 0) {
			product++;
			if(product > n) {
				fprintf(stderr, "wrong\n");
				exit(2);
			}
		}
		// 
		// it's the consumer
		if(strcmp(buf, "consume") == 0) {
			product--;
			if(product < 0) {
				fprintf(stderr, "wrong\n");
				exit(2);
			}
		}
	}
	fprintf(stderr, "correct\n");
	return 0;
}


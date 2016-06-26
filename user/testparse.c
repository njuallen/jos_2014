#include <inc/lib.h>

// use this to test parse
void
umain(int argc, char **argv) {
	char *str = "this/is/a/parse\\test/miao";
	char *sep = "/\\";
	get_token_init(str, sep);
	char *s;
	while((s = get_token()))
		printf("%s\n", s);

	printf("************\n");
	sep = "/";
	get_token_init(str, sep);
	while((s = get_token()))
		printf("%s\n", s);
}

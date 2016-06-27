#include <inc/lib.h>

// a simple error handling function for user program
// it prints some message and exits
void
perror(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	// Print the error message
	vcprintf(fmt, ap);
	cprintf("\n");

	exit();
}

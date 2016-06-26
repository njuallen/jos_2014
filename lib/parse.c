#include <inc/lib.h>

static char *s = NULL;
static char *sep = NULL;
static char *p = NULL;

void get_token_init(const char *str, const char *separator) {
	s = strcopy(str);
	sep = strcopy(separator);
	p = s;
}

char *get_token(void) {
	char *ptr = p;
	char *ret = NULL;
	bool match;
	while(*ptr) {
		match = false;
		char *sep_ptr = sep;
		while(*sep_ptr)
			if(*sep_ptr == *ptr) {
				match = true;
				break;
			}	   
			else 
				sep_ptr++;

		if(match) {
			ret = substr(p, ptr - p);
			p = ptr + 1;
			return ret;
		}
		else
			ptr++;
	}


	if(*p) {
		char *start = p;
		p = ptr;
		return substr(start, ptr - start);
	}
	else {
		free(s);
		free(sep);
	}
	return NULL;
}

#include <inc/lib.h>

#define debug 0

// a naive malloc-free implementation

// the heap grows from 0x90000000
#define HEAP_BOTTOM 0x90000000

#define HEAP_LIMIT 0xD0000000

static unsigned int heap_top = HEAP_BOTTOM;

void *malloc(unsigned int size) {
	if(debug)
		cprintf("size: %d ", size);
	// round up size to make it align on 16 bytes
	size = ROUNDUP(size, 16);
	// acquire enough memory
	unsigned int start = ROUNDDOWN(heap_top, PGSIZE);
	unsigned int end = ROUNDUP(heap_top + size, PGSIZE);
	if(debug)
		cprintf("size: %d start: %x end: %x\n", size, start, end);

	// do not touch the fd page
	if(end >= HEAP_LIMIT)
		return NULL;	

	// get the id the current environment
	envid_t id = sys_getenvid();


	unsigned int addr;
	for(addr = start; addr < end; addr += PGSIZE)
		// the page has not been mapped
		if (!(uvpd[PDX(addr)] & PTE_P) || !(uvpt[PGNUM(addr)] & PTE_P)) {
			int r = sys_page_alloc(id, (void *)addr, PTE_U | PTE_P | PTE_W);
			if(r < 0)
				panic("sys_page_alloc failed: %e\n", r);
		}

	void *result = (void *)heap_top;
	heap_top += size;
	return result;
}

// a wrapper of malloc
// if malloc fails it will panic
void *Malloc(unsigned int size) {
	void *p;
	if(!(p = malloc(size)))
			panic("Malloc: malloc failed\n");
	return p;
}

void free(void *p) {
	// do nothing
}

char *substr(const char *str, int len) {
	if(len < 0)
		panic("substr: invalid length %d\n", len);

	char *ret = (char *)Malloc((len + 1) * sizeof(char));

	memcpy(ret, str, len * sizeof(char));
	ret[len] = '\0';
	return ret;
}

char *strcopy(const char *str) {
	int len = strlen(str);
	char *s = Malloc((len + 1) * sizeof(char));
	strcpy(s, str);
	return s;
}

char *addstr(const char *s1, const char *s2) {
	int l1 = strlen(s1);
	int l2 = strlen(s2);
	char *str;
	str = (char *)Malloc(l1 + l2 + 1);
	strcpy(str, s1);
	strcpy(str + l1, s2);
	return str;
}

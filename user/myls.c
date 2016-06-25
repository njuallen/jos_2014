#include <inc/lib.h>

int flag[256];

// -a output all files(hidden files included)
// -l controls whether gives full imformation about files
// -t sort by modification time, newest first
// -h print human readable sizes
	void
usage(void)
{
	printf("usage: ls [-alth] [file...]\n");
	exit();
}

enum list_type {
	dummy = 0,
	regular = 1,
	directory = 2
};

#define MAX_NAME_LEN 128
// element in a list
struct List {
	char name[MAX_NAME_LEN + 1];
	int type;
	off_t size;
	struct Rtc create, access, modify;
	struct List *prev, *next;
	struct List *sub_directory;
};


struct List *lsdir(const char *path)
{
	int fd, n;
	struct File f;

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %e", path, fd);
	
	// create a dummy element
	struct List *head = (struct List *)malloc(1 * sizeof(struct List));
	struct List *tail = head;
	tail->type = dummy;
	tail->prev = tail->next = tail->sub_directory = NULL;

	while ((n = readn(fd, &f, sizeof f)) == sizeof f)
		// if it's not null, than it must be a valid entry
		if (f.f_name[0]) {
			struct List *curr = (struct List *)malloc(1 * sizeof(struct List));
			// not necessarily null terminated after strncpy
			// but I do not care
			strncpy(curr->name, f.f_name, MAX_NAME_LEN);
			curr->type = (f.f_type == FTYPE_DIR) ? directory : regular; 
			curr->size = f.f_size;
			curr->create = f.f_create;
			curr->access = f.f_access;
			curr->modify = f.f_modify;
			curr->prev = curr->next = curr->sub_directory = NULL;

			// add it to the list
			tail->next = curr;
			curr->prev = tail;
			tail = curr;
		}

	tail->next = head;
	head->prev = tail;

	if (n > 0)
		panic("short read in directory %s", path);
	if (n < 0)
		panic("error reading directory %s: %e", path, n);

	return head;
}

struct List *ls(const char *path)
{
	int r;
	struct Stat st;

	if ((r = stat(path, &st)) < 0)
		panic("stat %s: %e", path, r);

	struct List *ret = (struct List *)malloc(1 * sizeof(struct List));
	strncpy(ret->name, path, MAX_NAME_LEN);
	ret->type = st.st_isdir ? directory : regular; 
	ret->size = st.st_size;
	ret->create = st.st_create;
	ret->access = st.st_access;
	ret->modify = st.st_modify;
	ret->prev = ret->next = ret->sub_directory = NULL;

	// if this is a directory, we also need to list things under it
	if (st.st_isdir)
		ret->sub_directory = lsdir(path);
	return ret;
}

struct List *all_files = NULL;

void build_file_list(int argc, char **argv) {

	// create a dummy element
	all_files = (struct List *)malloc(1 * sizeof(struct List));
	struct List *tail = all_files;
	tail->type = dummy;
	tail->prev = tail->next = tail->sub_directory = NULL;

	struct List *curr = NULL;

	// list current directory
	if (argc == 1) {
		curr = ls("/");
		tail->next = curr;
		curr->prev = tail;
		tail = curr;
	}
	else {
		int i;
		for (i = 1; i < argc; i++) {
			curr = ls(argv[i]);
			tail->next = curr;
			curr->prev = tail;
			tail = curr;
		}
	}
	tail->next = all_files;
	all_files->prev = tail;
}

bool newer(struct Rtc *a, struct Rtc *b) {
	unsigned int second, minute, hour, day, month, year;
	if(a->year > b->year)
		return true;
	if(a->month > b->month)
		return true;
	if(a->day > b->day)
		return true;
	if(a->hour > b->hour)
		return true;
	if(a->minute > b->minute)
		return true;
	if(a->second > b->second)
		return true;

	return false;
}

void bubble_sort(struct List *head) {

	struct List *curr = head->next;
	// first, we need to know the size of list
	int n = 0;
	while(curr != head) {
		n++;
		curr = curr->next;
	}
	// put all the elements into an array
	// so that we could use bubble sort easily
	struct List *v = (struct List *)malloc(n * sizeof(struct List));

	curr = head->next;
	int count = 0;
	while(curr != head) {
		memcpy(&v[count++], curr, sizeof(struct List));
		curr = curr->next;
	}

	// actually uses bubble sort
	int i, j;
	for(i = 0; i < n; i++) 
	   for(j = 0; j < i; j++)	   
		   if(newer(&v[i].modify, &v[j].modify)) {
			   struct List tmp;
			   memcpy(&tmp, &v[i], sizeof(struct List));
			   memcpy(&v[i], &v[j], sizeof(struct List));
			   memcpy(&v[j], &tmp, sizeof(struct List));
		   }

	// rebuild the list
	for(i = 0; i < (n - 1); i++) {
		v[i].next = &v[i + 1];
		v[i + 1].prev = &v[i];
	}
	v[n - 1].next = head;
	v[0].prev = head;
	head->prev = &v[n - 1];
	head->next = &v[0];
}

void filter(void) {
	struct List *curr = all_files->next;

	// filter out hidden files
	if(!flag['a']) {
		while(curr != all_files) {

			// remove hidden files from the list
			if(curr->name[0] == '.') {
				curr->next->prev = curr->prev;
				curr->prev->next = curr->next;
			}

			// check files under the directory
			if(curr->type == directory && curr->sub_directory) {
				struct List *p = curr->sub_directory->next;
				while(p != curr->sub_directory) {
					if(p->name[0] == '.') {
						p->next->prev = p->prev;
						p->prev->next = p->next;
					}
					p = p->next;
				}
			}
			curr = curr->next;
		}
	}

	// we need to sort it according to last modified time
	if(flag['t']) {
		// sort the files and sub-directories under the directory
		bubble_sort(all_files);

		// sort all files under the sub-directories
		curr = all_files->next;
		while(curr != all_files) {
			if(curr->type == directory && curr->sub_directory)
				bubble_sort(curr->sub_directory);
			curr = curr->next;
		}
	}
}

struct Size_class {
	off_t size;
	char *name;
};

struct Size_class size_class[] = {
	{1, "B"},
	{1 * 1024, "KB"},
	{1 * 1024 * 1024, "MB"},
	{1 * 1024 * 1024, "GB"}
};

int nr_size_class = sizeof(size_class) / sizeof(struct Size_class);

void print_element(struct List *p) {
	if(flag['l']) {
		// output the size of the file
		if(flag['h']) {
			// determine size class
			int i;
			for(i = 0; i < nr_size_class; i++)
				if(p->size >= size_class[i].size && p->size < size_class[i + 1].size)
					break;	   
			printf("%11d%s ", p->size / size_class[i].size, size_class[i].name);
		}
		else
			printf("%11d ", p->size);
		// directory or regular file ?
		printf("%c ", (p->type == directory) ? 'd' : '-');

		// output modification time
		printf("%s ", asctime(&p->modify));
	}
	// output file name
	printf("%s", p->name);
}

void print(void) {
	struct List *curr = all_files->next;

	while(curr != all_files) {
		// check files under the directory
		if(curr->type == directory && curr->sub_directory) {
			print_element(curr);
			printf(":\n", curr->name);
			struct List *p = curr->sub_directory->next;

			while(p != curr->sub_directory) {
				// indente files in sub directories with a tab
				printf("    ");
				print_element(p);
				printf("\n");
				p = p->next;
			}
		}
		else {
			print_element(curr);
			printf("\n");
		}
		curr = curr->next;
	}
}

	void
umain(int argc, char **argv)
{
	int i;
	struct Argstate args;

	argstart(&argc, argv, &args);
	while ((i = argnext(&args)) >= 0)
		switch (i) {
			case 'a':
			case 'l':
			case 't':
			case 'h':
				flag[i]++;
				break;
			default:
				usage();
		}

	build_file_list(argc, argv);
	filter();
	print();
}

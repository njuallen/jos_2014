#include <inc/lib.h>

#define debug 0


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

	void
usage(void)
{
	printf("usage: stat [file...]\n");
	exit();
}

// output all the imformation about the file
void cmd_stat(const char *path)
{
	int r;
	struct Stat st;

	if ((r = stat(path, &st)) < 0)
		panic("stat %s: %e", path, r);

	// output path
	printf("%s:\n", path);

	// directory or regular file ?
	printf("type:%s\n", (st.st_isdir) ? "directory" : "regular file");

	// output the size of the file
	// determine size class
	if(st.st_size > 0) {
		int i;
		for(i = 0; i < nr_size_class; i++)
			if(st.st_size >= size_class[i].size && st.st_size < size_class[i + 1].size)
				break;	   
		printf("size: %d %d%s\n", st.st_size, st.st_size / size_class[i].size, size_class[i].name);
	}
	else
		printf("size: %d\n", st.st_size);

	// output modification time
	printf("create: %s\n", asctime(&st.st_create));
	printf("access: %s\n", asctime(&st.st_access));
	printf("modify: %s\n", asctime(&st.st_modify));
}
	void
umain(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++)
		cmd_stat(argv[i]);
}

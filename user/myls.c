#include <inc/lib.h>

int flag[256];

void lsdir(const char*, const char*);
void ls1(const char*, bool, off_t, const char*, 
		struct Rtc create, struct Rtc access, struct Rtc modify);

	void
ls(const char *path, const char *prefix)
{
	int r;
	struct Stat st;

	if ((r = stat(path, &st)) < 0)
		panic("stat %s: %e", path, r);
	if (st.st_isdir && !flag['d'])
		lsdir(path, prefix);
	else
		ls1(0, st.st_isdir, st.st_size, path, 
				st.st_create, st.st_access, st.st_modify);
}

	void
lsdir(const char *path, const char *prefix)
{
	int fd, n;
	struct File f;

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %e", path, fd);
	while ((n = readn(fd, &f, sizeof f)) == sizeof f)
		// if it's not null, than it must be a valid entry
		if (f.f_name[0])
			ls1(prefix, f.f_type==FTYPE_DIR, f.f_size, f.f_name,
					f.f_create, f.f_access, f.f_modify);
	if (n > 0)
		panic("short read in directory %s", path);
	if (n < 0)
		panic("error reading directory %s: %e", path, n);
}

	void
ls1(const char *prefix, bool isdir, off_t size, const char *name,
		struct Rtc create, struct Rtc access, struct Rtc modify)
{
	const char *sep;

	if(flag['l']) {
		printf("%11d %c\n", size, isdir ? 'd' : '-');
		printf("create time: %s\n", asctime(&create));
		printf("access time: %s\n", asctime(&access));
		printf("modify time: %s\n", asctime(&modify));
	}
	if(prefix) {
		if (prefix[0] && prefix[strlen(prefix)-1] != '/')
			sep = "/";
		else
			sep = "";
		printf("%s%s", prefix, sep);
	}
	printf("%s", name);
	if(flag['F'] && isdir)
		printf("/");
	printf("\n");
}

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

struct List {
	char *file_name;
	struct List *next;
};

void build_file_list(int argc, char **argv, List **files) {
	if (argc == 1)
		ls("/", "");
	else {
		for (i = 1; i < argc; i++)
			ls(argv[i], argv[i]);
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

	List *all_files = NULL;
	build_file_list(argc, argv, &all_files);
	filter(all_files);
	print(all_files);
}

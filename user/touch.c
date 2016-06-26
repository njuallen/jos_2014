#include <inc/lib.h>

#define debug 0

int flag[256];

void
touch(const char *path)
{
	int fd, n;
	char buf[2];
	struct File f;

	if ((fd = open(path, O_RDWR)) < 0)
		goto file_not_exist;

	// in order to modify the time tag
	// we need to directly modify the File structure in the directory
	// get the directory where path resides
	char *sep = strrchr(path, '/');
	char *dir, *name;
	// no '/' found, then it's father dir is pwd
	if(!sep) {
		dir = strcopy(pwd());
		name = strcopy(path);
	}
	else {
		dir = substr(path, sep - path);
		name = strcopy(sep + 1);
	}
	if(debug)
		printf("path: %s\ndir: %s\nname: %s\n", path, dir, name);

	if ((fd = open(dir, O_RDWR)) < 0)
		panic("open %s: %e", dir, fd);
	// the offset of the current entry
	off_t offset = 0;
	bool found = false;
	while ((n = readn(fd, &f, sizeof f)) == sizeof f) {
		// if it's not null, than it must be a valid entry
		if (f.f_name[0] && !strcmp(f.f_name, name)) {
			struct Rtc rtc;
			sys_read_rtc(&rtc);
			f.f_access = f.f_modify = rtc;
			// write back the changes
			seek(fd, offset);
			if((n = write(fd, &f, sizeof f) != sizeof f))
				panic("touch: write directory %s's entry failed\n", dir);
			found = true;
			break;
		}
		offset += sizeof f;
	}
	if(!found)
		panic("can not found '%s' in directory '%s'\n", name, dir);
	return;

file_not_exist:
	// create new file
	if(!flag['c'] && (fd = open(path, O_RDWR | O_CREAT)) < 0)
		panic("can not create file:%s\n", path);
}

// -c do not create any file
void
usage(void)
{
	printf("usage: ls [-c] [file...]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int i;
	struct Argstate args;

	argstart(&argc, argv, &args);
	while ((i = argnext(&args)) >= 0)
		switch (i) {
			case 'c':
				flag[i]++;
				break;
			default:
				usage();
		}

	if (argc == 1)
		exit();
	else {
		for (i = 1; i < argc; i++)
			touch(argv[i]);
	}
}

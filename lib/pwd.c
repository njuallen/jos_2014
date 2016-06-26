#include <inc/lib.h>

#define debug		0

// this functions are not reentrent
// and they are not thread-safe

char working_directory[MAXPATHLEN] = "/";

// reserve 2 more character in case the 
// newpath has not been terminated with a '/' and a null
static char newpath[MAXPATHLEN + 2];

char *pwd(void) {
	return &working_directory[0];
}


char *get_abs_path(const char *path) {
	if(debug) {
		printf("path:%s\n", path);
		printf("pwd:%s\n", working_directory);
	}
	// absolute path
	if(path[0] == '/')
		strncpy(newpath, path, MAXPATHLEN);
	else {
		// relative path
		// final path too long
		if(strlen(path) + strlen(working_directory) >= MAXPATHLEN)
			return NULL;
		strncpy(newpath, working_directory, MAXPATHLEN);
		strcat(newpath, path);
	}
	return &newpath[0];
}

// change working directory
int chdir(const char *path) {
	int r;
	struct Stat st;

	char *newpath = get_abs_path(path);
	if(!newpath)
		return -E_BAD_PATH;
	// this path does not exist
	if ((r = stat(newpath, &st)) < 0)
		return r;
	// not a directory
	if (!st.st_isdir)
		return -E_FILE_EXISTS;

	// our working_directory needs to be terminated with '/' and a null
	int len = strlen(newpath);
	if(newpath[len - 1] != '/') {
		newpath[len] = '/';
		newpath[len + 1] = '\0';
	}
	// after adding the '/', the path becomes too long
	if(strlen(newpath) >= MAXPATHLEN)
		return -E_BAD_PATH;

	// ok
	// update working_directory
	strncpy(working_directory, newpath, MAXPATHLEN);
	if(debug)
		printf("working_directory:%s\n", working_directory);
	return 0;
}

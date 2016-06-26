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

struct List_elem {
	char *path;
	struct List_elem *prev, *next;
};


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

	// now we need to deal with . and ..
	// well, I need C++ string to deal with these dirty things!

	// store each level of path name in a list
	// so that we can deal with '.' and '..' much more easily

	// the dummy element
	struct List_elem *head, *tail;
	head = tail = (struct List_elem *)Malloc(sizeof(struct List_elem));
	tail->path = "/";
	tail->prev = tail;
	tail->next = tail;

	// skip the first '/'
	get_token_init(strchr(newpath, '/') + 1, "/");
	char *token;
	while((token = get_token())) {
		if(debug)
			printf("token: %s\n", token);
		if(!strcmp(token, "."))
			free(token);
		else if(!strcmp(token, "..")) {
			// go to the parent directory
			// which means we need to remove the tail of the list
			// '/''s father is itself
			if(tail != head) {
				tail->next->prev = tail->prev;
				tail->prev->next = tail->next;
				struct List_elem *to_be_removed = tail;
				tail = tail->prev;
				free(to_be_removed);
			}
			free(token);
		}
		else {
			struct List_elem *elem = (struct List_elem *)Malloc(sizeof(struct List_elem));
			elem->path = token;
			elem->next = tail->next;
			elem->prev = tail;
			tail->next = elem;
			tail = elem;
		}
	}
	struct List_elem *p = head->next;
	char *ret = strcopy("/");
	bool first = true;
	while(p != head) {
		char *tmp_1;
		if(first) {
			tmp_1 = addstr(ret, "");
			first = false;
		}
		else
			tmp_1 = addstr(ret, "/");
		char *tmp_2 = addstr(tmp_1, p->path);
		free(ret);
		free(tmp_1);
		free(p->path);
		p = p->next;
		free(p->prev);
		ret = tmp_2;
	}
	strcpy(newpath, ret);
	free(ret);
	if(debug) {
		printf("ret:%s\n", ret);
		printf("newpath:%s\n", newpath);
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

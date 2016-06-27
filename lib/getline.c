#include <inc/stdio.h>
#include <inc/error.h>
#include <inc/lib.h>

#define BUFLEN 1024
static char buf[BUFLEN];

struct List_elem {
	char buf[BUFLEN];
	struct List_elem *prev, *next;
};

static struct List_elem *l = NULL;

char *
getline(const char *prompt)
{
	// the list is empty
	// first, we create a dummy element on it
	if(!l) {
		l = (struct List_elem *)Malloc(sizeof(struct List_elem));
		l->buf[0] = '\0';
		l->prev = l->next = l;
	}

	// use p to traverse the list
	// l always points to the newly added element
	struct List_elem *p = l;
	int i, c, echoing;

	// the positione of the cursor
	int cursor = 0;

	if (prompt != NULL) {
		fprintf(1, "%s", prompt);
		cursor += strlen(prompt);
	}

	i = 0;
	echoing = iscons(0);
	while (1) {
		c = getchar();
		// my experiment shows me that hit a arrow key 
		// will generate 2 serial interrupts.
		// And if we print out c, we will get three value 27, 91 and 65-68
		// The first value 27 seems to be special, so we
		// use it to catch arrow keys from serial port
		if(c == 27) {
			if((c = getchar()) == 91) {
				c = getchar();
				if(echoing && (c == 65 || c == 66)) {
					// overwrite contents in current buffer
					memcpy(buf, p->buf, BUFLEN * sizeof(char));


					// clear echoing characters on this line
					while(cursor > 0) {
						cputchar('\b');
						// use a white space to erase out previous output
						cputchar(' ');
						cputchar('\b');
						cursor--;
					}

					// reprint prompt
					if (prompt != NULL) {
						fprintf(1, "%s", prompt);
						cursor += strlen(prompt);
					}

					// output the contents in the buffer
					fprintf(1, "%s", buf);
					cursor += strlen(buf);

					// update i
					i = strlen(buf);
					// update p
					// up
					if(c == 65)
						p = p->prev;
					else
						p = p->next;
					continue;
				}
				// well, if you hit left and right
				// we simulate the default action
				if(echoing && c == 67) {
					fprintf(1, "%s", "[C");
					cursor += strlen("[C");
				}
				if(echoing && c == 68) {
					fprintf(1, "%s", "[D");
					cursor += strlen("[D");
				}
				continue;
			}
		}
		if (c < 0) {
			if (c != -E_EOF)
				cprintf("read error: %e\n", c);
			return NULL;
		} else if ((c == '\b' || c == '\x7f') && i > 0) {
			// back space or delete
			if (echoing) {
				cputchar('\b');
				cursor--;
			}
			i--;
		} else if (c >= ' ' && i < BUFLEN-1) {
			if (echoing) {
				cputchar(c);
				cursor++;
			}
			buf[i++] = c;
		} else if (c == '\n' || c == '\r') {
			if (echoing)
				cputchar('\n');
			buf[i] = 0;
			// record current line
			struct List_elem *curr = (struct List_elem *)Malloc(sizeof(struct List_elem));
			memcpy(curr, buf, BUFLEN * sizeof(char));
			curr->next = l->next;
			curr->prev = l;
			l->next = curr;
			curr->next->prev = curr;
			l = curr;
			return buf;
		}
	}
}

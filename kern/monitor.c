// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Backtracing the stack", mon_backtrace },
	{ "showmapping", "Show the memory mapping of a virtual memory address", mon_showmapping },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

static int get_hex(const char *str, uint32_t *dst) {
	while(*str && *str != 'x')
		str++;
	str++;
	if(*str == 0)
		return -1;
	*dst = 0;
	while(*str) {
		if (*str >= '0' && *str <= '9')
			*dst = *dst * 16 + *str - '0';
		else if(*str >= 'a' && *str <= 'f')
			*dst = *dst * 16 + *str - 'a' + 10;
		else
			return -1;
		*str++;
	}
	return 0;
}

int
mon_showmapping(int argc, char **argv, struct Trapframe *tf)
{
	if (argc < 2)
		cprintf("showmapping: expect virtual memory addresses in hexadicimal representation\n");
	else {
		int i;
		cprintf("%3s%-10s%-10s%-10s%-10s\n", " ", "va", "pde", "pte", "perm");
		for(i = 1; i < argc; i++) {
			uintptr_t va;
			int ret = get_hex(argv[i], &va);
			if(ret < 0) {
				cprintf("%s is not a legal hex number\n", argv[i]);
				return 0;
			}
			extern pde_t *kern_pgdir;
			pde_t pde = kern_pgdir[PDX(va)]; 
			if (pde & PTE_P) {
				pde &= 0xfffff000;
				pde_t pde_tmp = pde + KERNBASE;
				pte_t pte = *((pte_t *)pde_tmp + PTX(va));
				if(pte & PTE_P) {
					uint32_t perm = pte & 0xfff;
					pte &= 0xfffff000;
					cprintf("%3s%8x %8x %8x %8x\n", " ", va, pde, pte, perm);
				}
				else {
					cprintf("page does not exist");
				}
			}
			else {
				cprintf("%3s%-10x page table does not exist\n", " ", va);
			}
		}
	}
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// output format
	// Stack backtrace:
	// ebp f0109e58  eip f0100a62  args 00000001 f0109e80 f0109e98 f0100ed2 00000031
	// kern/monitor.c:143: monitor+106
	cprintf("Stack backtrace\n");
	unsigned int ebp = read_ebp();
	while(ebp != 0) {
		cprintf("ebp %08x ", ebp);
		cprintf("eip %08x ", *(unsigned int *)(ebp + 4));
		unsigned int eip = *(unsigned int *)(ebp + 4);
		cprintf("args ");
		int i;
		for(i = 0; i < 5; i++)
			cprintf("%08x ", *(unsigned int *)(ebp + 8 + 4 * i));
		cprintf("\n");
		struct Eipdebuginfo info;
		if(debuginfo_eip(eip, &info) == 0) {
			cprintf("%s:%d: %.*s+%d\n", 
					info.eip_file, info.eip_line, 
					info.eip_fn_namelen, info.eip_fn_name
					, info.eip_line);
		}
		cprintf("\n");

		// update ebp
		ebp = *(unsigned int *)ebp;
	}
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

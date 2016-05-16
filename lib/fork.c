// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

	void
print_trapframe(struct UTrapframe *utf);
static inline pte_t get_pte(uint32_t pn);
//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
	//
	// do we need to check FEC_PR and FEC_U ?????

	if(!((err & FEC_WR) && (get_pte(PGNUM(addr)) & PTE_COW)))
		panic("pgfault: unexpected type of page fault\n");
	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	void *fault_page_addr = ROUNDDOWN(addr, PGSIZE);
	envid_t envid = sys_getenvid();
	if((r = sys_page_alloc(envid, PFTEMP, 
					PTE_P | PTE_U | PTE_W)) < 0)
		panic("sys_page_alloc: %e\n", r);
	// copy the content of the old COW page into the new page
	memcpy((void *)PFTEMP, fault_page_addr, PGSIZE);

	// remap the fault page with PTE_W
	if((r = sys_page_map(envid, (void *)(PFTEMP), 
					envid, fault_page_addr, PTE_U | PTE_P | PTE_W)) < 0)
		panic("sys_page_map: %e\n", r);

	// unmap the page at PFTEMP
	if((r = sys_page_unmap(envid, (void *)PFTEMP)) < 0)
		panic("sys_page_unmap: %e\n", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	if(get_pte(pn) & PTE_W || get_pte(pn) & PTE_COW) {
		// if the page is writable or marked as copy on write
		// for child
		if((r = sys_page_map(sys_getenvid(), (void *)(pn * PGSIZE), 
						envid, (void *)(pn * PGSIZE), PTE_U | PTE_P | PTE_COW)) < 0)
			panic("sys_page_map: %e\n", r);

		// you can not do this
		// or you will never be able to execute the sys_page_map
		// following it
		/*
		// for parent
		// temporarily unmap the page
		if((r = sys_page_unmap(sys_getenvid(), (void *)(pn * PGSIZE))) < 0)
			panic("sys_page_unmap: %e\n", r);
			*/

		// remap it with different perm
		if((r = sys_page_map(envid, (void *)(pn * PGSIZE), 
						sys_getenvid(), (void *)(pn * PGSIZE), PTE_U | PTE_P | PTE_COW)) < 0)
			panic("sys_page_map: %e\n", r);
	}
	else {
		// if the page is read-only, directly map it 
		// in child's address space
		// is the perm right?
		if((r = sys_page_map(sys_getenvid(), (void *)(pn * PGSIZE), 
						envid, (void *)(pn * PGSIZE), PTE_U | PTE_P)) < 0)
			panic("sys_page_map: %e\n", r);
	}
	return 0;
}


//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// set up the page fault handler
	set_pgfault_handler(pgfault);

	envid_t envid;
	int r;
	// Allocate a new child environment.
	envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);

	if (envid == 0) {
		// We're the child.
		// The copied value of the global variable 'thisenv'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	else {
		// we are the parent
		// leave the exception stack untouched !!!
		int i;
		for(i = 0; i < PGNUM(USTACKTOP); i++)
			// we call duppage only for pages that do exist
			if((get_pte(i) & PTE_P) && (get_pte(i) & PTE_U))
				duppage(envid, i);

		// allocate the page fault exception stack for child
		if((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), 
						PTE_P | PTE_U | PTE_W)) < 0)
			panic("sys_page_alloc: %e\n", r);

		// register the handler entry for child
		// note that, since parent and child have the same address space
		// their _pgfault_handler have the same value
		// so we do not need to call set_pgfault_handler for child again
		// we only need to register the upcall entry for child
		// Assembly language pgfault entrypoint defined in lib/pfentry.S.
		extern void _pgfault_upcall(void);
		if((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0)
			panic("sys_env_set_pgfault: %e\n", r);

		// Start the child environment running
		if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
			panic("game: %d\n", ENV_RUNNABLE);
			panic("sys_env_set_status: %e", r);
		}
		return envid;
	}
}

// the so called continuous 4MB ptes are not continuous at all,
// 
// since some of the page tables are not allocated
// in order to make sure our read on upvt does not triger a page fault
// we need to consult the uvpd in advance.
// In order to make that easy
// I write a wrapper function that does the checking for you
// 
// get the according pte of the pnth page
// if the page table is not currently allocated
// it will noinline t result in a page fault just like
// what you would get with upvt[pn].
// instead, you will get zero
static inline pte_t get_pte(uint32_t pn) {
	// does the corresponding page table exist ?
	// in fact the PTE_U check is not necessary 
	// since all entries of page table directory
	// is marked with PTE_U(if corresponding page table exists)
	if(uvpd[(pn >> 10) & 0x3ff] & PTE_P)
		return uvpt[pn];
	return 0;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}

// I add all these for debugging
	void
print_regs(struct PushRegs *regs);

	void
print_trapframe(struct UTrapframe *utf)
{
	cprintf("********** UTRAP frame ************\n");
	print_regs(&utf->utf_regs);
	cprintf("  va   0x%08x", utf->utf_fault_va);
	cprintf("  err  0x%08x", utf->utf_err);
		cprintf(" [%s, %s, %s]\n",
				utf->utf_err & 4 ? "user" : "kernel",
				utf->utf_err & 2 ? "write" : "read",
				utf->utf_err & 1 ? "protection" : "not-present");
	cprintf("  eip  0x%08x\n", utf->utf_eip);
	cprintf("  flag 0x%08x\n", utf->utf_eflags);
	cprintf("  esp  0x%08x\n", utf->utf_esp);
}

	void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

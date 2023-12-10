#define _GNU_SOURCE

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#define SYSCALL	0x050f
#define SYSCALL_MASK 0x000000000000ffff
#define SIZEOF_SYSCALL 2


#define LIBC_PATH "/lib/libc-"

/* Basic object for keeping state. */
struct ptrace_do{
	int pid;
	unsigned long sig_ignore;
	struct user_regs_struct saved_regs;

	struct parse_maps *map_head;	
	unsigned long syscall_address;
};


/* ptrace_do_init() hooks the target and prepares it to run our commands. */
struct ptrace_do *ptrace_do_init(int pid);
void ptrace_do_get_regs(struct ptrace_do *target);
void ptrace_do_set_regs(struct ptrace_do *target);

/* ptrace_do_syscall() will execute the given syscall inside the remote process. */
unsigned long ptrace_do_syscall(struct ptrace_do *target, unsigned long rax, \
		unsigned long rdi, unsigned long rsi, unsigned long rdx, unsigned long r10, unsigned long r8, unsigned long r9);

/* ptrace_do_cleanup() will detatch and do it's best to clean up the data structures. */
void ptrace_do_cleanup(struct ptrace_do *target);


/*
 * parse_maps 
 *
 * Added functionality to parse the /proc/PID/maps file of the target process into an internal data structure.
 * This allows us to walk areas of executable memory looking for a SYSCALL instruction to borrow.
 *
 * We leave the maps data structure accessible in the ptrace_do object, in case it comes in handy for the user.
 *
 * "man proc" for more information on the shape of the maps file.
 */

#define MAPS_READ		0x10000
#define MAPS_WRITE		0x01000
#define MAPS_EXECUTE	0x00100
#define MAPS_PRIVATE	0x00010
#define MAPS_SHARED		0x00001

/* Basic parse_maps object representing the different fields represented in the file. */
struct parse_maps {

	unsigned long start_address;
	unsigned long end_address;
	unsigned int perms;
	unsigned long offset;
	unsigned int dev_major;
	unsigned int dev_minor;
	unsigned long inode;
	char pathname[PATH_MAX];

	struct parse_maps *next;
	struct parse_maps *previous;
};

/* get_proc_pid_maps() processes the maps file and returns the created object.*/
struct parse_maps *get_proc_pid_maps(pid_t target);

/* free_parse_maps_list() destroys a parse_maps object chain. */
void free_parse_maps_list(struct parse_maps *head);

/* Mostly for debugging, but in case it comes in handy, this function prints the parse_maps object members. */
void dump_parse_maps_list(struct parse_maps *head);
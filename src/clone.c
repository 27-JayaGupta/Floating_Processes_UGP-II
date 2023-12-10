#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdlib.h> 
#include <linux/sched.h>
#include <sys/syscall.h>    /* Definition of SYS_* constants */

#include "libptrace_do.h"

#define CLONE_ATTACH 0x400000000ULL

#define STACK_SIZE (1024*1024)

int
childFunc1(void *arg)
{   
    while(1){
        printf("Speaking, pid: %d\n", getpid());
        sleep(5);
    }
}

int main(){
	int pid;

	struct ptrace_do *target;
    void* child_stack1 = malloc(STACK_SIZE);

    pid = clone(childFunc1,
                (void*)((unsigned long)child_stack1 + STACK_SIZE),
                SIGCHLD | CLONE_VM,
                NULL
                );

	// Hook the target.
    printf("Before ptrace_init\n");
	target = ptrace_do_init(pid);
    ptrace_do_get_regs(target);
    printf("Before ptrace_syscall. NR_CLONE: %d\n", SYS_clone);
	// Now that it's in the remote memory we can remotely call the write() syscall, and point it at the remote address.

	pid = ptrace_do_syscall(target, SYS_clone, SIGCHLD | CLONE_VFORK | CLONE_ATTACH, 0, 0, 0, 0, 0);

    printf("New PID spawned: %d\n", pid);
    printf("Before ptrace_cleanup\n");
	// Unhook and clean up.
	ptrace_do_cleanup(target);

    wait(NULL);

	return(0);
}

/*
    Spawn 2 process A and B, where B is child of A. In this case when detachment takes place, the parent is already waiting for the child
*/

#define _GNU_SOURCE
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define STACK_SIZE (1024*1024)
#define CLONE_ATTACH 0x400000000ULL

int
childFunc1(void *arg)
{   
    while(1){
        printf("Speaking: PID: %d\n", getpid());
    }
}

int main() {
    int fd, user_ns, old_net_ns, new_net_ns;

    void* child_stack1 = malloc(STACK_SIZE);

    assert(child_stack1 != NULL);
    pid_t pid1;

    /* Spawn a new user namespace */
    printf("[*] Spawn new child #1\n");
    pid1 = clone(childFunc1,
                child_stack1 + STACK_SIZE,
                SIGCHLD | CLONE_VM | CLONE_ATTACH,  NULL);
    assert(pid1 != -1);

    printf("[Parent] Child1 spawned with pid: %d\n", pid1);

    printf("Parent going to WAIT for child\n");
    wait(NULL);

    printf("Parent Returned from WAIT and Now Exiting\n");
  

    return 0;
}

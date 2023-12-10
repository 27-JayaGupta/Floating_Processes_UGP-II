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

int
childFunc1(void *arg)
{   
    printf("%d Child[lvl: 1] Speaking!!\n", getpid());
}

int main() {
    int fd, user_ns, old_net_ns, new_net_ns;
    signal(SIGCHLD, SIG_IGN);
    void* child_stack1 = malloc(STACK_SIZE);
    void* child_stack2 = malloc(STACK_SIZE);

    assert(child_stack1 != NULL);
    assert(child_stack2 != NULL);
    pid_t pid1, pid2;

    /* Spawn a new user namespace */
    int child_num = 1;
    printf("[*] Spawn new pid namespace #1\n");
    pid1 = clone(childFunc1,
                child_stack1 + STACK_SIZE,
                CLONE_NEWPID | SIGCHLD, &child_num);
    assert(pid1 != -1);
    child_num++;
    printf("[Parent] Child1 spawned with pid: %d\n", pid1);

    // printf("[*] Spawn new pid namespace #2\n");
    // pid2 = clone(childFunc1,
    //             child_stack2 + STACK_SIZE,
    //             SIGCHLD | CLONE_NEWPID, &child_num);
    // assert(pid2 != -1);
    // printf("[Parent] Child2 spawned with pid: %d\n", pid2);

    // wait(NULL);
    printf("######################################################## Parent Exited #####################3\n");
    return 0;
}   
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define STACK_SIZE 65536

#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434   /* System call # on most architectures */
#endif

static int
pidfd_open(pid_t pid, unsigned int flags)
{
    return syscall(__NR_pidfd_open, pid, flags);
}

int run1(void* args) {
    sleep(2);
    printf("Child One in Action\n");
    while(1);
}

int run2(void* args) {
    sleep(2);
    printf("Child Two in Action\n");

    int sibling_pid = *(int*)(args);

    char filename[30];
    sprintf(filename, "/proc/%d/ns/user", sibling_pid);
    int fd = open(filename, O_RDONLY);

    if(fd == -1) {
        printf("C2: Failed to open pid file of C1");
        exit(-1);
    }

    if(setns(fd, CLONE_NEWUSER) < 0) {
        printf("Child: setns failed: %d\n", errno);
        exit(-1);
    }

    printf("Successfully changed user namespace of C2 to C1\n");
}

int main() {

    void* stack1 = malloc(STACK_SIZE);
    void* stack2 = malloc(STACK_SIZE);

    if(stack1 ==NULL || stack2 ==NULL){
        printf("malloc error\n");
        exit(1);
    }

    // Children in new user namespace
    int pid1 = clone(run1, stack1 + STACK_SIZE, CLONE_NEWUSER|SIGCHLD, NULL);

    if(pid1 == -1){
        printf("clone erro\n");
        exit(1);
    }

    // Second children in another new user namespace
    int pid2 = clone(run2, stack2 + STACK_SIZE, CLONE_NEWUSER|SIGCHLD, (void*)(&pid1));

    if(pid2 == -1){
        perror("clone");
        exit(1);
    }

    char filename[30];
    sprintf(filename, "/proc/%d/ns/user", pid1);

    int fd = open(filename, O_RDONLY);

    if(fd == -1) {
        printf("P: Failed to open pid file of C1");
        exit(-1);
    }

    printf("Parent successfuylly opened the pid file for C1\n");

    if(setns(fd, CLONE_NEWUSER) < 0) {
        printf("Parent: setns failed: %d\n", errno);
        exit(-1);
    }

    printf("P: Successfully changed user namespace of P to C1\n");

    wait(NULL);
}
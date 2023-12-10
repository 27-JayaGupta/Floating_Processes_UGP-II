/* Proof of concept of the inability to attach to an existing NS in
 * the same user namespace. */

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
childFunc(void *arg)
{   
    printf("%d Child Speaking!!\n", getpid());
    while(1);
    return 0;
}

int main() {
    int fd, user_ns, old_net_ns, new_net_ns;
    char path[PATH_MAX];
    char map[128];
    void* child_stack = malloc(STACK_SIZE);
    assert(child_stack != NULL);
    pid_t pid1, pid2;

    /* Spawn a new user namespace */
    printf("[*] Spawn new pid namespace #1\n");

    assert(unshare(CLONE_NEWPID) == 0);

    pid1 = clone(childFunc,
                child_stack + STACK_SIZE,
                SIGCHLD, NULL);
    assert(pid1 != -1);
    printf("[Parent] Child1 spawned with pid: %d\n", pid1);

    printf("[*] Spawn new pid namespace #2\n");
    assert(unshare(CLONE_NEWPID) == 0);
    pid2 = clone(childFunc,
                child_stack + STACK_SIZE,
                SIGCHLD, NULL);
    assert(pid2 != -1);
    printf("[Parent] Child2 spawned with pid: %d\n", pid2);

    // This should work.
    printf("[*] Setting the pid namespace to that of child#1\n");
    snprintf(path, sizeof(path), "/proc/%d/ns/pid", pid1);
    fd = open(path, O_RDWR);
    assert(fd >= 0);
    assert(setns(fd, CLONE_NEWPID) == 0);

    // Check whether this works or not
    // Beacause as in the setns syscall implementation, it does a check whether the pid_ns_for_children is same as the parent pid namespace.
    // If not, it returns an error and does not set the pid_ns_for_children again.

    // This should fail.
    printf("[*] Setting the pid namespace to that of child#2  without cloning\n");
    snprintf(path, sizeof(path), "/proc/%d/ns/pid", pid2);
    fd = open(path, O_RDWR);
    assert(fd >= 0);
    assert(setns(fd, CLONE_NEWPID) == 0);

    wait(NULL);

    return 0;
}
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
    char buf;
    int *p = arg;
    close(p[1]);
    assert(read(p[0], &buf, 1) == 0);
    return 0;
}

int main() {
    int fd, user_ns, old_net_ns, new_net_ns;
    char path[PATH_MAX];
    char map[128];
    void* child_stack = malloc(STACK_SIZE);
    assert(child_stack != NULL);
    int p[2];
    pid_t pid;

    /* Spawn a new user namespace */
    printf("[*] Spawn new user namespace\n");
    assert(pipe(p) == 0);

    pid = clone(childFunc,
                child_stack + STACK_SIZE,
                CLONE_NEWUSER | CLONE_NEWNET | SIGCHLD, p);
    
    assert(pid != -1);
    close(p[0]);

    /* Setup UID/GID map */
    printf("[*] Setup UID/GID map\n");
    snprintf(path, sizeof(path), "/proc/%d/uid_map", pid);
    fd = open(path, O_RDWR);
    assert(fd >= 0);
    snprintf(map, sizeof(map), "0 %d 1", getuid());
    assert(write(fd, map, strlen(map)) == (ssize_t)strlen(map));
    close(fd);
    snprintf(path, sizeof(path), "/proc/%d/setgroups", pid);
    fd = open(path, O_RDWR);
    assert(fd >= 0);
    assert(write(fd, "deny", 4) == 4);
    close(fd);
    snprintf(path, sizeof(path), "/proc/%d/gid_map", pid);
    fd = open(path, O_RDWR);
    assert(fd >= 0);
    snprintf(map, sizeof(map), "0 %d 1", getgid());
    assert(write(fd, map, strlen(map)) == (ssize_t)strlen(map));
    close(fd);

    /* Keep FD to user namespace */
    printf("[*] Keep a reference to new user namespace\n");
    snprintf(path, sizeof(path), "/proc/%d/ns/user", pid);
    user_ns = open(path, O_RDONLY);
    assert(user_ns >= 0);

    /* Signal children this is over */
    printf("[*] Tell child to die\n");
    close(p[1]);
    assert(waitpid(pid, NULL, 0) == pid);

    /* Go into the new user namespace */
    printf("[*] Enter new user namespace\n");
    assert(setns(user_ns, 0) != -1);

    /* Now, spawn a new net namespace */
    printf("[*] Spawn new net namespace\n");
    assert(pipe(p) == 0);
    pid = clone(childFunc,
                child_stack + STACK_SIZE,
                CLONE_NEWNET | SIGCHLD, p);
    assert(pid != -1);
    close(p[0]);

    /* Keep FD to current and next user namespace */
    printf("[*] Keep FD to current and next net namespace\n");
    snprintf(path, sizeof(path), "/proc/%d/ns/net", pid);
    new_net_ns = open(path, O_RDONLY);
    assert(new_net_ns >= 0);
    snprintf(path, sizeof(path), "/proc/%d/ns/net", getpid());
    old_net_ns = open(path, O_RDONLY);
    assert(old_net_ns >= 0);

    /* Signal children this is over */
    printf("[*] Tell child to die\n");
    close(p[1]);
    assert(waitpid(pid, NULL, 0) == pid);

    /* Go into the new net namespace */
    printf("[*] Enter new net namespace\n");
    assert(setns(new_net_ns, 0) != -1);

    /* Go back into the old net namespace */
    printf("[*] Enter old net namespace\n");
    assert(setns(old_net_ns, 0) != -1);

    return 0;
}
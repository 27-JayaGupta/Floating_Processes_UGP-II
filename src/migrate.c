#define _GNU_SOURCE

#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "migrate.h"
#include "libptrace_do.h"

int __migrate(pid_t source_pid, pid_t target_pid)
{
    int fd = open("/dev/migration", O_RDWR);

    if(fd < 0) {
        printf("Failed in opening chardev\n");
        exit(-1);
    }

    char buf[4096];
    int count = sprintf(buf, "%d %d", source_pid, target_pid);
    if(write(fd, buf, count) < 0){
        printf("Error in writing to the chardev\n");
        close(fd);
        exit(-1);
    }

    // Detachment
    if(ioctl(fd, IOCTL_DETACH, NULL) < 0){
       printf("Detachment failed\n");
       exit(-1);
    }

    printf("Process Detached Successfully\n");

    /*
        * ATTACHMENT
        i) Make a ptrace clone syscall on behalf of target process.
    */

    int pid;
    struct ptrace_do* target;
    target = ptrace_do_init(target_pid);
    ptrace_do_get_regs(target);
    pid = ptrace_do_syscall(target, SYS_clone, SIGCHLD | CLONE_VFORK | CLONE_VM | CLONE_ATTACH, 0, 0, 0, 0, 0);
    ptrace_do_cleanup(target);
   
    printf("Process Attached Successfully: NewPID: %d\n", pid);
    return pid;
}

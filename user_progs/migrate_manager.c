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

int main(int argc, char* argv[]) {

    if(argc != 3) {
        printf("./migrate source_pid target_pid");
        exit(-1);
    }

    pid_t source_pid, target_pid;

    source_pid = atoi(argv[1]);
    target_pid = atoi(argv[2]);

    printf("New Pid after migration: %d\n", __migrate(source_pid, target_pid));

    while(1);

}


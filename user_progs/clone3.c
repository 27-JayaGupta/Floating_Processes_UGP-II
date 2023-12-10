#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef CLONE_PIDFD
#define CLONE_PIDFD 0x00001000
#endif

#ifndef __NR_clone3
#define __NR_clone3 -1
struct clone_args {
	__aligned_u64 flags;
	__aligned_u64 pidfd;
	__aligned_u64 child_tid;
	__aligned_u64 parent_tid;
	__aligned_u64 exit_signal;
	__aligned_u64 stack;
	__aligned_u64 stack_size;
	__aligned_u64 tls;
};
#endif

static pid_t sys_clone3(struct clone_args *args)
{
	return syscall(435, args, sizeof(struct clone_args));
}

static int wait_for_pid(pid_t pid)
{
	int status, ret;

again:
	ret = waitpid(pid, &status, 0);
	if (ret == -1) {
		if (errno == EINTR)
			goto again;

		return -1;
	}

	if (ret != pid)
		goto again;

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		return -1;

	return 0;
}

#define ptr_to_u64(ptr) ((__u64)((uintptr_t)(ptr)))

int main(int argc, char *argv[])
{
	int pidfd = -1;
	pid_t parent_tid = -1, pid = -1;
	struct clone_args args = {
		/* CLONE_PARENT_SETTID */
		.parent_tid = ptr_to_u64(&parent_tid),
		/* CLONE_PIDFD */
		.pidfd = ptr_to_u64(&pidfd),
		.flags = CLONE_PIDFD | CLONE_PARENT_SETTID,
		.exit_signal = SIGCHLD,
	};

	pid = sys_clone3(&args);
	if (pid < 0) {
		fprintf(stderr, "%s - Failed to create new process\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (pid == 0) {
		printf("Child process with pid %d\n", getpid());
		exit(EXIT_SUCCESS);
	}

	printf("Parent process received child's pid %d as return value\n", pid);
	printf("Parent process received child's pidfd %d\n", pidfd);
	printf("Parent process received child's pid %d as return argument\n",
	       *(pid_t *)args.parent_tid);

	if (wait_for_pid(pid)) {
		fprintf(stderr, "Failed to wait on child process\n");
		exit(EXIT_FAILURE);
	}

	if (pid != parent_tid)
		exit(EXIT_FAILURE);

	close(pidfd);

	return 0;
}

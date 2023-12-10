/*
    Spawns 5 processes A,B,C,D,E in 3 different namespaces with B/C being children of A and siblings with each other.

    A is NS1, B/D is NS2, C/E in NS3
        A
       / \
      B   C
      /   \
      D    E.

    We need to change parent of E from C to B.
    Source(E) -> Target(B)
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

int infiniteLoop() {
    while(1) {

    }

    return 0;
}

int
childFunc1(void *arg)
{   
    int child_num = *(int*)(arg);
    pid_t child_pid;

    // Fork a child process
    if ((child_pid = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process
        char *program = "./image";  // Example: replace with the path to the executable you want to run
        char *args[] = {program, "-l", NULL};

        // Execute the new program in the child process
        execvp(program, args);

        // If execvp fails, print an error message and exit
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        printf("%d Child[lvl: 1, num: %d] Speaking. Spawned child process with pid: %d!!\n", getpid(), child_num, child_pid);
        printf("%d Child[lvl: 1, num: %d] Going to sleep\n", getpid(), child_num);
        sleep(30);        
        while(1);
        // wait(NULL);
        printf("%d Child[lvl: 1, num: %d] Exited!!\n", getpid(), child_num);
    }
}

int main() {
    int fd, user_ns, old_net_ns, new_net_ns;

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
                SIGCHLD | CLONE_NEWPID,  &child_num);
    assert(pid1 != -1);
    child_num++;
    printf("[Parent] Child1 spawned with pid: %d\n", pid1);

    printf("[*] Spawn new pid namespace #2\n");
    pid2 = clone(infiniteLoop,
               child_stack2 + STACK_SIZE,
               SIGCHLD | CLONE_NEWPID, &child_num);
    assert(pid2 != -1);
    printf("[Parent] Child2 spawned with pid: %d\n", pid2);
	
    printf("Parent going to SLEEP\n");
    sleep(30);
    printf("Parent back from SLEEP, called WAIT\n");
    
    while(1);
    // wait(NULL);

    printf("######################################################## Parent Returned from WAIT or SLEEP  #####################3\n");
	
    return 0;
}

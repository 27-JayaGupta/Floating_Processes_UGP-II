/*
    Spawns 5 processes A,B,C,D,E,F in 3 different namespaces with B/C being children of A and siblings with each other.

    A is NS1, B/D/F is NS2, C/E in NS3
        A
       / \
      B   C
     / \   \
    D   F   E.

    In this scenario, F and D are taking to each other using a shared memory space.
    F gets migrated to new namespace NS3 with C as its parent.
    After the migration also, communication  between D and F contniues as normal.
    But now F is in a new pid namespace with different parent.
*/
    

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#define SHM_SIZE 1024

#define STACK_SIZE (1024*1024)

int
childFunc2(void *arg)
{   
    while(1){
	    // A pid=3 process should appear eventually
    	printf("Child running: pid: %d\n", getpid());
    	sleep(10);
    }
}

int
childFunc1(void *arg)
{   
    void* child_stack = malloc(STACK_SIZE);
    assert(child_stack != NULL);

    int child_num = *(int*)(arg);
    pid_t child_pid;

    child_pid = clone(childFunc2,
                    child_stack + STACK_SIZE,
                    SIGCHLD, NULL);
        
    assert(child_pid != -1);
   
    printf("%d Child[lvl: 1, num: %d] Speaking. Spawned child process with pid: %d!!\n", getpid(), child_num, child_pid);
    printf("%d Child[lvl: 1, num: %d] Going to sleep\n", getpid(), child_num);
    wait(NULL);
    printf("%d Child[lvl: 1, num: %d] Exited!!\n", getpid(), child_num);
}

int childFunc() {
    int shmid;
    key_t key = ftok("shmfile", 'R');
    char *shmaddr;

    // Create shared memory segment
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    if ((shmaddr = shmat(shmid, NULL, 0)) == (char *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Fork the first child process
    pid_t child1_pid = fork();

    if (child1_pid == 0) {
        // First child process
        while (1) {
            sprintf(shmaddr, "Message from Child 1");
            sleep(2);
        }
    }

    // Fork the second child process
    pid_t child2_pid = fork();

    if (child2_pid == 0) {
        // Second child process
        while (1) {
            printf("Child 2 received: %s\n", shmaddr);
            sleep(2);
        }
    }

    // Parent process
    printf("[%d] Parent process forked 2 child process, PID1: %d, PID2: %d\n", getpid(), child1_pid, child2_pid);

    // Wait for the child processes to complete
    // waitpid(child1_pid, NULL, 0);
    // waitpid(child2_pid, NULL, 0);

    wait(NULL);
    wait(NULL);

    // Detach and remove the shared memory segment
    shmdt(shmaddr);
    shmctl(shmid, IPC_RMID, NULL);

    printf("Parent process exiting.\n");

    return 0;
}

int main() {
    int fd;

    void* child_stack1 = malloc(STACK_SIZE);
    void* child_stack2 = malloc(STACK_SIZE);

    assert(child_stack1 != NULL);
    assert(child_stack2 != NULL);
    pid_t pid1, pid2;

    /* Spawn a new user namespace */
    int child_num = 1;
    printf("[*] Spawn new pid namespace #1\n");
    pid1 = clone(childFunc,
                child_stack1 + STACK_SIZE,
                SIGCHLD | CLONE_NEWPID, NULL);
    assert(pid1 != -1);
    child_num++;
    printf("[Parent] Child1 spawned with pid: %d\n", pid1);

    printf("[*] Spawn new pid namespace #2\n");
    pid2 = clone(childFunc1,
               child_stack2 + STACK_SIZE,
               SIGCHLD | CLONE_NEWPID, &child_num);
    assert(pid2 != -1);
    printf("[Parent] Child2 spawned with pid: %d\n", pid2);
	
    printf("Parent going to SLEEP\n");
    sleep(30);
    printf("Parent back from SLEEP, called WAIT\n");
    
    wait(NULL);

    printf("######################################################## Parent Returned from WAIT or SLEEP  #####################3\n");
    // while(1);
	
    return 0;
}


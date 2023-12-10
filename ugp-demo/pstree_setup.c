#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>

#define STACK_SIZE (1024 * 1024)
#define PORT 8080

struct CloneArgs {
    int* intArray;
    size_t arraySize;
};

// Function for the infinite while loop
static int infiniteLoop() {
    while (1) {
        // Do nothing, run in an infinite loop
    }

    return 0;
}

static int msgManager(pid_t targetPid) {
    int status, client_fd;
    struct sockaddr_in serv_addr;
    char pid_str[20];
    
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 0;
    }

    printf("Client FD received: %d\n", client_fd);
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
 
    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<= 0) {
        perror("Invalid address/ Address not supported \n");
        return 0;
    }

reconnect:

    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        perror("\nConnection Failed \n");
        sleep(2);
        goto reconnect;
    }

    // Get and send the target PID to the server
    sprintf(pid_str, "%d", targetPid);
    send(client_fd, pid_str, sizeof(pid_str), 0);

    printf("PID sent to the server: %s\n", pid_str);
    // sleep(10);
    while(1){
	    struct stat s;
	    if(fstat(client_fd, &s))
		    break;
	    printf("Still active, going to sleep\n");
	    sleep(1);
    }

    printf("Done with this round of migration\n");
    return 0;
}

static int migrating_func(void *arg) {
    struct CloneArgs *args = (struct CloneArgs *)arg;

    // printf("Migrating Child process PID: %d\n", getpid());

    // Do something and then message manager
    int add = 0;
    for(int i=0; i<100; i++){
        add++;
    }

    msgManager(args->intArray[0]);
    printf("[PID: %d] Migrating child process again active after 1st migration, add: %d\n", getpid(), add);

    for(int i=0; i<100; i++){
        add++;
    }

    msgManager(args->intArray[1]);
    printf("[PID: %d] Migrating child process again active after 2nd migration, add: %d\n", getpid(), add);

    for(int i=0; i<100; i++){
         add++;
    }

    msgManager(args->intArray[2]);
    printf("[PID: %d] Migrating child process again active after 3rd migration, add: %d\n", getpid(), add);

    while(1){
        printf("Socket child still active\n");
        sleep(5);
    }

    return 0;
}


static int child_function_with_socket(void *arg) {

    void* child_stack1 = malloc(STACK_SIZE);
    void* child_stack2 = malloc(STACK_SIZE);

    printf("Socket Parent process PID: %d\n", getpid());

    pid_t child_pid1 = clone(migrating_func, (void*)((unsigned long)child_stack1 + STACK_SIZE), SIGCHLD, arg);

    if (child_pid1 == -1) {
        perror("clone, socket parent process");
        exit(EXIT_FAILURE);
    }
    printf("[Child: Socket, Level: 1]cloned Child process with PID: %d\n", child_pid1);

    
    pid_t child_pid2 = clone(infiniteLoop, (void*)((unsigned long)child_stack2 + STACK_SIZE), SIGCHLD, NULL);

    if (child_pid2 == -1) {
        perror("clone, socket parent process");
        exit(EXIT_FAILURE);
    }
    printf("[Child: Socket, Level: 1]cloned Child process with PID: %d\n", child_pid2);

    // Wait for the second grandchild to finish namespace setup
    while(1);

    return 0;
}

static int child_function(void *arg) {
    int child_num = *(int*)(arg);
    int i = child_num;
    while(i--) {
	if(fork() == 0) while(1);
    }
    printf("[Child: %d, Level: 1]Created pstree with %d number of processes\n", child_num, child_num);
    while(1);
    return 0;
}

int main() {
    void* child_stack1 = malloc(STACK_SIZE);
    void* child_stack2 = malloc(STACK_SIZE);
    void* child_stack3 = malloc(STACK_SIZE);
    void* child_stack4 = malloc(STACK_SIZE);

    // Create the first child process in a new user and pid namespace
    int child_num = 3;
    pid_t child_pid1 = clone(child_function, (void*) ((unsigned long) child_stack1 + STACK_SIZE) , CLONE_NEWPID | SIGCHLD, &child_num);

    if (child_pid1 == -1) {
        perror("clone, main parent1");
        exit(EXIT_FAILURE);
    }

    printf("Main Parent: Cloned child with pid: %d\n", child_pid1);

    int child_num2 = 4;
    // Create the second child process in a new user and pid namespace
    pid_t child_pid2 = clone(child_function, (void*) ((unsigned long) child_stack2 + STACK_SIZE) , CLONE_NEWPID | SIGCHLD, &child_num2);

    if (child_pid2 == -1) {
        perror("clone, main parent2");
        exit(EXIT_FAILURE);
    }
    printf("Main Parent: Cloned child with pid: %d\n", child_pid2);
    int child_num3 = 5;

    // Create the third child process in a new user and pid namespace
    pid_t child_pid3 = clone(child_function, (void*) ((unsigned long) child_stack3 + STACK_SIZE) , CLONE_NEWPID | SIGCHLD, &child_num3);

    if (child_pid3 == -1) {
        perror("clone, main parent3");
        exit(EXIT_FAILURE);
    }
    printf("Main Parent: Cloned child with pid: %d\n", child_pid3);

    child_num ++;
    int pidArray[] = {child_pid3, child_pid2, child_pid1};
    size_t arraySize = sizeof(pidArray) / sizeof(pidArray[0]);

    struct CloneArgs args;
    args.intArray = pidArray;
    args.arraySize = arraySize;

    // Create the fourth child process in a new user and pid namespace
    pid_t child_pid4 = clone(child_function_with_socket, (void*) ((unsigned long) child_stack4 + STACK_SIZE) , CLONE_NEWPID | SIGCHLD, &args);

    if (child_pid4 == -1) {
        perror("clone, main parent4");
        exit(EXIT_FAILURE);
    }
    printf("Main Parent: Cloned child with pid: %d\n", child_pid4);

    while(1);

    printf("Parent process exiting.\n");

    return 0;
}

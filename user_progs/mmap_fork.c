#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#define KB * 1024
#define MB * 1024 * 1024
#define SIZE 8 KB 

const char *filename = "/home/jaya/ugp-ii/user_progs/fruits.txt";
void test_fs() {
	int fd = open(filename, O_WRONLY|O_CREAT);
	int childPid = fork();
	char buf[32] = "cherimoya\n";
	if(childPid==0) {
		printf("Child process (PID: %d) writing to file\n", getpid());
		write(fd, buf, 32);
		while(1) {}
	}
	else {

        	printf("Parent process (PID: %d) waiting for the child to complete\n", getpid());
       		wait(NULL); // Wait for the child process to complete
		printf("Parent process writing to file\n");
		write(fd, buf, 32);
		close(fd);
	}
}

void test_mem() {
	
    int *shared_memory;
    shared_memory = (int*)mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    pid_t child_pid = fork();

    if (child_pid == -1) {
        // Error occurred while forking
        perror("fork");
        munmap(shared_memory, SIZE); // Unmap the memory before exiting
        exit(EXIT_FAILURE);
    } else if (child_pid == 0) {
        // Child process
        printf("Child process (PID: %d) writing to shared memory\n", getpid());
        *shared_memory = 42; // Write to shared memory
        while(1) {}
    } else {
        // Parent process
        printf("Parent process (PID: %d) waiting for the child to complete\n", getpid());
        wait(NULL); // Wait for the child process to complete
        printf("Value read from shared memory in parent process: %d\n", *shared_memory);

        munmap(shared_memory, SIZE/2); // Unmap the memory after child process completes
        printf("Memory unmapped\n");
	shared_memory += SIZE/2;
	*shared_memory = 55;
    }
}

void test_multiple_child() {

    int num_children = 5;
    int pipes[num_children][2]; // 2D array to store pipe file descriptors
    pid_t child_pids[num_children]; // Array to store child process IDs

    // Create pipes for communication
    for (int i = 0; i < num_children; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Create child processes
    for (int i = 0; i < num_children; ++i) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            close(pipes[i][1]); // Close write end of the pipe
            while(1){
	   	 char message[100];
            	 read(pipes[i][0], message, sizeof(message));
           	 printf("Child %d received: %s\n", i + 1, message);
	    }
        } else {
            // Parent process
            child_pids[i] = pid;
            close(pipes[i][0]); // Close read end of the pipe
        }
    }

    // Parent process sends messages to children
    int i =0;
    while (1) {
        char message[100];
	sprintf(message, "Hello from parent: %d", i++);

        for (int i = 0; i < num_children; ++i) {
            write(pipes[i][1], message, sizeof(message));
        }

	sleep(5);
    }

    // Clean up child processes
    for (int i = 0; i < num_children; ++i) {
        waitpid(child_pids[i], NULL, 0);
    }

}

int main() {
    // test_mem();
    // test_fs();
    test_multiple_child();
    return 0;
}


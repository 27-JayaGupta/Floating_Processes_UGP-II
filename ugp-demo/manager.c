#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "migrate.h"

#define PORT 8080

int main(int argc, char* argv[]) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if(argc != 2) {
        printf("Usage: ./manager source_pid\n");
        exit(-1);
    }

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    pid_t source_pid = atoi(argv[1]);
    pid_t target_pid;

    if (listen(server_fd, 10) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);
    // Listen for incoming connections
    while(1) {
        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Read the client's PID
        valread = read(new_socket, buffer, sizeof(buffer));
        if(valread < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        target_pid = atoi(buffer);
        printf("Source Pid: %d, Target PID: %d\n", source_pid, target_pid);

        source_pid = __migrate(source_pid, target_pid);
    }

    return 0;
}

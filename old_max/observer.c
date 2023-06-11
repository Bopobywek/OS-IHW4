#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

int client_socket;
void sigint_handler(int signum) {
    printf("Observer stopped\n");
    close(client_socket);
    exit(0);
}

int main(int argc, char *argv[]) {
    unsigned short server_port;
    int working_time;
    char *server_ip;
    char buffer[BUFFER_SIZE];
    int bytes_received, total_bytes_received;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server address> <Server port>\n", argv[0]);
        exit(1);
    }

    server_ip = argv[1];

    server_port = atoi(argv[2]);

    client_socket = createClientSocket(server_ip, server_port);

    while (1) {
        char buffer[1024];
        if ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) < 0) {
            perror("recv() bad");
            close(client_socket);
            exit(-1);
        }

        if (bytes_received == sizeof(int)) {
            printf("Server closed the connection...\n");
            close(client_socket);
            exit(0);
        }

        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    close(client_socket);
    exit(0);
}

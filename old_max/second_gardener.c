#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

void work(int client_socket, int working_time, struct FieldSize field_size) {
    struct GardenerTask task;
    task.gardener_id = 2;
    task.working_time = working_time;
    int rows = field_size.rows;
    int columns = field_size.columns;

    int i = rows - 1;
    int j = columns - 1;
    task.status = 0;
    while (j >= 0) {
        while (i >= 0) {
            task.plot_i = i;
            task.plot_j = j;
            sendHandleRequest(client_socket, task);
            --i;
        }

        --j;
        ++i;

        while (i < rows) {
            task.plot_i = i;
            task.plot_j = j;
            sendHandleRequest(client_socket, task);
            ++i;
        }

        --i;
        --j;
    }

    task.status = 1;
    sendHandleRequest(client_socket, task);
}

int main(int argc, char *argv[]) {
    int client_socket;
    unsigned short server_port;
    int working_time;
    char *server_ip;
    char buffer[BUFFER_SIZE];
    int bytes_received, total_bytes_received;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Server address> <Server port> <Working time>\n", argv[0]);
        exit(1);
    }

    server_ip = argv[1];

    server_port = atoi(argv[2]);
    working_time = atoi(argv[3]);

    client_socket = createClientSocket(server_ip, server_port);

    struct FieldSize field_size;
    if ((bytes_received = recv(client_socket, &field_size, sizeof(field_size), MSG_NOSIGNAL)) !=
        sizeof(field_size)) {
        printf("Server closed the connection...\n");
        exit(0);
    }

    work(client_socket, working_time, field_size);
    printf("The work is completed\n");

    close(client_socket);
    exit(0);
}

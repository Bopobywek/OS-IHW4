#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

int createClientSocket(char *server_ip, int server_port) {
    int client_socket;

    if ((client_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Unable to create client socket");
        exit(-1);
    }

    struct sockaddr_in server_address = getServerAddress(server_ip, server_port);

    // TODO
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Unable to connect");
        exit(-1);
    }

    return client_socket;
}

struct sockaddr_in getServerAddress(char *server_ip, int server_port) {
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    return server_address;
}

void sendHandleRequest(int client_socket, struct GardenerTask task) {
    int status;
    int received;
    do {
        // &task -- сериализация, передаем структуру по байтам
        if (send(client_socket, &task, sizeof(task), 0) != sizeof(task)) {
            perror("send() bad");
            exit(-1);
        }

        if ((received = recv(client_socket, &status, sizeof(int), 0)) != sizeof(int)) {
            perror("recv() bad");
            exit(-1);
        }
    } while (status != 1);

    if (task.status != 1) {
        printf("Gardener %d handle plot (%d, %d)\n", task.gardener_id, task.plot_i, task.plot_j);
    }
}
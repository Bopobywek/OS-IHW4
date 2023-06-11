#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "common.h"

#define MAXRECVSTRING 1024

pthread_mutex_t mutex;

int client_socket;
void sigint_handler(int signum) {
    printf("Observer stopped\n");
    close(client_socket);
    pthread_mutex_destroy(&mutex);
    exit(0);
}

void *eventReader(void *args) {
    char recvString[MAXRECVSTRING + 1];
    int recvStringLen;

    while (1) {
        struct pollfd fd;
        fd.fd = client_socket;
        fd.events = POLLIN;

        int result = poll(&fd, 1, -1);
        if (result < 0) {
            perror("Can't poll socket");
            exit(-1);
        }

        pthread_mutex_lock(&mutex);
        if ((recvStringLen = recvfrom(client_socket, recvString, MAXRECVSTRING, 0, NULL, 0)) < 0) {
            perror("Can't bind");
            exit(-1);
        }
        pthread_mutex_unlock(&mutex);
        recvString[recvStringLen] = '\0';

        if (strcmp("_exit", recvString) == 0) {
            printf("Server closed connection\n");
            return 0;
        }
        printf("%s\n", recvString);
    }
}

void *pingSender(void *args) {
    enum observer_request_type type = PING;
    while (1) {
        pthread_mutex_lock(&mutex);
        send(client_socket, &type, sizeof(type), 0);
        pthread_mutex_unlock(&mutex);
    }
}

pthread_t sender;
pthread_t receiver;

int main(int argc, char *argv[]) {
    struct sockaddr_in multicastAddr;
    char *server_ip;
    unsigned short server_observable_port;
    char recvString[MAXRECVSTRING + 1];
    int recvStringLen;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Observer Port>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, sigint_handler);

    pthread_mutex_init(&mutex, NULL);

    server_ip = argv[1];
    server_observable_port = atoi(argv[2]);

    client_socket = createClientSocket(server_ip, server_observable_port);
    enum observer_request_type type = CONNECT;
    send(client_socket, &type, sizeof(type), 0);
    pthread_create(&sender, NULL, pingSender, NULL);
    pthread_create(&receiver, NULL, eventReader, NULL);

    pthread_join(receiver, NULL);

    close(client_socket);
    pthread_mutex_destroy(&mutex);
    exit(0);
}

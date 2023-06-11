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
pthread_t sender;
pthread_t receiver;
int client_socket;
void sigint_handler(int signum) {
    printf("Observer stopped\n");
    pthread_cancel(receiver);
    pthread_cancel(sender);
    close(client_socket);
    pthread_mutex_destroy(&mutex);
    exit(0);
}

double last_server_ping_ans;

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
            pthread_cancel(sender);
            exit(-1);
        }

        pthread_mutex_lock(&mutex);
        if ((recvStringLen = recvfrom(client_socket, recvString, MAXRECVSTRING, 0, NULL, 0)) < 0) {
            perror("Can't bind");
            pthread_cancel(sender);
            exit(-1);
        }

        int status = 0;
        if (recvStringLen == sizeof(status)) {
            status = *((int *)recvString);
        }

        last_server_ping_ans = time(0);

        pthread_mutex_unlock(&mutex);

        if (status == 1) {
            continue;
        }

        recvString[recvStringLen] = '\0';

        if (strcmp("_exit", recvString) == 0) {
            printf("Server closed connection\n");
            pthread_cancel(sender);
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

        sleep(3);

        pthread_mutex_lock(&mutex);
        if (difftime(time(0), last_server_ping_ans) > 10) {
            printf("Server doesnt respond on ping for 10 seconds\nConnection lost\n");
            pthread_cancel(receiver);
            exit(0);
        }
        pthread_mutex_unlock(&mutex);
    }
}

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

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    close(client_socket);
    pthread_mutex_destroy(&mutex);
    exit(0);
}

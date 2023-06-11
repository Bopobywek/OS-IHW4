#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAXRECVSTRING 1024

int sock;
void sigint_handler(int signum) {
    printf("Observer stopped\n");
    close(sock);
    exit(0);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in multicastAddr;
    char *multicastIP;
    unsigned short multicastPort;
    char recvString[MAXRECVSTRING + 1];
    int recvStringLen;
    struct ip_mreq multicastRequest;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Multicast IP> <Multicast Port>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, sigint_handler);

    multicastIP = argv[1];
    multicastPort = atoi(argv[2]);

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Can't socket");
        exit(-1);
    }

    memset(&multicastAddr, 0, sizeof(multicastAddr));
    multicastAddr.sin_family = AF_INET;
    multicastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    multicastAddr.sin_port = htons(multicastPort);

    if (bind(sock, (struct sockaddr *)&multicastAddr, sizeof(multicastAddr)) < 0) {
        perror("Can't bind");
        exit(-1);
    }

    multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastIP);
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&multicastRequest,
                   sizeof(multicastRequest)) < 0) {
        perror("Can't bind");
        exit(-1);
    }

    while (1) {
        if ((recvStringLen = recvfrom(sock, recvString, MAXRECVSTRING, 0, NULL, 0)) < 0) {
            perror("Can't bind");
            exit(-1);
        }

        recvString[recvStringLen] = '\0';

        if (strcmp("_exit", recvString) == 0) {
            printf("Server closed connection\n");
            break;
        }
        printf("%s\n", recvString);
    }

    close(sock);
    exit(0);
}

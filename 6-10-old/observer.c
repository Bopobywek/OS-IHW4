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
    struct sockaddr_in multicastAddr;   /* Multicast Address */
    char *multicastIP;                  /* IP Multicast Address */
    unsigned short multicastPort;       /* Port */
    char recvString[MAXRECVSTRING + 1]; /* Buffer for received string */
    int recvStringLen;                  /* Length of received string */
    struct ip_mreq multicastRequest;    /* Multicast address join structure */

    if (argc != 3) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Multicast IP> <Multicast Port>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, sigint_handler);

    multicastIP = argv[1];         /* First arg: Multicast IP address (dotted quad) */
    multicastPort = atoi(argv[2]); /* Second arg: Multicast port */

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Can't socket");
        exit(-1);
    }

    /* Construct bind structure */
    memset(&multicastAddr, 0, sizeof(multicastAddr));  /* Zero out structure */
    multicastAddr.sin_family = AF_INET;                /* Internet address family */
    multicastAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    multicastAddr.sin_port = htons(multicastPort);     /* Multicast port */

    /* Bind to the multicast port */
    if (bind(sock, (struct sockaddr *)&multicastAddr, sizeof(multicastAddr)) < 0) {
        perror("Can't bind");
        exit(-1);
    }

    /* Specify the multicast group */
    multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastIP);
    /* Accept multicast from any interface */
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    /* Join the multicast address */
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&multicastRequest,
                   sizeof(multicastRequest)) < 0) {
        perror("Can't bind");
        exit(-1);
    }

    while (1) {
        /* Receive a single datagram from the server */
        if ((recvStringLen = recvfrom(sock, recvString, MAXRECVSTRING, 0, NULL, 0)) < 0) {
            perror("Can't bind");
            exit(-1);
        }

        recvString[recvStringLen] = '\0';
        printf("%s\n", recvString); /* Print the received string */
    }

    close(sock);
    exit(0);
}

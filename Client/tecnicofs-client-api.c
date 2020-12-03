#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

char buffer[10];
char *socketName;
int sockfd;
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;


int setSockAddrUn(char *path, struct sockaddr_un *addr) {

    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {
    char newString[MAX_INPUT_SIZE];
    strcpy(newString, "c ");
    strcat(newString, filename);
    strcat(newString, " ");
    strcat(newString, &nodeType);
    strcat(newString, "\0");
    if (sendto(sockfd, newString, strlen(newString) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);
    }
    int i = atoi(buffer);
    return i;
}

int tfsDelete(char *path) {
    char newString[MAX_INPUT_SIZE];
    strcpy(newString, "d ");
    strcat(newString, path);
    strcat(newString, "\0");
    if (sendto(sockfd, newString, strlen(newString) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, &servlen) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);
    }
    int i = atoi(buffer);
    return i;
}

int tfsMove(char *from, char *to) {
    char newString[MAX_INPUT_SIZE];
    strcpy(newString, "m ");
    strcat(newString, from);
    strcat(newString, " ");
    strcat(newString, to);
    strcat(newString, "\0");
    if (sendto(sockfd, newString, strlen(newString) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, &servlen) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);
    }
    int i = atoi(buffer);

    return i;
}

int tfsLookup(char *path) {
    char newString[MAX_INPUT_SIZE];
    strcpy(newString, "l ");
    strcat(newString, path);
    strcat(newString, "\0");
    if (sendto(sockfd, newString, strlen(newString) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, &servlen) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);
    }
    int i = atoi(buffer);
    return i;
}

int tfsMount(char *sockPath) {
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("client: can't open socket");
        return 1;
    }

    unlink("/tmp/ClientSocket");//FIXME tirar unlink daqui deixar no fim e mudar logica do nome do socket
    clilen = setSockAddrUn("/tmp/ClientSocket", &client_addr);
    if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
        perror("client: bind error");
        return 1;
    }

    servlen = setSockAddrUn(sockPath, &serv_addr);
    return 0;
}

int tfsUnmount() {
    close(sockfd);

    unlink("/tmp/ClientSocket");
    return 0;
}

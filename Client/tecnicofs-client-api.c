#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

char buffer[1024];
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
    char *newString;
    strcat(newString, filename);
    strcat(newString, " ");
    strcat(newString, &nodeType);
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
    if (sendto(sockfd, path, strlen(path) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
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

int tfsMove(char *from, char *to) {
    char *newString;
    strcat(newString, from);
    strcat(newString, " ");
    strcat(newString, to);
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

int tfsLookup(char *path) {
    if (sendto(sockfd, path, strlen(path) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
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
}

int tfsMount(char * sockPath) {

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("client: can't open socket");
        exit(EXIT_FAILURE);
    }

    //unlink(sockPath);
    clilen = setSockAddrUn(sockPath, &client_addr);
    if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
        perror("client: bind error");
        exit(EXIT_FAILURE);
    }

    servlen = setSockAddrUn(sockPath, &serv_addr);
  return 1;
}

int tfsUnmount() {
    close(sockfd);

    //unlink(argv[1]);
    return 1;
}

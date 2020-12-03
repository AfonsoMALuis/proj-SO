#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

char socketNameClient[108];
int sockfd, msgReceived;
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;

int sendAndReceive(char *msgToSend){
    if (sendto(sockfd, msgToSend, strlen(msgToSend) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    if (recvfrom(sockfd, &msgReceived, sizeof(msgReceived), 0, 0, 0) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);

    }
    return msgReceived;
}


int setSockAddrUn(char *path, struct sockaddr_un *addr) {

    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {
    char msgToSend[MAX_INPUT_SIZE];
    strcpy(msgToSend, "c ");
    strcat(msgToSend, filename);
    strcat(msgToSend, " ");
    strcat(msgToSend, &nodeType);
    strcat(msgToSend, "\0");

    return sendAndReceive(msgToSend);
}

int tfsDelete(char *path) {
    char msgToSend[MAX_INPUT_SIZE];
    strcpy(msgToSend, "d ");
    strcat(msgToSend, path);
    strcat(msgToSend, "\0");

    return sendAndReceive(msgToSend);
}

int tfsMove(char *from, char *to) {
    char msgToSend[MAX_INPUT_SIZE];
    strcpy(msgToSend, "m ");
    strcat(msgToSend, from);
    strcat(msgToSend, " ");
    strcat(msgToSend, to);
    strcat(msgToSend, "\0");

    return sendAndReceive(msgToSend);
}

int tfsLookup(char *path) {
    char msgToSend[MAX_INPUT_SIZE];
    strcpy(msgToSend, "l ");
    strcat(msgToSend, path);
    strcat(msgToSend, "\0");

    return sendAndReceive(msgToSend);
}

int tfsPrint(char *filename){
    char msgToSend[MAX_FILE_NAME+2];
    strcpy(msgToSend, "p ");
    strcat(msgToSend, filename);
    strcat(msgToSend, "\0");

    return sendAndReceive(msgToSend);
}

int tfsMount(char *sockPath) {
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("client: can't open socket");
        return 1;
    }
    char socketId[10];
    strcpy(socketNameClient, "/tmp/ClientSocket");
    sprintf(socketId, "%d", sockfd);
    strcat(socketNameClient, socketId);
    //unlink("/tmp/ClientSocket");//FIXME tirar unlink daqui deixar no fim e mudar logica do nome do socket
    clilen = setSockAddrUn(socketNameClient, &client_addr);
    if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
        perror("client: bind error");
        return 1;
    }

    servlen = setSockAddrUn(sockPath, &serv_addr);
    return 0;
}

int tfsUnmount() {
    close(sockfd);

    unlink(socketNameClient);
    return 0;
}

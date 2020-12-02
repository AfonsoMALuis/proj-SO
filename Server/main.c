#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

pthread_mutex_t mutexCommands;
int sockfd;
socklen_t addrlen;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void applyCommands(){
    while (1){

        struct sockaddr_un client_addr;
        char in_buffer[MAX_INPUT_SIZE], out_buffer[10];
        int c;

        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0, (struct sockaddr *)&client_addr, &addrlen);


        char token, type;
        char name[MAX_INPUT_SIZE], name_origin[MAX_INPUT_SIZE], name_destiny[MAX_INPUT_SIZE];
        int numTokens;
        if (in_buffer[0] == 'm')
            numTokens = sscanf(in_buffer, "%c %s %s", &token, name_origin, name_destiny);
        else
            numTokens = sscanf(in_buffer, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult, moveResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        sprintf(out_buffer, "%d", create(name, T_FILE));
                        sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        sprintf(out_buffer, "%d", create(name, T_DIRECTORY));
                        sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                searchResult = lookup(name);
                sprintf(out_buffer, "%d", searchResult);
                sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                sprintf(out_buffer, "%d", delete(name));
                sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                break;
            case 'm':
                moveResult = move(name_origin, name_destiny);
                sprintf(out_buffer, "%d", moveResult);
                sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                if (moveResult == 1)
                    printf("Impossible to move, the file/directory %s doesn't exist\n", name_origin);
                else if (moveResult == 2)
                    printf("Impossible to move, the file/directory %s already exists\n", name_destiny);
                else if (moveResult == 0)
                    printf("Moved from: %s to %s\n", name_origin, name_destiny);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    int numThreads;
    numThreads = atoi(argv[1]);
    if (argc != 3){
        printf("A função tem de ter 3 argumentos\n");
        return (-1);
    }
    if (numThreads < 1){
        printf("A função tem de ter pelo menos uma thread\n");
        return (-1);
    }
    /* init filesystem */
    init_fs();

    struct sockaddr_un server_addr;
    char *socketName;

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    socketName = argv[2];

    addrlen = setSockAddrUn (socketName, &server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
    //-----------------------Criacao de Tarefas--------------------------------
    if (pthread_mutex_init(&mutexCommands, NULL) != 0){
        perror("Error initializing global mutexes!\n");
        exit(1);
    }
    int i;
    pthread_t tid[numThreads];
    for (i=0; i<numThreads; i++) {
        if (pthread_create (&tid[i], NULL, (void *(*)(void *)) applyCommands, NULL) != 0){
            printf("Erro ao criar tarefa.\n");
            return 1;
        }
    }
    for (i=0; i<numThreads; i++){
        if (pthread_join(tid[i], NULL) != 0){
            printf("Erro ao dar join da tarefa.\n");
            return 1;
        }
    }

    exit(EXIT_SUCCESS);
}

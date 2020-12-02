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

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
pthread_mutex_t mutexCommands;
int numberCommands = 0;
int headQueue = 0;
int sockfd;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

//int insertCommand(char* data) {
//    if(numberCommands != MAX_COMMANDS) {
//        strcpy(inputCommands[numberCommands++], data);
//        return 1;
//    }
//    return 0;
//}
//
//char* removeCommand() {
//    if (pthread_mutex_lock(&mutexCommands) != 0){
//        perror("Error locking commands mutex!");
//        exit(1);
//    }
//    if(numberCommands > 0){
//        numberCommands--;
//        char* commandReturn = inputCommands[headQueue++];
//        if (pthread_mutex_unlock(&mutexCommands) != 0){
//            perror("Error unlocking commands mutex!");
//            exit(1);
//        }
//        return commandReturn;
//    }
//    if (pthread_mutex_unlock(&mutexCommands) != 0){
//        perror("Error unlocking commands mutex!");
//        exit(1);
//    }
//    return NULL;
//}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

//void processInput(FILE *inputFile){
//    char line[MAX_INPUT_SIZE];
//
//
//    /* break loop with ^Z or ^D */
//    while (fgets(line, MAX_INPUT_SIZE, inputFile)) {
//        char token, type;
//        char name[MAX_INPUT_SIZE];
//
//        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);
//
//        /* perform minimal validation */
//        if (numTokens < 1) {
//            continue;
//        }
//        switch (token) {
//            case 'c':
//                if(numTokens != 3)
//                    errorParse();
//                if(insertCommand(line))
//                    break;
//                return;
//
//            case 'l':
//                if(numTokens != 2)
//                    errorParse();
//                if(insertCommand(line))
//                    break;
//                return;
//
//            case 'd':
//                if(numTokens != 2)
//                    errorParse();
//                if(insertCommand(line))
//                    break;
//                return;
//
//            case '#':
//                break;
//
//            case 'm':
//                if (numTokens != 3)
//                    errorParse();
//                if(insertCommand(line))
//                    break;
//                return;
//
//            default: { /* error */
//                errorParse();
//            }
//        }
//    }
//}

void applyCommands(socklen_t addrlen){
    while (1){
//        const char* command = removeCommand();
//        if (command == NULL){
//            break;
//        }

        struct sockaddr_un client_addr;
        char in_buffer[MAX_INPUT_SIZE], out_buffer[10];
        int c;

        addrlen=sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,
                     (struct sockaddr *)&client_addr, &addrlen);
        if (c <= 0) continue;
        //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0',
        in_buffer[c]='\0';

        printf("Recebeu mensagem de %s\n", client_addr.sun_path);



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
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        fprintf(out_buffer, "%d", create(name, T_DIRECTORY));
                        sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            case 'm':
                moveResult = move(name_origin, name_destiny);
                /*puts(name_origin);
                puts(name_destiny);*/
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
    numThreads = atoi(argv[3]);
    if (argc != 3){
        printf("A função tem de ter 3 argumentos\n");
        return (-1);
    }
    if (numThreads < 1){
        printf("A função tem de ter pelo menos uma thread\n");
        return (-1);
    }
    //struct timeval start, end;
    /* init filesystem */
    init_fs();

    /* process input and print tree */
//    FILE *inputFile, *outputFile;
//    inputFile = fopen(argv[1], "r");
//    if(inputFile == NULL) {
//        perror("Error opening file");
//        return(-1);
//    }
//    gettimeofday(&start, NULL);
//    processInput(inputFile);
//    fclose(inputFile);

    struct sockaddr_un server_addr;
    char *socketName;
    socklen_t addrlen;

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    socketName = argv[2];

//    unlink(path);

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
        if (pthread_create (&tid[i], NULL, (void *(*)(void *)) applyCommands, addrlen) != 0){
            printf("Erro ao criar tarefa.\n");
            return 1;
        }
    }
    for (i=0; i<numThreads; i++){
        if (pthread_join(tid[i], NULL) != 0){
            printf("Erro ao dar joind da tarefa.\n");
            return 1;
        }
    }
//    gettimeofday(&end, NULL);
//    //-----------------Fim de execucao de Tarefas------------------------------
//    outputFile = fopen(argv[2], "w");
//    if(inputFile == NULL) {
//        perror("Error creating file");
//        return(-1);
//    }
//    print_tecnicofs_tree(outputFile);
//    fclose(outputFile);
//
//    /* release allocated memory */
//    destroy_fs();
//    printf("TecnicoFS completed in %.4lf seconds\n",
//           ((end.tv_sec + end.tv_usec * (1e-6))) -
//            (start.tv_sec + start.tv_usec * (1e-6)));

    exit(EXIT_SUCCESS);
}

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
pthread_mutex_t mutexCommands;
int numberCommands = 0;
int headQueue = 0;

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if (pthread_mutex_lock(&mutexCommands) != 0){
        perror("Error locking commands mutex!");
        exit(1);
    }
    if(numberCommands > 0){
        numberCommands--;
        if (pthread_mutex_unlock(&mutexCommands) != 0){
            perror("Error unlocking commands mutex!");
            exit(1);
        }
        return inputCommands[headQueue++];
    }
    if (pthread_mutex_unlock(&mutexCommands) != 0){
        perror("Error unlocking commands mutex!");
        exit(1);
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(FILE *inputFile){
    char line[MAX_INPUT_SIZE];


    /* break loop with ^Z or ^D */
    while (fgets(line, MAX_INPUT_SIZE, inputFile)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case '#':
                break;

            default: { /* error */
                errorParse();
            }
        }
    }
}

void applyCommands(char *strategy){
    while (1){
        const char* command = removeCommand();
        if (command == NULL){
            break;
        }


        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE, strategy);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY, strategy);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                searchResult = lookup(name, strategy);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name, strategy);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5){
        printf("A função tem de ter 5 argumentos\n");
        return (-1);
    }
    if (strcmp(argv[4], "nosync") == 0 && strcmp(argv[3], "1") != 0){
        printf("Quando se escolhe a estratégia \"nosync\", deve-se escolher apenas 1 thread\n");
        return (-1);
    }
    if (strcmp(argv[4], "nosync") != 0 && strcmp(argv[4], "mutex") != 0 && strcmp(argv[4], "rwlock") != 0){
        printf("A estratégia escolhida tem de ser \"nosync\", \"mutex\" ou \"rwlock\"\n");
        return (-1);
    }
    struct timeval start, end;
    /* init filesystem */
    init_fs(argv[4]);

    /* process input and print tree */
    FILE *inputFile, *outputFile;
    inputFile = fopen(argv[1], "r");
    if(inputFile == NULL) {
        perror("Error opening file");
        return(-1);
    }
    processInput(inputFile);
    fclose(inputFile);
    gettimeofday(&start, NULL);
    //-----------------------Criacao de Tarefas--------------------------------
    if (pthread_mutex_init(&mutexCommands, NULL) != 0){
        perror("Error initializing global mutexes!\n");
        exit(1);
    }
    int i, numThreads;
    numThreads = atoi(argv[3]);
    pthread_t tid[numThreads];
    for (i=0; i<numThreads; i++) {
        if (pthread_create (&tid[i], NULL, (void *(*)(void *)) applyCommands, argv[4]) != 0){
            printf("Erro ao criar tarefa.\n");
            return 1;
        }
    }
    for (i=0; i<numThreads; i++){
        pthread_join(tid[i], NULL);
    }
    //applyCommands((int) argv[3]);
    //-----------------Fim de execucao de Tarefas------------------------------
    outputFile = fopen(argv[2], "w");
    print_tecnicofs_tree(outputFile);
    fclose(outputFile);

    /* release allocated memory */
    destroy_fs(argv[4]);
    gettimeofday(&end, NULL);
    printf("TecnicoFS completed in %.4lf seconds\n",
           ((end.tv_sec + end.tv_usec * (1e-6))) -
            (start.tv_sec + start.tv_usec * (1e-6)));

    exit(EXIT_SUCCESS);
}

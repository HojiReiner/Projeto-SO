#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include "fs/operations.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int headQueue = 0;

//~ Input Variables
FILE *InputFile;
FILE *OutputFile;
char *InputFile_Name;
char *OutputFile_Name;

//~ Time Variables
struct timeval start, end;
double exectime;

//~ Lock Variables
pthread_mutex_t lock;

//~ Condition Variables
pthread_cond_t rmCommand;
pthread_cond_t addCommand; 
int nCommands = 0;
int addPos = 0;
int rmPos = 0;
bool done = false;


void Lock(){
    if(pthread_mutex_lock(&lock) != 0){
        fprintf(stderr, "Error: problem locking mutex\n");
        exit(EXIT_FAILURE);
    }
}


void Unlock(){
    if(pthread_mutex_unlock(&lock) != 0){
        fprintf(stderr, "Error: problem unlocking mutex\n");
        exit(EXIT_FAILURE);
    }
}


void Wait(pthread_cond_t* cond){
    if(pthread_cond_wait(cond,&lock) != 0){
        fprintf(stderr, "Error: problem in condition waiting\n");
        exit(EXIT_FAILURE);
    }
}


void Signal(pthread_cond_t* cond){
    if(pthread_cond_signal(cond) != 0){
        fprintf(stderr, "Error: problem in signaling condition\n");
        exit(EXIT_FAILURE);
    }
}


void Broadcast(pthread_cond_t* cond){
    if(pthread_cond_broadcast(cond) != 0){
        fprintf(stderr, "Error: problem in signaling condition\n");
        exit(EXIT_FAILURE);
    }
}


//^ Inserts command on vector inputCommands
int insertCommand(char* data) {
    if(addPos == MAX_COMMANDS){
        addPos = 0;
    }
    strcpy(inputCommands[addPos++], data);
    return 1;

}


//^ Removes command from vector inputCommands
char* removeCommand() {
    if(rmPos == MAX_COMMANDS){
        rmPos = 0;
    }
    return inputCommands[rmPos++];
}


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}


void processInput(){
    char line[MAX_INPUT_SIZE];
    char name[MAX_INPUT_SIZE];
    char token, type;
    int numTokens;

    //* Start timer
    gettimeofday(&start, NULL);
    
    //* Open file
    if((InputFile = fopen(InputFile_Name, "r")) == NULL){
        fprintf(stderr, "Error: file %s doesn't exist\n",InputFile_Name);
        exit(EXIT_FAILURE);
    };

    //* Read file and copy
    while (fgets(line, sizeof(line)/sizeof(char), InputFile)) {
        numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        if (numTokens < 1) {
            continue;
        }

        Lock();
        while(nCommands == MAX_COMMANDS){
            Wait(&addCommand);
        }
        nCommands++;

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
            
            case 'm':
                insertCommand(line);
                break;

            case '#':
                nCommands--;
                break;
            
            default: { //* error
                errorParse();
            }
        }
        Signal(&rmCommand);
        Unlock();
    }

    Lock();
    done = true;
    Broadcast(&rmCommand);
    Unlock();

    fclose(InputFile);
}

void *applyCommands(){
    const char* command;
    char token;
    char name[MAX_INPUT_SIZE];
    char target[MAX_INPUT_SIZE];
    int numTokens;
    int searchResult;
    
    while(true){

        Lock();
        while(nCommands == 0 && !done){
            Wait(&rmCommand);
        }

        if(nCommands == 0 && done){
            Unlock();
            return NULL;
        }

        command = removeCommand();
        nCommands--;

        numTokens = sscanf(command, "%c %s %s", &token, name, target);
        if (numTokens < 2){
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        Signal(&addCommand);
        Unlock();

        
        switch (token){
            case 'c':
                switch (target[0]){
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type \n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookfor(name);
                if (searchResult >= 0){
                    printf("Search: %s found\n", name);
                }
                else{
                    printf("Search: %s not found\n", name);
                }
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;

            case 'm':
                printf("Move: %s to %s\n", name, target);
                move(name, target);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}


void threadPool(){
    pthread_t tid[numberThreads];
    int i;

    //* Creates threads
    for(i=0; i<numberThreads; i++){ 
        if(pthread_create (&tid[i], NULL, applyCommands, NULL) != 0){
            fprintf(stderr, "Error: problems creating thread\n");
            exit(EXIT_FAILURE);
        }
    }

    //* Process input
    processInput();

    //* Joins threads
    for(i = 0 ; i < numberThreads ; i++){
        if(pthread_join (tid[i], NULL) != 0){
            fprintf(stderr, "Error: problems joining thread\n");
            exit(EXIT_FAILURE);
        }
    }

    //* End timer
    gettimeofday(&end, NULL);
    exectime = (end.tv_sec-start.tv_sec)*1000000 + end.tv_usec-start.tv_usec;
    printf("TecnicoFS completed in [%0.4f] seconds. \n", exectime/1000000);
}

int main(int argc, char* argv[]){   
    if(argc != 4){
        fprintf(stderr, "Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    InputFile_Name = argv[1];
    OutputFile_Name = argv[2];

    if((numberThreads =  atoi(argv[3])) <= 0){
        fprintf(stderr, "Error: not a valid number of threads\n");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&rmCommand, NULL);
    pthread_cond_init(&addCommand, NULL);

    //* init filesystem 
    init_fs();

    //* Initiates thread pool and executes the commands
    threadPool();

    //* Open/Create output file
    if((OutputFile = fopen(OutputFile_Name, "w")) == NULL){
        fprintf(stderr, "Error: file %s doesn't exist\n",OutputFile_Name);
        exit(EXIT_FAILURE);
    }
    print_tecnicofs_tree(OutputFile);
    //* Closes file
    fclose(OutputFile);

    //* Release allocated memory
    destroy_fs();
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&rmCommand);
    pthread_cond_destroy(&addCommand);

    exit(EXIT_SUCCESS);
}
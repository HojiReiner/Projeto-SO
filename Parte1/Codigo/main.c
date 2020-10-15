#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include "fs/sync.h"
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

//^Input Variables
FILE *InputFile;
FILE *OutputFile;
char *InputFile_Name;
char *OutputFile_Name;
char *syncStrat;

struct timeval start, end;
double exectime;

Sync_Lock remove_lock;
Sync_Lock commands_lock;

//^ Inserts command on vector inputCommands
int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

//^ Removes command from vector inputCommands
char* removeCommand() {
    Lock(remove_lock, NA);
    if(numberCommands > 0){
        numberCommands--;
        Unlock(remove_lock);
        return inputCommands[headQueue++];
    }
    Unlock(remove_lock);
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(){
    char line[MAX_INPUT_SIZE];
    //* Open file
    if((InputFile = fopen(InputFile_Name, "r")) == NULL){
        fprintf(stderr, "Error: file %s doesn't exist\n",InputFile_Name);
        exit(EXIT_FAILURE);
    };

    //* Read file and copy
    while (fgets(line, sizeof(line)/sizeof(char), InputFile)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        //* Perform minimal validation
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
    fclose(InputFile);
}

void *applyCommands(){   
    while (numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
        }


        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2){
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token){
            case 'c':
                switch (type){
                    case 'f':
                        Lock(commands_lock, LWRITE);
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        Unlock(commands_lock);
                        break;
                    case 'd':
                        Lock(commands_lock, LWRITE);
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        Unlock(commands_lock);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                Lock(commands_lock, LREAD);
                searchResult = lookup(name);
                if (searchResult >= 0){
                    printf("Search: %s found\n", name);
                    Unlock(commands_lock);
                }
                else{
                    printf("Search: %s not found\n", name);
                    Unlock(commands_lock);
                }
                break;
            case 'd':
                Lock(commands_lock, LWRITE);
                printf("Delete: %s\n", name);
                delete(name);
                Unlock(commands_lock);
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

    //* Start timer
    gettimeofday(&start, NULL);

    for(i=0; i<numberThreads; i++){ 
        if(pthread_create (&tid[i], NULL, applyCommands, NULL) != 0){
            fprintf(stderr, "Error: problems creating thread\n");
            exit(EXIT_FAILURE);
        }
    }

    for(i=0 ; i < numberThreads ; i++){
        if(pthread_join (tid[i], NULL) != 0){
            fprintf(stderr, "Error: problems joining thread\n");
            printf("BOOOP");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* argv[]){   
    if(argc != 5){
        fprintf(stderr, "Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    InputFile_Name = argv[1];
    OutputFile_Name = argv[2];
    numberThreads =  atoi(argv[3]);
    syncStrat = argv[4];


    /*Creates the lock for removeCommands*/
    if(strcmp(syncStrat,"nosync") == 0){
        if(numberThreads == 1){
            remove_lock = Lock_Init(syncStrat);
        }
        else{
            fprintf(stderr, "Error: can only use nosync with 1 thread\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(strcmp(syncStrat,"mutex") == 0 || strcmp(syncStrat,"rwlock") == 0){
        remove_lock = Lock_Init("mutex");
    }
    else{
        fprintf(stderr, "Error: %s is not an available sync strategy\n",syncStrat);
        exit(EXIT_FAILURE);
    }

    /*Creates the lock for applyCommands*/
    commands_lock = Lock_Init(syncStrat);


    /* init filesystem */
    init_fs(syncStrat);

    /* process input and print tree */
    processInput();

    threadPool();

    //* Open/Create output file
    OutputFile = fopen(OutputFile_Name, "w");
    print_tecnicofs_tree(OutputFile);
    //* Closes file
    fclose(OutputFile);

    /* release allocated memory */
    
    destroy_fs();
    Destroy_Lock(remove_lock);
    Destroy_Lock(commands_lock);

    // * End timer
    gettimeofday(&end, NULL);
    exectime = (end.tv_sec-start.tv_sec)*1000000 + end.tv_usec-start.tv_usec;
    printf("TecnicoFS completed in [%0.4f] seconds. \n", exectime/1000000);

    exit(EXIT_SUCCESS);
}
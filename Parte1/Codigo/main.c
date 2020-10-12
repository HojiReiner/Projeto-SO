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

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

//^Input Variables
FILE *InputFile;
FILE *OutputFile;
char *InputFile_Name;
char *OutputFile_Name;
char *SyncStrat;

struct timeval start, end;
double exectime;

/*
*Possible code for pool
*(Possible code 1)
*   while(numberCommands > 0){
*       for(i=0; i<numberThreads; i++){ 
*           pthread_create (&tid[i], NULL, applyComand, NULL);
*       }
*       
*       for(i=0; i<numberThreads; i++){
*           pthread_join(tid[i])
*       }
*   }


*(Possible code 2)
*   for(i=0; i<numberThreads; i++){
*           removeCommand();
*           pthread_create (&tid[i], NULL, applyComand, NULL);
*       }
*   for(i=0; i<numberThreads && numberCommand >0; i++)
*       pthread_join(tid[i]);
*       pthread_create (&tid[i], NULL, applyComand, NULL);
*       if((i + 1) == numberThreads){
*           i = 0;
*       }
*   }
*/


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
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
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

void applyCommands()
{   
    //!Remover o while;
    while (numberCommands > 0)
    {
        const char* command = removeCommand();
        if (command == NULL)
        {
            continue;
        }


        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2)
        {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token)
        {
            case 'c':
                switch (type)
                {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
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
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    //!pthread_exit(EXIT_SUCCESS);
}
void pool(){
    //pthread_t tid [numberThreads];
    //* Start timer
    gettimeofday(&start, NULL);
    applyCommands();
}

int main(int argc, char* argv[])
{   
    if(argc != 3)
    {
        fprintf(stderr, "Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    InputFile_Name = argv[1];
    OutputFile_Name = argv[2];
    //!numberThreads =  atoi(argv[3]);
    //!SyncStrat = argv[4];

    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput();

    //! Not final name ?
    pool();

    //* Open/Create output file
    OutputFile = fopen(OutputFile_Name, "w");
    print_tecnicofs_tree(OutputFile);
    //* Closes file
    fclose(OutputFile);

    /* release allocated memory */
    destroy_fs();
    
    // * End timer
    gettimeofday(&end, NULL);
    exectime = (end.tv_sec-start.tv_sec)*1000000 + end.tv_usec-start.tv_usec;
    printf("TecnicoFS completed in [%0.4f] seconds. \n", exectime/1000000);

    exit(EXIT_SUCCESS);
}
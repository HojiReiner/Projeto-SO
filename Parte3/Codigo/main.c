#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <strings.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "fs/operations.h"

#define MAX_INPUT_SIZE 100

int numberThreads = 0;
int sockfd;

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}


int setAddr(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}


void *applyCommands(){
    struct sockaddr_un client_addr;
    socklen_t addrlen = sizeof(struct sockaddr_un);
    FILE *outFile; 
    char command[MAX_INPUT_SIZE];
    char name[MAX_INPUT_SIZE];
    char target[MAX_INPUT_SIZE];
    char token;
    int numTokens;
    int result;
    
    while(1){
        result = recvfrom(sockfd, command, sizeof(command), 0,(struct sockaddr *)&client_addr, &addrlen);
        if (result <= 0) continue;

        numTokens = sscanf(command, "%c %s %s", &token, name, target);
        if (numTokens < 2){
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        
        switch (token){
            case 'c':
                switch (target[0]){
                    case 'f':
                        printf("Create file: %s\n", name);
                        result = create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        result =  create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type \n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                result = lookfor(name);
                if (result >= 0){
                    printf("Search: %s found\n", name);
                }
                else{
                    printf("Search: %s not found\n", name);
                }
                break;
            case 'd':
                printf("Delete: %s\n", name);
                result = delete(name);
                break;

            case 'm':
                printf("Move: %s to %s\n", name, target);
                result = move(name, target);
                break;

            case 'p':
                printf("Print tree to %s\n", name);
                if((outFile = fopen(name, "w")) == NULL){
                    fprintf(stderr, "Error: problem opening %s\n",name);
                    exit(EXIT_FAILURE);
                }
                result = print_tecnicofs_tree(outFile);
                fclose(outFile);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        sendto(sockfd, &result, sizeof(int), 0, (struct sockaddr *)&client_addr, addrlen);
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


    //* Joins threads
    for(i = 0 ; i < numberThreads ; i++){
        if(pthread_join (tid[i], NULL) != 0){
            fprintf(stderr, "Error: problems joining thread\n");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* argv[]){
    char *path;
    struct sockaddr_un addr;
    socklen_t addrlen;


    if(argc != 3){
        fprintf(stderr,"Server: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    if((numberThreads =  atoi(argv[1])) <= 0){
        fprintf(stderr,"Server: not a valid number of threads\n");
        exit(EXIT_FAILURE);
    }

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr,"Server: can't open socket\n");
        exit(EXIT_FAILURE);
    }

    path = argv[2];
    unlink(path);

    addrlen = setAddr(path, &addr);
    if (bind(sockfd, (struct sockaddr *) &addr, addrlen) < 0) {
        fprintf(stderr,"Server: bind error");
        exit(EXIT_FAILURE);
    }

    //* init filesystem 
    init_fs();

    //* Initiates thread pool and executes the commands
    threadPool();

    //* Release allocated memory
    destroy_fs();

    exit(EXIT_SUCCESS);
}
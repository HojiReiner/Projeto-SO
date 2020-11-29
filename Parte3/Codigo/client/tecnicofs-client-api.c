#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#define MAX_SIZE 100

int sockfd;
socklen_t server_len;
struct sockaddr_un server_addr;
char clientName[MAX_INPUT_SIZE];

int setAddr(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}


void snd(char * command){
  
  if (sendto(sockfd, command, strlen(command)+1, 0, (struct sockaddr *) &server_addr, server_len) < 0) {
    fprintf(stderr,"Client: sendto error\n");
    exit(EXIT_FAILURE);
  }
}

int rcv(){
  int res;
  if (recvfrom(sockfd, &res, sizeof(res), 0, 0, 0) < 0) {
    fprintf(stderr,"Client: recvfrom error\n");
    exit(EXIT_FAILURE);
  } 
  return res;
}

int tfsCreate(char *filename, char nodeType) {
  char command[MAX_SIZE];
  sprintf(command,"c %s %c", filename, nodeType);
  
  snd(command);
  return rcv();
}


int tfsDelete(char *path) {
  char command[MAX_SIZE];
  sprintf(command,"d %s", path);
  
  snd(command);
  return rcv();
}


int tfsMove(char *from, char *to) {
  char command[MAX_SIZE];
  sprintf(command,"m %s %s", from, to);
  
  snd(command);
  return rcv();
}


int tfsLookup(char *path) {
  char command[MAX_SIZE];
  sprintf(command,"l %s", path);
  
  snd(command);
  return rcv();
}


int tfsPrint(char *filename) {
  char command[MAX_SIZE];
  sprintf(command,"p %s", filename);
  
  snd(command);
  return rcv();
}

//!
int tfsMount(char * sockPath) {
  struct sockaddr_un client_addr;
  socklen_t client_len;

  //* Set up client sock
  sprintf(clientName,"/tmp/CLIENT_%d", getpid());

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    fprintf(stderr, "Client: Unable to create socket");
    exit(EXIT_FAILURE);
  }
  
  unlink(clientName);
  client_len = setAddr(clientName, &client_addr);

  if (bind(sockfd, (struct sockaddr *) &client_addr, client_len) < 0) {
    fprintf(stderr, "Client: Unable to bind socket");
    exit(EXIT_FAILURE);
  }  

  //* Set up server sock
  server_len = setAddr(sockPath, &server_addr);

  return SUCCESS;
}

int tfsUnmount() {
  close(sockfd);
  unlink(clientName);
  return SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

//initialize functions provided in the code to prevent function not found errors
int doprocessing (int sock);
getwords(char *line, char *words[], int *maxwords);
int put(char* key, char* value, char* res);
int get(char* key, char* res);
int del(char* key, char* res);

//Define the KeyValue struct
struct KeyPair {
  char key[33];
  char value[1024];
};

int id;
const int NUM_KEY_PAIRS = 1024;
int mem_id;
struct KeyPair* keys;

int main(int argc, char *argv[] ) {

  int sockfd, newsockfd, portno, clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n, pid, ptr;

  //method for creating a shared memory segment
  mem_id = shmget(IPC_PRIVATE, sizeof(struct KeyPair) * NUM_KEY_PAIRS, IPC_CREAT|0777);

  //method for loading the content of the shared memory segment to keys
  keys = (struct KeyPair*)shmat(mem_id, 0, 0);

  // DEBUG: printf("Keys: %p\n", keys);

  //fill the shared memory values with zero
  memset(keys, 0, sizeof(struct KeyPair) * NUM_KEY_PAIRS);
  //create a socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }

  //fill socket with zero
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = 6669;

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno); //convert the port no. to network format

  //bind the address to socket
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    exit(1);
  }

  // start listening on socket and waiting for request
  listen(sockfd,5);
  clilen = sizeof(cli_addr);

  //INFINITE LOOP
  while (1) {
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0) {
      perror("ERROR on accept");
      exit(1);
    }

    //create child process if needed
    pid = fork();

    // DEBUG: result = shmctl(id, cmd, buffer);

    if (pid < 0) {
      perror("ERROR on fork");
      exit(1);
    }

    //check if a new process was generated
    if (pid == 0) {
      //the process is kept alive until the client closes the connection
      while (doprocessing(newsockfd) == 0) {}
      //closes the socket and basic clean up of the shared memory
      close(sockfd);
      shmdt(keys);
      shmctl(id, IPC_RMID, 0);
      exit(0);
    } else {
        close(newsockfd);
    }
  }
}

//handle user input
int doprocessing (int sock) {
  int n;
  char buffer[256];
  bzero(buffer, 256);
  n = read(sock,buffer,255);
  char *words[sizeof(buffer)/2];
  char res[256];

  //divide buffer into array of words
  int nwords = getwords(buffer, words, 10);

  //errorhandling
  if (n < 0) {
    perror("ERROR reading from socket");
    exit(1);
  }

  printf("Incomming: %s\n", buffer);

  if (n < 0) {
    perror("ERROR writing to socket");
    exit(1);
  }

  //cancels the request by "EXIT"
  if(strncmp(buffer, "EXIT", 4) == 0) {
    return 1;
  }

  //handle user input
  if (strncmp(words[0], "TST", 3) == 0){
    printf("ERROR SUCCESS \n");
  } else if(strncmp(words[0], "PUT", 3) == 0) {
    put(words[1], words[2], res);
  } else if(strncmp(words[0], "GET", 3) == 0) {
    get(words[1], res);
  } else if(strncmp(words[0], "DEL", 3) == 0) {
    del(words[1], res);
  }

  //Output handling
  char szOutput[256];
  sprintf(szOutput, "Output: %s\n Id: %i\n", res, id);
  n = write(sock, szOutput, strlen(szOutput)); //write answer to client

  return 0;

}
//takes the string and returns an array of  words
getwords (char *line, char *words[], int *maxwords) {
  char *p = line;
  int nwords = 0;

  while (1) {
    while (isspace(*p)) {
      p++;
    }

    if (*p == "\0") {
      return nwords;
    }

    words[nwords++] = p;

    while (!isspace (*p) && *p != '\0') {
      p++;
    }
    if (*p == '\0') {
      return nwords;
    }

    *p++ = '\0';

    if (nwords >= maxwords) {
      return nwords;
    }
  }
}

//checks if key is existing, if not then store in value into struct
int put(char* key, char* value, char* res) {
  for(int i = 0; i < NUM_KEY_PAIRS; i++) {
    if (strlen(keys[i].key) == 0) {
      strncpy(keys[i].key, key, sizeof(keys[i].key));
      strncpy(keys[i].value, value, sizeof(keys[i].value));
      return 0;
    }
  }
  strcpy(res, "NIL");
  return 1;
}

//checks if key exists, if yes returns the value of the requested key
int get(char* key, char* res) {
  for(int i = 0; i < NUM_KEY_PAIRS; i++) {
    if (strncmp(keys[i].key, key, sizeof(keys[i].key)) == 0) {
      strcpy(res, keys[i].value);
      return 0;
    }
  }
  strcpy(res, "NIL");
  return 1;
}
//checks if key exists, if yes deletes the values
int del(char* key, char* res) {

  for(int i = 0; i < NUM_KEY_PAIRS; i++) {
    if (strncmp(keys[i].key, key, sizeof(keys[i].key)) == 0) {
      strncpy(res, keys[i].value, sizeof(keys[i].value));
      memset(&keys[i], 0, sizeof(keys[i]));
      return 0;
    }
  }
  strcpy(res, "NIL");
  return 1;
}
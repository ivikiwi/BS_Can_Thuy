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

int put(char* key, char* value, char* res);
int get(char* key, char* res);
int del(char* key, char* res);
int doproc(int sock);
int getwords(char *line, char *words[], int maxwords);

struct KeyPair {
	char key[33];
	char value[1024];
};

struct KeyPair keys[10];
const int NUM_KEYS = 1024;
int id;
int mem_id, *shar_mem;
int pid[10];

int main(int argc, char *argv[]) {
	int mysocket , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char client_message[2000];
	int n, pid, ptr;

	//methode um ein shared memory segment zu erstellen - mit IPC_RIVATE erzeugt der UNIX-KERN den numerischen schlüssel selbst
 	mem_id = shmget(IPC_PRIVATE, sizeof(struct KeyPair) * 10, IPC_CREAT|0777);

 	//methode, um den inhalt des shared memory segments in die keys zu laden
  	shar_mem = (int *)shmat(mem_id, 0, 0);
	*shar_mem = 0;
  	//keys = shmat(mem_id, 0, 0);

  	//printf("Keys: %p\n", keys);

  	//fill the shared memory values with zero
  	//memset(keys, 0, sizeof(struct KeyPair) * 10);

	mysocket = socket(AF_INET , SOCK_STREAM , 0);

	// stellt sicher, falls ein port bereits genutzt wird, der port forciert erneut genutzt werden soll
	int option = 1;
	setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	if(mysocket == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");
	
	//Socket adresse und port binden
	server.sin_family = AF_INET; //IPv4
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8888 );

	//Binden
	if( bind(mysocket,(struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(mysocket, 3);

	//Accept and incoming connection
	puts("Warten auf eingehende connections...");
	c = sizeof(struct sockaddr_in);

	/*client_sock = accept(mysocket, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
	puts("Connection akzeptiert");*/

	/*while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0) {
		//write(client_sock , client_message , strlen(client_message));
		char *buf = client_message;
		int i = 0;
	    char *p = strtok(buf, " ");
	    char *array[3];
	    char res[256];

	    while (p != NULL) {
	        array[i++] = p;
	        p = strtok (NULL, " ");
	    }

	    //for (i = 0; i < 3; ++i) 
	     //   printf("%s\n", array[i]);

		// check user input
		if(strncmp(client_message, "PUT", 3) == 0) {
			//write(client_sock , client_message, strlen(client_message));
			put(array[1], array[2], res);
		} 
		if(strncmp(client_message, "GET", 3) == 0) {
			get(array[1], res);
		}
		if(strncmp(client_message, "DEL", 3) == 0) {
			del(array[1], res);
		}
		//char szOutput[256];
		//sprintf(szOutput, "Output: %s\n", res);
		write(client_sock, res, strlen(res)); //write answer to client

		// array leeren
	    memset(array, 0, 3);
	}

	if(read_size == 0) {
		puts("Client disconnected");
	 	fflush(stdout);
	} else if(read_size == -1) {
		perror("recv failed");
	}
	*/
	//return 0;

	while (1) {
	    client_sock = accept(mysocket, (struct sockaddr *)&client, (socklen_t*)&c);

	    if (client_sock < 0) {
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
	      while (doproc(client_sock) == 0) {}
	      //closes the socket and basic clean up of the shared memory
	      close(mysocket);
	      shmdt(keys);
	      shmctl(id, IPC_RMID, 0);
	      exit(0);
	    } else {
	        close(client_sock);
	    }
	}
}

int doproc(int sock) {
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

int getwords (char *line, char *words[], int maxwords) {
  char *p = line;
  int nwords = 0;

  while (1) {
    while (isspace(*p)) {
      p++;
    }

   /* if (*p == "\0") {
      return nwords;
    }*/
      if(strcmp(p, "\0")) {
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


int put(char* key, char* value, char* res) {
	//printf("%lu\n", sizeof(keys)/sizeof(keys[0]));
	for(int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
		if (strlen(keys[i].key) == 0) {
			strncpy(keys[i].key, key, sizeof(keys[i].key));
    	  	strncpy(keys[i].value, value, sizeof(keys[i].value));
    	  	strcpy(res, "geklappt\n");
			return 1;
		} 
	}
	strcpy(res, "nicht geklappt\n");
	return 1;
	/*for(int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
		if(strlen(keys[i].key) == 0)
	}*/

	/*for(int i = 0; i < NUM_KEYS; i++) {
    	if (strlen(keys[i].key) == 0) {
    	  strncpy(keys[i].key, key, sizeof(keys[i].key));
    	  strncpy(keys[i].value, value, sizeof(keys[i].value));
    	  return 0;
    	}
    	//printf("%s\n", keys[i].key);
    	//printf("%s\n", keys[i].value);
  	}
  	printf("err");
  	strcpy(res, "NIL");
  	return 1;*/
}

int get(char* key, char* res) {
	//printf("%lu\n", sizeof(keys)/sizeof(keys[0]));
  printf("first %s\n", key);
  for(int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
  	printf("abc %s\n", keys[i].key);
  	//printf("test %s\n", key);
    if (strncmp(keys[i].key, key, sizeof(keys[i].key)) == 0) {
    printf("gefunden");
      strcpy(res, keys[i].value);
      return 0;
    }
  }
  strcpy(res, "Key nicht vorhanden\n");
  return 1;
}

int del(char* key, char* res) {
  for(int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
    if (strncmp(keys[i].key, key, sizeof(keys[i].key)) == 0) {
      strncpy(res, keys[i].value, sizeof(keys[i].value));
      memset(&keys[i], 0, sizeof(keys[i]));
      strcpy(res, "Key wurde gelöscht");
      return 0;
    }
  }
  strcpy(res, "Löschen nicht geklappt\n");
  return 1;
}
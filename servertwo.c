#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/shm.h>

int put(char* key, char* value, char* res);
int get(char* key, char* res);
int del(char* key, char* res);

struct KeyPair {
	char key[33];
	char value[1024];
};

struct KeyPair keys[10];
const int NUM_KEYS = 1024;
int mem_id;

int main(int argc, char *argv[]) {
	int mysocket , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char client_message[2000];

	//methode um ein shared memory segment zu erstellen
 	//mem_id = shmget(IPC_PRIVATE, sizeof(struct KeyPair) * NUM_KEYS, IPC_CREAT|0777);

 	//methode, um den inhalt des shared memory segments in die keys zu laden
  	//keys = (struct KeyPair*)shmat(mem_id, 0, 0);

  	//printf("Keys: %p\n", keys);

  	//fill the shared memory values with zero
  	//memset(keys, 0, sizeof(struct KeyPair) * NUM_KEYS);

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
	listen(mysocket, 5);

	//Accept and incoming connection
	puts("Warten auf eingehende connections...");
	c = sizeof(struct sockaddr_in);

	client_sock = accept(mysocket, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
	puts("Connection akzeptiert");

	while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0) {
		//write(client_sock , client_message , strlen(client_message));
		char *buf = client_message;
		int i = 0;
	    char *p = strtok (buf, " ");
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
			get(array[3], res);
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

	return 0;
}

int put(char* key, char* value, char* res) {
	//printf("%lu\n", sizeof(keys)/sizeof(keys[0]));
	for(int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
		if (strlen(keys[i].key) == 0) {
			strncpy(keys[i].key, key, sizeof(keys[i].key));
    	  	strncpy(keys[i].value, value, sizeof(keys[i].value));
    	  	strcpy(res, "geklappt");
			return 1;
		} 
	}
	strcpy(res, "nicht geklappt");
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
  	strcpy(res, "testtest");
  	return 1 ;
}

int get(char* key, char* res) {
	//printf("%lu\n", sizeof(keys)/sizeof(keys[0]));
  printf("first %s\n", key);
  for(int i = 0; i < 10; i++) {
  	printf("abc %s\n", keys[i].key);
  	//printf("test %s\n", key);
    if (strncmp(keys[i].key, key, sizeof(keys[i].key)) == 0) {
    printf("gefunden");
      strcpy(res, keys[i].value);
      return 0;
    }
  }
  strcpy(res, "Key nicht vorhanden");
  return 1;
}

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int put(char* key, char* value, char* res);
int get(char* key, char* res);
int del(char* key, char* res);

int main(int argc, char *argv[]) {
	int socket_desc , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char client_message[2000];

	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if(socket_desc == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");
	
	//Socket adresse und port binden
	server.sin_family = AF_INET; //IPv4
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8888 );

	//Binden
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(socket_desc , 3);

	//Accept and incoming connection
	puts("Warten auf eingehende connections...");
	c = sizeof(struct sockaddr_in);

	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
	puts("Connection akzeptiert");

	while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0) {
		write(client_sock , client_message , strlen(client_message));
		//user input
		if(strncmp(client_message, "PUT", 3) == 0) {
			write(client_sock , client_message, strlen(client_message));
		}
	}

	if(read_size == 0) {
		puts("Client disconnected");
	 	// fflush(stdout);
	} else if(read_size == -1) {
		perror("recv failed");
	}

	return 0;
}

int put(char* key, char* value, char* res) {
	printf("Hallo");
	return 1;
}

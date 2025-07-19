#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>

#define PONG "+PONG\r\n"
#define MAX_SIZE 2048
#define MAX_ARGS 1024

enum Commands {
	PING, 
	ECH0, 
	UNKNOWN,
};



char * parseBulkString(char ** input , int length){

	char *str = (char*)malloc(length + 1);

	if(str == NULL){

		printf("Not able to allocate memory.\n");
		return 1; 

	}

	strncpy(str , *input , length); 

	str[length] = '\0'; 

	(*input) += 2 ; // Skip CRLF.

	return str; 

}

/**
 * 
 * One function to find length i.e to convert *2/r/n to 2. 
 * 
 * Another thing what should be the arguments. The buffer itself, so a pointer to the buffer. As we want to modify that. 
 * Why not a pointer to buffer, because that will not update the buffer in the main function itself. 
*/

int parseLen(char **input){

	printf("The parsed is of type %c \n" , **input); 

	(*input)++; // Skip the initial character. 

	int length = 0 ; 

	// Atoi type function. 

	while(**input != '\r' || **input != '\n'){

		length = length * 10 + ((**input) - '0')*10; 

		(*input)++; 

	}

	(*input) += 2; // Skip CRLF. 

	return length;




}

enum Commands parseCommand(char *bulkstr){

	if(strcasecmp(bulkstr , "echo") == 0){

		return ECHO; 

	}

	if(strcasecmp(bulkstr , "ping") == 0){

		return PING;
	}

	return UNKNOWN; 


}



void *routine(void *arg){

	int fd = *(int*)arg;
	
	char buf[MAX_SIZE]; 

	char** memoryAddress = &buf; 



	while(recv(fd , buf , MAX_SIZE , 0) != 0){

		int numArgs = parseLen(memoryAddress);

		for(int i = 0 ; i < numArgs ; i++){

			int stringlength = parseLen(memoryAddress);

			char* bulkstr = parseBulkString(memoryAddress , stringlength); 


			enum Commands command = bulkstr; 

			if(command == PING){

				send(fd , PONG , MAX_SIZE , 0); 

			}
			else{

				i++; // Skipping to next argument. 

				int currlen = parseLen(memoryAddress);
				
				char* currentArg = parseBulkString(memoryAddress , currlen); 

				char* toSend = (char*)malloc(currlen + 5); 

				sprintf(toSend , "$%d/r/n%s/r/n" , currlen , currentArg);

				send(fd , toSend , currlen , 0); 

				free(currentArg);
				free(currlen); 
			}


		}





		

	}



	close(fd); 

}


int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	
	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment this block to pass the first stage
	//
	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	//
	// // Since the tester restarts your program quite often, setting SO_REUSEADDR
	// // ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(6379),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);

	
	int client_fd = 0; 

	pthread_t offshoots[5]; 

	int thread_index = 0 ; 
	
	while( (client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) != -1 ){
	printf("Client connected\n");

	int* arg = malloc(sizeof(int)); 

	*arg = client_fd; 

	if(thread_index == 5){

		printf("Not able to connect.\n"); 
		return 3; 

	}

	if(pthread_create(&offshoots[thread_index] , NULL , &routine , arg) == -1){

		thread_index++; 
		perror("Not able to create thread.\n"); 
		return 2; 

	}


	


}

	for(int i = 0 ; i < thread_index ; i++){

		pthread_join(offshoots[i] , NULL); 

	}


	
	close(server_fd);

	return 0;
}

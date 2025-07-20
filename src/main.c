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
#include <time.h>

#define PONG "+PONG\r\n"
#define MAX_SIZE 2048
#define MAX_ARGS 1024
#define INFINITY 1000000000

enum Option{

	PX ,
	OTHER

};

enum Commands {
	PING, 
	ECHO, 
	GET , 
	SET,
	UNKNOWN,
};

typedef struct KeyValue{

	char *key ;
	char* value; 

	struct timespec currTime; 

	int expiretime; 

	struct KeyValue* next; 



} KeyValue;


enum Option currOption(char *option){


	if(strcasecmp(option, "px") == 0){

		return PX;


	}

	return UNKNOWN; 



}
double getTimeDifference(struct timespec begin){

	struct timespec curr; 

	timespec_get(&curr , TIME_UTC); 

	double diff = (curr.tv_sec - begin.tv_sec)* 1000.0 + (curr.tv_nsec - begin.tv_nsec)/1000000.0;

	return diff; 


}

KeyValue* head = NULL; 

KeyValue* initialize(char* aKey , char *aValue , int expiry){

	KeyValue* newPair = (KeyValue*)malloc(sizeof(KeyValue)); 

	newPair->key = aKey;
	newPair->value = aValue;
	timespec_get(&(newPair->currTime), TIME_UTC); 
	newPair->expiretime = expiry;
	newPair->next = NULL; 


}

void freeKeyValue(KeyValue* head){

	KeyValue* temp = head; 

	while(temp != NULL){

		temp = temp->next; 
		free(head->key);
		free(head->value);
		free(head); 
		

		head = temp; 

	}



}

KeyValue* setValue(char* aKey , char*aValue , int expiry , KeyValue* head){

	if(head == NULL){
		return initialize(aKey , aValue , expiry); 
	}

	KeyValue* temp = head; 

	while(temp->next != NULL){

		temp = temp->next;

	}

	temp->next = initialize(aKey, aValue , expiry);

	return head;

}

char* getValue(char* aKey , KeyValue* head){

	KeyValue* temp = head; 
	
	while(temp != NULL){

		if(strcmp(temp->key, aKey) == 0){

			if(getTimeDifference(temp->currTime) < temp->expiretime){

				return temp->value;
			}

		}
		temp = temp->next; 

	}

	return NULL; 

}




char * parseBulkString(unsigned char ** input , int length){

	char *str = (char*)malloc(length + 1);

	if(str == NULL){

		printf("Not able to allocate memory.\n"); 

		return NULL;

	}

	strncpy(str , *input , length); 

	str[length] = '\0'; 

	(*input) += length;

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

int parseLen(unsigned char **input){


	(*input)++; // Skip the initial character. 

	int length = 0 ; 

	// Atoi type function. 

	while(**input != '\r'){

		length = length * 10 + (**input - '0'); 

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
	if(strcasecmp(bulkstr , "get") == 0){

		return GET;
	}

	if(strcasecmp(bulkstr , "set") == 0){

		return SET;
	}

	return UNKNOWN; 


}



void *routine(void *arg){

	int fd = *(int*)arg;

	free(arg);

	while(1){
	
	unsigned char *buf = (char*)malloc(MAX_SIZE);



	unsigned char* input = buf; 

	int bytesRead ; 

	if( (bytesRead = read(fd , buf , MAX_SIZE )) <= 0){

		printf("Connection terminated.\n"); 

		free(buf);
		close(fd);
		return NULL; 

	}

	int numArgs = parseLen(&input);

	if(numArgs <= 0){

		free(buf); 
		return NULL;

	}



	

		int stringlength = parseLen(&input);

		char* bulkstr = parseBulkString(&input , stringlength); 

		if(bulkstr == NULL){
			return NULL; 
		}




		enum Commands command = parseCommand(bulkstr); 

		free(bulkstr);

		if(command == PING){

			send(fd , PONG , strlen(PONG) , 0); 

		}
		else if(command == ECHO){

			int currlen = parseLen(&input);
				
			char* currentArg = parseBulkString(&input , currlen); 

			char* toSend = (char*)malloc(currlen + sizeof(currlen) + 5); 



			sprintf(toSend , "$%d\r\n%s\r\n" , currlen, currentArg);

			

			send(fd , toSend , strlen(toSend), 0); 

			free(currentArg);
			free(toSend); 
			
		}

		else if(command == SET){

			int keyLen = parseLen(&input); 
			char* key = parseBulkString(&input , keyLen); 

			int valueLen = parseLen(&input); 

			char* value = parseBulkString(&input , valueLen);

			int expiry = INFINITY;

			printf("Bytes read %d \n", bytesRead); 

		

			if( buf + bytesRead > input){

				int optlen = parseLen(&input); 
				char* optionStr = parseBulkString(&input , optlen);

				int expirlen = parseLen(&input); 

				char* expiryStr = parseBulkString(&input, expirlen);

				enum Option option = currOption(optionStr); 
				free(optionStr);

				if(option == PX){

					expiry = atoi(expiryStr);

					free(expiryStr);

					

				}

			}

			head = setValue(key , value , expiry , head); 

			char* reply = (char*)malloc(6); 

			sprintf(reply, "+OK\r\n"); 

			send(fd , reply , strlen(reply), 0 );


			free(reply); 
			

		}

		else if(command == GET){

			int keyLen = parseLen(&input); 
			char* key = parseBulkString(&input , keyLen);

			char* value = getValue(key , head);

			if(value == NULL){

				char* reply = (char*)malloc(6);

				sprintf(reply, "$-1\r\n"); 

				send(fd, reply , strlen(reply), 0); 

				free(reply); 
				

			}
			else{
					char* reply = (char*)malloc(strlen(value) + sizeof(strlen(value)) + 5);

					sprintf(reply , "$%d\r\n%s\r\n", (int)strlen(value), value); 

					send(fd , reply , strlen(reply) , 0); 

					free(reply); 
			}


		}


	
	
	free(buf); 

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


	
	while( (client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) != -1 ){
	printf("Client connected\n");

	int* arg = malloc(sizeof(int)); 

	*arg = client_fd; 

	pthread_t offshoot;

	if(pthread_create(&offshoot , NULL , &routine , arg) == -1){


 
		perror("Not able to create thread.\n"); 
		return 2; 

	}

	pthread_detach(offshoot);


	


}


	freeKeyValue(head); 
	
	close(server_fd);

	return 0;
}

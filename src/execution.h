#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include "datastructures.h"
#define RESP_PONG "+PONG\r\n"
#define RESP_OK "+OK\r\n"
#define RESP_NULL "$-1\r\n"
#define MAX_SIZE 2048
#define MAX_ARGS 1024
#define INFINITY 1000000000



KeyValue* keyValueHead = NULL;

KeyValueList* keyValueListHead = NULL;
KeyValueList* keyValueListTail = NULL; 


char * parseBulkString( char ** input , int length){

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

int parseLen( char **input){


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

    if(strcasecmp(bulkstr , "rpush") == 0){

        return RPUSH;
    }

	return UNKNOWN; 


}



void *routine(void *arg){

	int fd = *(int*)arg;

	free(arg);

	while(1){
	
	char *buf = (char*)malloc(MAX_SIZE);



	char* input = buf; 

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

			send(fd , RESP_PONG , strlen(RESP_PONG) , 0); 

		}
		else if(command == ECHO){

			int currlen = parseLen(&input);
				
			char* currentArg = parseBulkString(&input , currlen); 

			int digits = snprintf(NULL, 0 , "%d", currlen);

			char* toSend = (char*)malloc(currlen + digits + 6); // Extra for '\0'



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

			keyValueHead = setValue(key , value , expiry , keyValueHead); 

			free(key);
			free(value);

		

			send(fd , RESP_OK , strlen(RESP_OK), 0 );


			

		}

		else if(command == GET){

			int keyLen = parseLen(&input); 
			char* key = parseBulkString(&input , keyLen);

			char* value = getValue(key , keyValueHead);

			if(value == NULL){

				send(fd, RESP_NULL, strlen(RESP_NULL), 0); 

			}
			else{
					int digits = snprintf(NULL , 0 , "%d" , (int)strlen(value)); 
					char* reply = (char*)malloc(strlen(value) + digits + 6);

					sprintf(reply , "$%d\r\n%s\r\n", (int)strlen(value), value); 

					send(fd , reply , strlen(reply) , 0); 

					free(reply); 
			}

			free(key); 





		}
        else if(command == RPUSH){

            int keyLen = parseLen(&input); 
            char* key = parseBulkString(&input , keyLen);

            int valueLen = parseLen(&input);

            char* value = parseBulkString(&input , valueLen);

            int numEl = insertKeyList(keyValueListHead, key , value , keyValueListTail); 

            int digit = snprintf(NULL , 0 , "%d" , numEl);

            char* tosend = (char*)malloc(digit + 4); 

            sprintf(tosend , ":%d\r\n" , numEl);

            send(fd, tosend , strlen(tosend) , 0);





        }


	
	
	free(buf); 

	}






	close(fd); 

}
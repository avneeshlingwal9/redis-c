
#include <pthread.h>
#include <sys/socket.h>
#include "parser.h"


pthread_mutex_t mutex; 

KeyValue* keyValueHead = NULL;

KeyValueList* keyValueListHead = NULL;
KeyValueList* keyValueListTail = NULL; 




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

			pthread_mutex_lock(&mutex); 

			keyValueHead = setValue(key , value , expiry , keyValueHead); 

			pthread_mutex_unlock(&mutex); 

			free(key);
			free(value);

		

			send(fd , RESP_OK , strlen(RESP_OK), 0 );


			

		}

		else if(command == GET){

			int keyLen = parseLen(&input); 

			char* key = parseBulkString(&input , keyLen);

			pthread_mutex_lock(&mutex);

			char* value = getValue(key , keyValueHead);

			pthread_mutex_unlock(&mutex); 

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

			int numOfValues = numArgs - 2 ; 

			int numEl = 0;

			for(int i = 0 ; i < numOfValues ; i++){

				int valueLen = parseLen(&input);

				char* value = parseBulkString(&input , valueLen);

				pthread_mutex_lock(&mutex);

				numEl = insertKeyList(&keyValueListHead, key , value , &keyValueListTail); 

				pthread_mutex_unlock(&mutex); 

				free(value);


			}

            int digit = snprintf(NULL , 0 , "%d" , numEl);



            char* tosend = (char*)malloc(digit + 4); 

			

            sprintf(tosend , ":%d\r\n" , numEl);

            send(fd, tosend , strlen(tosend) , 0);

			free(key);


			free(tosend); 





        }


		else if(command == LRANGE){

			int keylen = parseLen(&input);
			char* key = parseBulkString(&input , keylen);

			int startLen = parseLen(&input);

			char* startStr = parseBulkString(&input , startLen);

			int endLen = parseLen(&input);

			char* endStr = parseBulkString(&input , endLen);

			int start = atoi(startStr);

			int end = atoi(endStr); 

			int numberOfElements = 0; 

			char** values = getElements(keyValueListHead, key , start , end, &numberOfElements); 

			if(values == NULL){

				send(fd , RESP_NULL_ARRAY , strlen(RESP_NULL_ARRAY) , 0); 


			}
			else{

			char* tosend = encodeArray(values , numberOfElements); 

			send(fd , tosend , strlen(tosend), 0); 
			
			free(tosend); 

			free(values);

		}


			free(key);
			free(startStr);

			free(endStr);









		}
	
	free(buf); 

	}






	close(fd); 

}
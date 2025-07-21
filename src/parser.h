
#include "datastructures.h"
#define RESP_PONG "+PONG\r\n"
#define RESP_OK "+OK\r\n"
#define RESP_NULL "$-1\r\n"
#define MAX_SIZE 2048
#define MAX_ARGS 1024
#define INFINITY 1000000000
#define RESP_NULL_ARRAY "*0\r\n"

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
	if(strcasecmp(bulkstr , "lrange") == 0){

		return LRANGE;

	}
    if(strcasecmp(bulkstr, "lpush") == 0){

        return LPUSH;

    }
    if(strcasecmp(bulkstr , "llen") == 0){

        return LLEN;

    }
    if(strcasecmp(bulkstr , "lpop") == 0){

        return LPOP;
    }

	return UNKNOWN; 


}

char* encodeStr(char *str){

    int numDigits = snprintf(NULL , 0, "%d" , (int)strlen(str));

    char* encodedStr = (char*)malloc(strlen(str) + 6 + numDigits);

    sprintf(encodedStr,"$%d\r\n%s\r\n", (int)strlen(str) ,str);

    return encodedStr;
    

}
char* encodeArray(char** values , int numEl){

    

    char *encodedArr = (char*)malloc(MAX_SIZE); 

    int countEl = 0; 

    sprintf(encodedArr, "*%d\r\n", numEl);

    countEl += strlen(encodedArr);

    for(int i = 0 ; i < numEl; i++){

        char* encodedStr = encodeStr(values[i]);

        countEl += strlen(encodedStr);

        strcat(encodedArr , encodedStr);

        free(encodedStr);

        free(values[i]);


    }

    encodedArr = (char*)realloc((void*)encodedArr , countEl);


    return encodedArr;

}
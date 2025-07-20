#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
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
    RPUSH,
};
typedef struct Node{

	char* value ; 

	struct Node* next; 


}Node;


typedef struct KeyValueList{

	char* key;

	Node* head; 

	Node* tail; 

    struct KeyValueList* next; 

    int numOfElement; 


}KeyValueList;

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

	return OTHER; 



}

double getTimeDifference(struct timespec begin){

	struct timespec curr; 

	timespec_get(&curr , TIME_UTC); 

	double diff = (curr.tv_sec - begin.tv_sec)* 1000.0 + (curr.tv_nsec - begin.tv_nsec)/1000000.0;

	return diff; 


}



KeyValue* initialize(char* aKey , char *aValue , int expiry){

	KeyValue* newPair = (KeyValue*)malloc(sizeof(KeyValue)); 

	newPair->key = strdup(aKey);
	newPair->value = strdup(aValue);
	timespec_get(&(newPair->currTime), TIME_UTC); 
	newPair->expiretime = expiry;
	newPair->next = NULL; 

	return newPair; 


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


KeyValueList* getKeyValueList(KeyValueList* head , char* key){

    KeyValueList* temp = head; 

    while(temp != NULL){

        if(strcmp(temp->key , key) == 0){

            return temp;

        }

        temp = temp->next;

    }

    return NULL; 



}

void insertValue(KeyValueList* curr , char* value){

	curr->numOfElement++; 

    Node* node = (Node*)malloc(sizeof(Node));
    node->value = strdup(value); 

    if(curr->head == NULL){

        curr->head = node;
        curr->tail = node; 

    }
    else{

        curr->tail->next = node;

    }

    curr->tail = curr->tail->next; 




}


int insertKeyList(KeyValueList* head , char* key , char* value , KeyValueList* tail){

 

    KeyValueList* keyList = getKeyValueList(head , key); 

    if(keyList != NULL){

        insertValue(keyList , value);

        return keyList->numOfElement;
        

    }

    else{

        KeyValueList* newKey = (KeyValueList*)malloc(sizeof(KeyValueList));

        newKey->key = key; 
        newKey->numOfElement = 0 ; 
        newKey->head = NULL; 
        newKey->tail = NULL;
        newKey->next = NULL; 


        insertValue(newKey , key); 


        if(head == NULL){

            head = newKey;
            tail = newKey;

        }

        else{

            tail->next = newKey; 

        }

        tail = tail->next; 

        return newKey->numOfElement;
        

    }

    return -1; 

    


}


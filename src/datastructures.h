#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
enum Option{

	PX ,
	OTHER

};

typedef enum Commands {
	PING, 
	ECHO, 
	GET , 
	SET,
	UNKNOWN,
    RPUSH,
	LRANGE,
	LPUSH,
	LLEN,
	LPOP,
}Command;
typedef struct Node{

	char* value ; 

	struct Node* nextNode; 


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

	if(newPair == NULL){

		return NULL; 
	}

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

void insertValue(KeyValueList* curr , char* value , Command command){

	curr->numOfElement++; 

    Node* node = (Node*)malloc(sizeof(Node));

	if(node == NULL){
		return; 
	}
    node->value = strdup(value); 

    if(curr->head == NULL){

        curr->head = node;
		curr->tail = node;

		return;

    }

	if(command == LPUSH){

		// Insert at head. 

		node->nextNode = curr->head; 

		curr->head = node; 



	}

	else{

		curr->tail->nextNode = node;

		curr->tail = node; 

	}









}

Node* removeFromLeft(Node** head){

	Node* temp = *head; 

	*head = temp->nextNode; 


	temp->nextNode = NULL;

	return temp;



}



int insertKeyList(KeyValueList** head , char* key , char* value , KeyValueList** tail , Command command){

 

    KeyValueList* keyList = getKeyValueList(*head , key); 

    if(keyList != NULL){

        insertValue(keyList , value, command);

        return keyList->numOfElement;
        

    }

    else{



        KeyValueList* newKey = (KeyValueList*)malloc(sizeof(KeyValueList));

		if(newKey == NULL){

			return -1; 
		}

        newKey->key = strdup(key); 
        newKey->numOfElement = 0 ; 
        newKey->head = NULL; 
        newKey->tail = NULL;
        newKey->next = NULL; 


        insertValue(newKey , value, command); 




        if(*head == NULL){

            *head = newKey;


        }

        else{

            (*tail)->next = newKey; 

        }

        *tail = newKey; 

        return newKey->numOfElement;
        

    }

    return -1; 

    


}


char **getElements(KeyValueList* keyValueListHead , char* key , int start , int end , int *numberOfElements){

		KeyValueList* desiredKey = getKeyValueList(keyValueListHead , key);
		if(start < 0){

		if(abs(start) >= desiredKey->numOfElement){

			start = 0; 

		}
		else{

		start = desiredKey->numOfElement + start; 
	}

	}

	if(end < 0){

		if(abs(end) >= desiredKey->numOfElement){
			end = 0 ;
		}
		else{

			end = desiredKey->numOfElement + end ; 

		}


	}




	if(desiredKey == NULL || (start >= desiredKey->numOfElement) || (start > end)){

		return NULL; 
	}






	if(end >= desiredKey->numOfElement){

		end = desiredKey->numOfElement - 1;

	}
	int elementsCount = end - start + 1; 

	*numberOfElements = elementsCount;

	char ** array = (char**)malloc(sizeof(char*)* elementsCount);

	if(array == NULL){

		return NULL; 
	}

	int i = 0 ; 

	Node* temp = desiredKey->head;

	// Reach till start index. 

	while(i < start){

		temp = temp->nextNode;
		i++;
	}

	while(i <= end){

		array[i - start] = strdup(temp->value);
		i++;
		temp = temp->nextNode; 

	}









	return array; 

}

void freeNode(Node* nodeHead){

	Node* tempHead = nodeHead; 

	while(tempHead != NULL){

		tempHead = tempHead->nextNode;

		free(nodeHead->value);

		free(nodeHead);

		nodeHead = tempHead;

	}


}

void freeKeyValueList(KeyValueList* keyHead){


	KeyValueList* temp = keyHead; 

	while(temp != NULL){

		temp = temp->next;

		freeNode(keyHead->head);
		free(keyHead->key);
		free(keyHead);

		keyHead = temp; 

	}







}

int getCount(KeyValueList* head , char* key){

	KeyValueList* desired = getKeyValueList(head, key); 

	return desired == NULL ? 0 : desired->numOfElement; 

}

char* lpop(KeyValueList* keyHead , char* key){

	KeyValueList* desired = getKeyValueList(keyHead , key);

	if(desired == NULL){

		return NULL;
	}

	Node* frontNode = removeFromLeft(&desired->head);

	desired->numOfElement--;

	char* toReturn = frontNode->value;
	
	free(frontNode);

	return toReturn;



}


char** lpopMultiple(KeyValueList* listHead , char* key , int len){

	KeyValueList* desired = getKeyValueList(listHead , key); 


	if(desired == NULL){

		return NULL;

	}

	if(len > desired->numOfElement){

		len = desired->numOfElement;

	}

	char** arr = (char**)malloc(len); 

	Node* temp = desired->head;
	Node* prev = temp; 

	for(int i = 0 ; i < len ; i++){

		arr[i] = temp->value;

		temp = temp->nextNode; 

		prev->nextNode = NULL;
		
		free(prev); 

		prev = temp; 

		desired->numOfElement--;









	}

	desired->head = prev; 

	return arr; 







}
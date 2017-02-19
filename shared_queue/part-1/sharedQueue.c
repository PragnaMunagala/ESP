#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include"sharedQueue.h"

#define queue_length 10

/* creation of shared queue */
sharedQueue* sq_create(){
	sharedQueue *SQ = (sharedQueue *)malloc(sizeof(sharedQueue));
	SQ->elements = malloc(sizeof(struct messages)*queue_length);
	SQ->front = -1;
	SQ->rear = -1;
	SQ->size = 0;
	return SQ;
}

/* enqueue operaion */
int sq_write(sharedQueue *Q, int msgId, int sId, int rId, char *sendermsg)
{
	time_t t;
	time(&t);
	/* to check if queue is full */
	if(Q->size == queue_length){
		return -1;
	}
	else{		
		Q->size++;
		Q->rear++;
		/* As queue is filled in circular manner */
		if(Q->rear == queue_length)
			Q->rear = 0;

		/* to load the element into the queue */
		strcpy(Q->elements[Q->rear].message,sendermsg);
		Q->elements[Q->rear].messageId = msgId;
		Q->elements[Q->rear].sourceId = sId;
		Q->elements[Q->rear].destinationId = rId;			
		return Q->rear;
	}	
}

/* dequeue operaion */
int sq_read(sharedQueue *Q){
	/* to check if queue is empty */
	if(Q->size == 0){
		return -1;
	}
	else{
		Q->size--;
		Q->front++;
		/* As queue operates in circular manner */
		if(Q->front == queue_length)
			Q->front = 0;		
		return Q->front;
	}
}

/* to delete shared queue */
void sq_delete(sharedQueue *Q){
	free(Q);
}




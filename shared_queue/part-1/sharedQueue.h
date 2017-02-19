/* definition of queue structure */
typedef struct{
	int length;
	int rear;
	int front;
	int size;
	
	struct messages{
		int messageId;
		int sourceId;
		int destinationId;
		char message[80];
		float elapseTime;
		
	}message_data;

	struct messages *elements;

}sharedQueue;

/* methods for queue operations */
sharedQueue* sq_create();
int sq_write(sharedQueue *, int, int, int, char *);
int sq_read(sharedQueue *);
void sq_delete(sharedQueue *);






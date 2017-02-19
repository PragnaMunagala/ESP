#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/types.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <linux/input.h>

/*********************************************** Signal handler function ***********************************************/
void handler(int signal_number){
	printf("Thread Id of signal handled = %lu\n", pthread_self());         //To display the thread id of the arbitrary thread which receives the signal
}

/*********************************************** Thread function ***********************************************/
void* thread_function(void *ptr){
	while(1);            //While loop for continuos program running
	return NULL;
}

/*********************************************** Main program ***********************************************/
int main(){	
	int i;
	pthread_t thread[2];
	
	/* Creating variable and assigning handler to SIGIO signal */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &handler;
	sigaction(SIGIO, &sa, NULL);
	
	/* creating five threads */	
	for(i = 0; i < 5; i++)
		pthread_create(&thread[i], NULL, thread_function, NULL);
	
	/* pthread join for main program to wait till thread function is executed */
	for(i = 0; i < 5; i++)
		pthread_join(thread[i], NULL);
		
	return 0;
}

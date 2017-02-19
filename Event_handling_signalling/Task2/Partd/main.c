#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/types.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <linux/input.h>

struct timespec ts;
pthread_t thread;

/* Signal handler function */
void handler(int signal_number){
	printf("User 1 Signal handled\n");	//printf to indicate signal handled
	exit(1);		//to terminate the program once the signal is handled
}

/* Thread function */
void* thread_function(void *ptr){
	while(1){
		pthread_kill(thread, SIGUSR1);		//to generate the signal to the runnable thread
		nanosleep(&ts, NULL);			//to delay the thread
	}
	return NULL;
}

/* Main program */
int main(){
	/* Creating variable and assigning handler to SIGIO signal */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &handler;
	sigaction(SIGUSR1, &sa, NULL);
	
	/* To initialize the timespec variable */ 
	ts.tv_sec = 0;
    	ts.tv_nsec = 100000000;
    	
    	/* creating a thread */
	pthread_create(&thread, NULL, thread_function, NULL);
	
	/* joining thread */
	pthread_join(thread, NULL);
	return 0;
}

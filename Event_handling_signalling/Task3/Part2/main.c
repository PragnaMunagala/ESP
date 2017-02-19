#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/types.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <linux/input.h>
#include "sig_lib.h"

pthread_mutex_t thread_lock, temp;
int i = 0;          //counter to register even numbered threads out of five generated threads


/********************************** Signal handler function ************************************/
void handler(int signal_number){	
		printf("Signal handled Thread Id = %lu\n", pthread_self());
		
		/* to call function specific to thread calling signal handler */
		pthread_mutex_lock(&temp);
		
		/* function to deliver SIGIO to all registered threads */
		handle_threads();
		
		sleep(1);       //to see the output properly
		pthread_mutex_unlock(&temp);
}

/************************************** Thread function ***************************************/
void* thread_function(void *ptr){
	int fd, check;
	struct input_event event;
	
	/* lock for the i variable to be thread synchronized */	
	pthread_mutex_lock(&thread_lock);
	
	/* calling library function to register the even numbered threads to receive SIGIO */
	if(i == 0 || i == 2 || i == 4)
		register_threads(pthread_self());
	i++;
	pthread_mutex_unlock(&thread_lock);

	/* To open the mouse device file */
	fd = open("/dev/input/event6", O_RDONLY);
	if (fd == -1) {
		printf("Cannot open device file.\n");	
	}else{
		while(1){
			check = read(fd, &event, sizeof(struct input_event));                    //To read the input event
			if(check != -1){
				if(event.type == EV_KEY && event.code == BTN_LEFT && event.value == 0){            //To detect the click of mouse left button
					kill(0, SIGIO);				//To deliver the SIGIO signal 
					
				}
			}		
		}
	}
	return NULL;
}

/************************************* Main program ***************************************/
int main(){
	int i;
	
	pthread_t 	thread[5];
	
	/* Creating variable and assigning handler to SIGIO signal */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &handler;
	sigaction(SIGIO, &sa, NULL);
	
	/* Initialization of lock */
	pthread_mutex_init(&thread_lock, NULL);
	pthread_mutex_init(&temp, NULL);
	
	/* creating five threads */
	for(i = 0; i <5; i++)
		pthread_create(&thread[i], NULL, thread_function, NULL);
	
	/* pthread join for main program to wait till thread function is executed */	
	for(i = 0; i < 5; i++)
		pthread_join(thread[i], NULL);
	
	/* To destroy the lock */ 
	pthread_mutex_destroy(&temp);
	pthread_mutex_destroy(&thread_lock);
	return 0;
}

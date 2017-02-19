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
#include <sched.h>

pthread_t thread[2];
/* To assign priorities to threads */
pthread_attr_t tattr[2];
struct sched_param param[2];
int priority[2] = {90, 91};

/* Signal handler function */
void handler(int signal_number){
	printf("SIGIO Signal handled\n");	//printf to indicate signal handled
	exit(1);          //to terminate the program once the signal is handled
}

/* High Priority Thread function */
void* thread_function1(void *ptr){
	while(1){
		pthread_kill(thread[1], SIGUSR1);	//to generate the signal to the runnable thread
	}
	return NULL;
}

/* Low Priority Thread function */
void* thread_function2(void *ptr){
	while(1);
}


/* Main program */
int main(){
	int i;
	
	/* Creating variable and assigning handler to SIGIO signal */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &handler;
	sigaction(SIGUSR1, &sa, NULL);
	
	for(i = 0; i < 2; i++){
		/* initialized with default attributes */
		pthread_attr_init (&tattr[i]);
		
		/* safe to get existing scheduling param */
		pthread_attr_getschedparam (&tattr[i], &param[i]);
		
		/* set the priority */
		param[i].sched_priority = priority[i];
		
		/* setting the new scheduling param */
		pthread_attr_setschedparam (&tattr[i], &param[i]);
	}

	/* creating a thread */	
	pthread_create(&thread[0], &tattr[0], thread_function1, NULL);
	pthread_create(&thread[1], &tattr[1], thread_function2, NULL);
	
	/* thread join for main program to wait till thread execution is done */	
	for(i = 0; i < 2; i++)
		pthread_join(thread[i], NULL);
	
	return 0;
}

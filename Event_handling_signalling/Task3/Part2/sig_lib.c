#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "sig_lib.h"

unsigned long int thread_ids[3];    //to hold the registerd threads id's
int count=0;

/* To deliver SIGIO to all registered threads */
void handle_threads(){
	int i;	
	for(i = 0; i < 3; i++){		
		printf("Registered thread id for which SIGIO is being delivered  = %lu\n",thread_ids[i]);
		pthread_kill(thread_ids[i], SIGIO);		
	}
}

/* To register the threads which receive SIGIO */
void register_threads(unsigned long int pid){
	thread_ids[count] = pid;
	count++;
}

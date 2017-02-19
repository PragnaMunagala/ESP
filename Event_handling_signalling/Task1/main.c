#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/types.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <linux/input.h>

jmp_buf jumper;		//buffer to hold the values for setjmp, longjmp
pthread_t input_thread;		//thread to capture mouse double click event

/**************************************** Function to calculate time stamp counter ****************************************/
static __inline__ unsigned long long rdtsc(void)
{
     unsigned a, d;
     asm volatile("rdtsc" : "=a" (a), "=d" (d));
     return (((unsigned long long)a) | (((unsigned long long)d) << 32));
}

/**************************************** Signal handler function ****************************************/
void handler(int signal_number){
	longjmp(jumper, -1);
}

/**************************************** Thread function for mouse click input event ****************************************/
void* input_function(void *ptr){
	int fd, check, click_event = 0;
	struct input_event event;
	
	/* To hold the time stamp of the mouse click */
	unsigned long long clickTime = 0;
	
	/* To open the mouse device file */
	fd = open("/dev/input/event6", O_RDONLY);
	if (fd == -1) {
		printf("Cannot open device file.\n");	
	}
	else{
		while(1){
			check = read(fd, &event, sizeof(struct input_event));                    //To read the input event
			if(check != -1){
				if(event.type == EV_KEY && event.code == BTN_RIGHT && event.value == 0){	//To check if the event is of right button click
					if(click_event == 0){ 	                        //check for first or double click
						clickTime = rdtsc();
						click_event = 1;
					}else{	
						click_event = 0;
						if((rdtsc() - clickTime)/400000000 <= 1000){			//condition for duration of two right click events
							clickTime = rdtsc();
							pthread_kill(input_thread, SIGUSR1);			//generating the signal
						}else{
							printf("Mouse double click is not detected");
						}
					}	
				}
			}
		}
	}
	return NULL;	
}

/**************************************** Main program ****************************************/
int main(){
	int n1 = 0, sum = 0, check;
	struct sigaction sa;
	
	/* creating sigaction */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &handler;
	sigaction(SIGUSR1, &sa, NULL);
	
	/* creating thread to capture the mouse click event */
	pthread_create(&input_thread, NULL, input_function, NULL);
	
	/* setjmp and performing imprecise computation of finding sum of first n even numbers */
	check = setjmp(jumper);
	if(check == 0){
		while(1){
			/* To calculate the sum of first n even numbers */
			sum = sum + n1;
			n1 += 2;
			if(sum > 2147483645){        //overflow check for sum which is of integer datatype
				sum = 0;
				n1 = 0;
			}				
			sleep(1);
		}	
	}
	/* printing the result when the signal is handled for double click event */
	printf("Sum of first %d even numbers = %d\n", n1/2,sum);
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define PATTERN_WRITE 0x0000

pthread_mutex_t lock;
pthread_t distance_thread, display_thread;
int led_fd, pulse_fd;
long double current_distance = 0.0, previous_distance = 0.0;
long ioctl(int, unsigned int, unsigned long);

/***************************** Thread function to measure distance from HC-SR04 sensor *******************************/
void* calculateDistance(void *ptr){
	char *buf;
	long long width = -1;
	int *p = NULL;
	long double pulse_width;
	/* To open the device driver for pulse measurement */
	pulse_fd = open("/dev/pulse_dev", O_RDWR);
	if (pulse_fd < 0 ) {
		printf("Can not open pulse device file.\n");		
	}
	
	buf = malloc(10*sizeof(char));
	strcpy(buf,"123456789");
	
	while(1){
		pthread_mutex_lock(&lock);
		
		/* To generate the trigger input for sensor */
		write(pulse_fd, buf, 10);
		
		width = -1;
		
		/* To read the value of pulse width of echo signal */
		while(width == -1){
			width = read(pulse_fd, (char *)p, sizeof(long long));
		}
		pthread_mutex_unlock(&lock);
		pulse_width = width / (long double)1000000;
		pthread_mutex_lock(&lock);
		
		/* To calculate the distance sensed in cm */
		current_distance = (pulse_width*((long double)17000));
		printf("distance in cm = %Lf\n", current_distance);
		pthread_mutex_unlock(&lock);
		usleep(100000);	
	}	
}

/****************************** Thread function to control display patterns on LED Matrix ******************************/
void* controlDisplay(void *ptr)
{
	int row, col;
	unsigned int **pattern_list = malloc(10*sizeof(unsigned int *));
	
	int i;
	unsigned long q;
	unsigned int pattern = PATTERN_WRITE;
	
	/* Buffer to hold the sequence of patterns to be displayed on LED Matrix */
	int sequence_buf[6] = {0};
	int right_pattern[6] = {4, 100, 1, 200, 0, 0};
	int left_pattern[6] = {7, 100, 3, 200, 0, 0};
	
	
	for(row = 0; row < 10; row++)
	{
		pattern_list[row] = malloc(8*sizeof(unsigned int));
	}
	
	/* Temporary pattern list */
	unsigned int pattern_list_temp[10][8] = {{0x00, 0x00, 0x04, 0x02, 0xFF, 0x02, 0x04, 0x00},
					    {0x10, 0x10, 0x10, 0x10, 0x10, 0x54, 0x38, 0x10},
					    {0x00, 0x00, 0x20, 0x40, 0xFF, 0x40, 0x20, 0x00},
					    {0x10, 0x38, 0x54, 0x10, 0x10, 0x10, 0x10, 0x10},
					    {0x80, 0x40, 0x20, 0x10, 0x08, 0x05, 0x03, 0x07},
					    {0x01, 0x02, 0x04, 0x08, 0x10, 0xA0, 0xC0, 0xE0},
					    {0xE0, 0xC0, 0xA0, 0x10, 0x08, 0x04, 0x02, 0x01},
					    {0x07, 0x03, 0x05, 0x08, 0x10, 0x20, 0x40, 0x80},
					    {0x10, 0x38, 0x54, 0x10, 0x10, 0x54, 0x38, 0x10},
					    {0x00, 0x00, 0x24, 0x42, 0xFF, 0x42, 0x24, 0x00}};
	
	
	/* To create the 10 pattern array to be written into kernel space buffer */
	for(row = 0; row < 10; row++)
		for(col = 0; col < 8; col++)
			pattern_list[row][col] = pattern_list_temp[row][col];
		
	/* To open the device driver for pattern display */
	led_fd = open("/dev/spidev1.0", O_RDWR);
	if (led_fd < 0 ) {
		printf("Can not open LED device file.\n");		
	}	
	
	q = (unsigned long)pattern_list;    /* To hold the address of pattern list */
	
	/* To send the patterns to kernel space buffer */
	ioctl(led_fd, pattern, q);
	
	/* To display the patterns and change their direction based on the distance measured */
	while(1){
		if(current_distance <= previous_distance){                      /* to move patterns to right */
			for(i = 0; i < 6; i++){
				sequence_buf[i] = right_pattern[i];
			}
		}else if(current_distance > previous_distance){			/* to move patterns to left */
			for(i = 0; i < 6; i++)
				sequence_buf[i] = left_pattern[i];
		}
		/* To write the pattern sequence to SPI */
		write(led_fd, (char *)sequence_buf, (sizeof(int))*6);
		usleep(1000);
		pthread_mutex_lock(&lock);
		previous_distance = current_distance;
		pthread_mutex_unlock(&lock);
		usleep(100000);
	}
}

/* signal handler for SIGINT */
void sigint_handler(int sig)
{
    printf("killing process %d\n",getpid());
    exit(0);
}

/****************************** main program ******************************/
int main(){   
        
        signal(SIGINT, sigint_handler);
	/* initializing the mutex lock */
	pthread_mutex_init(&lock, NULL);

	/* Creation of pthread for LED display pattern */
	pthread_create(&display_thread, NULL, controlDisplay, NULL);

	/* Creation of pthread for pulse measurement */
	pthread_create(&distance_thread, NULL, calculateDistance, NULL);

	/* To perform join of the threads */
	pthread_join(display_thread, NULL);
	pthread_join(distance_thread, NULL);

	/* To destroy the mutex lock */
	pthread_mutex_destroy(&lock);        
        	
	return 0;
}

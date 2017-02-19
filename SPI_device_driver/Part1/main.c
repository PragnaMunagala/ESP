#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include "spi_gpio_module.h"

#define ARRAY_SIZE(array) sizeof(array)/sizeof(array[0])       //to hold the length attribute of spi_ioc_transfer
#define SPI_MODE SPI_MODE_0           //SPI_MODE_0 for write mode of SPI bus
#define MSB_FIRST 0		//to enable MSB_FIRST mode

pthread_mutex_t lock;
pthread_t distance_thread, display_thread;
int spi_dev;
long double pulse_width, current_distance, previous_distance = 0.0;

/* dog walking and running patterns for LED Matrix */
uint8_t dog_right_walk[8] = {0x08, 0x90, 0xF0, 0x10, 0x10, 0x37, 0xDF, 0x98};
uint8_t	dog_right_run[8] = {0x20, 0x10, 0x70, 0xD0, 0x10, 0x97, 0xFF, 0x18};
uint8_t	dog_left_walk[8] = {0x18, 0xDF, 0xB7, 0x10, 0x10, 0xF0, 0x90, 0x08};
uint8_t	dog_left_run[8] = {0x18, 0xFF, 0x97, 0x10, 0xD0, 0x70, 0x10, 0x20};

/* row addresses of LED Matrix */
uint8_t	led_row_address[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/* Control register values for LED Matrix */
uint8_t	control_reg_address[8] = {0x09, 0x00, 0x0A, 0x00, 0x0B, 0x07, 0x0C, 0x01};

static uint8_t mode = SPI_MODE;
static uint8_t lsb = MSB_FIRST;
static uint8_t bits = 8;
static uint32_t speed = 50000;

char buf[256];

/* structure variable for poll function */
struct pollfd fdp;

/* To hold the start and end times of the echo pulse */
unsigned long long startTime, endTime;

/* Function to calculate time stamp counter */
static __inline__ unsigned long long rdtsc(void)
{
     unsigned a, d;
     asm volatile("rdtsc" : "=a" (a), "=d" (d));

     return (((unsigned long long)a) | (((unsigned long long)d) << 32));
}

/* To open the spi bus before start of data transfer */
void spidevSet(){
	spi_dev = open("/dev/spidev1.0",O_WRONLY);
	if(spi_dev < 0) {
		printf("not able to open spidev1.0");
	}	
}

/***************************** Thread function to measure distance from HC-SR04 sensor ******************************/
void* calculateDistance(void *ptr)
{
	int value;
	sprintf(buf, "/sys/class/gpio/gpio%d/value", 14);
	value = open(buf, O_RDWR);  
	fdp.fd = value;
	fdp.events = POLLPRI|POLLERR;
	
	while(1){
		gpioSetEdge(14,"rising");      //to set the rising edge of echo signal
		lseek(fdp.fd, 0, SEEK_SET); 
		read(fdp.fd, buf, 256);
	
		/* To generate trigger pulse to sensor */
		gpioSet(13,1);
		usleep(10);
		gpioSet(13,0);
		
		fdp.revents = 0;
		if(poll(&fdp, 1, 50) > 0){         //to detect the rising edge of echo signal
		
			//to hold the time at which rising echo signal is detected
			startTime = rdtsc();
			lseek(fdp.fd, 0, SEEK_SET); 
			
			//to clear the flag
			read(fdp.fd, buf, 256);
			if(fdp.revents & POLLPRI)   
			{
				gpioSetEdge(14,"falling");         //to set the falling edge of echo signal
				
				if(poll(&fdp, 1, 50) > 0) 	   //to detect the falling edge of echo signal
				{
					//to hold the time at which rising echo signal is detected
					endTime = rdtsc();					
					if(fdp.revents & POLLPRI)
					{
						pthread_mutex_lock(&lock);
						pulse_width = (endTime-startTime)/((long double)400000000);            //to calculate the pulse width of echo signal			
						current_distance = (pulse_width*((long double)17000));                 //to calculate the distance sensed
						printf("distance in cm = %Lf\n", current_distance);
						pthread_mutex_unlock(&lock);
					}			
				}
			}	
		}
		usleep(100000);
	}
}

/****************************** Thread function to control display patterns on LED Matrix ******************************/
void* controlDisplay(void *ptr)
{
	int ret, i = 0;
	//unsigned long spi_speed;
	unsigned char data_out[2]={0};
	unsigned char data_in[2]={0};
	
	gpioSet(15,1);
	
	/* For setting SPI1_SS, IO12 */	
	spidevSet();
	
	//ret = ioctl(spi_dev, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
	
	/* To enable the WR_MODE for spi bus */
	ret = ioctl(spi_dev, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		printf("can't set spi mode");
	
	/* To enable the MSB_FIRST for spi bus */
	ret = ioctl(spi_dev, SPI_IOC_WR_LSB_FIRST, &lsb);
	if (ret == -1)
		printf("can't set lsb");
	
	/* To enable the BITS_PER_WORD for spi transfer */
	ret = ioctl(spi_dev, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("can't set bits per word");
	
	/* To enable the MAX_SPEED for spi transfer */	
	ret = ioctl(spi_dev, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("can't set max speed hz");
	
	/* structure for spi transfer */
	struct spi_ioc_transfer data = {
		.tx_buf = (unsigned long)data_out,
		.rx_buf = (unsigned long)data_in,
		.len = ARRAY_SIZE(data_out),
		.speed_hz = 500000,
		.cs_change = 1,
		.delay_usecs = 0,
		.bits_per_word = 8,
	};
	
	/* To write values to control register */
	for(i = 0; i < 8; i++){
		data_out[0] = control_reg_address[i];
		data_out[1] = control_reg_address[i+1];
		gpioSet(15,0);
		ioctl(spi_dev, SPI_IOC_MESSAGE(1), &data);	
		gpioSet(15,1);
		i++;
	}
	
	while(1){
		if(current_distance <= previous_distance){
			/* writing LED pattern of dog walking towards right */
			for(i = 0 ;i < 8; i++){
				data_out[0] = led_row_address[i];
				data_out[1] = dog_right_walk[i];
				gpioSet(15,0);
				ioctl(spi_dev, SPI_IOC_MESSAGE(1), &data);	
				gpioSet(15,1);					
			}
			usleep(((int)current_distance)*10000);
			/* writing LED pattern of dog running towards right */
			for(i = 0 ;i < 8; i++){
				data_out[0] = led_row_address[i];
				data_out[1] = dog_right_run[i];
				gpioSet(15,0);
				ioctl(spi_dev, SPI_IOC_MESSAGE(1), &data);	
				gpioSet(15,1);					
			}
		}else if(current_distance > previous_distance){
			/* writing LED pattern of dog walking towards left */
			for(i = 0 ;i < 8; i++){
				data_out[0] = led_row_address[i];
				data_out[1] = dog_left_walk[i];
				gpioSet(15,0);
				ioctl(spi_dev, SPI_IOC_MESSAGE(1), &data);	
				gpioSet(15,1);					
			}
			usleep(((int)current_distance)*10000);
			/* writing LED pattern of dog running towards left */
			for(i = 0 ;i < 8; i++){
				data_out[0] = led_row_address[i];
				data_out[1] = dog_left_run[i];
				gpioSet(15,0);
				ioctl(spi_dev, SPI_IOC_MESSAGE(1), &data);	
				gpioSet(15,1);					
			}
		}
		pthread_mutex_lock(&lock);
		previous_distance = current_distance;
		pthread_mutex_unlock(&lock);
		usleep(100000);
	}	
}

/****************************** main program ******************************/
int main(){
	
	/* initializing the mutex lock */
	pthread_mutex_init(&lock, NULL);
	
	/* To initialize the IO pins */
	IOinit();
	
	/* Creation of pthread for LED display pattern */
	pthread_create(&display_thread, NULL, controlDisplay, NULL);
	
	/* Creation of pthread for pulse measurement */
	pthread_create(&distance_thread, NULL, calculateDistance, NULL);
	
	pthread_join(display_thread, NULL);
	pthread_join(distance_thread, NULL);
	
	/* To destroy the mutex lock */
	pthread_mutex_destroy(&lock);
	
	return 0;
}

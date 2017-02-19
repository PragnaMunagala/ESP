#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include "spi_gpio_module.h"

int fd, value;
char buf[256];

/* To export gpio pin */
void gpioExport(int gpio)
{    
	fd = open("/sys/class/gpio/export",O_WRONLY);
	sprintf(buf, "%d", gpio); 
	write(fd, buf, strlen(buf));
	close(fd);
}

/* To set direction of gpio pin */
void gpioDirection(int gpio, int direction) // direction 1 for output, 0 for input
{
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
    fd = open(buf,O_WRONLY);

    if (direction)
    {
        write(fd, "out", 3);
    }
    else
    {
        write(fd, "in", 2);
    }
    close(fd);
}

/* To set value for a gpio pin */
void gpioSet(int gpio, int value)
{		
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	fd = open(buf,O_WRONLY);
	if(value == 0)
	{
		write(fd, "0", 1);
	}
	else if(value == 1)
	{	
		write(fd, "1", 1);
	}
	close(fd);
}

/* To unexport gpio pin */
void gpioUnexport(int gpio)
{    
	fd = open("/sys/class/gpio/unexport",O_WRONLY);
	sprintf(buf, "%d", gpio); 
	write(fd, buf, strlen(buf));
	close(fd);
}


/* To set an edge on a gpio pin */
void gpioSetEdge(unsigned int gpio, char *edge)
{
	sprintf(buf, "/sys/class/gpio/gpio%d/edge", gpio);
	fd = open(buf,O_WRONLY);
	write(fd, edge, strlen(edge));
	close(fd);
}


/*For IO initialization */
void IOinit(){
		
	/*********************************** To intialize the gpio pins for sending trigger pulse to the sensor **********************************/	
	/* To set the IO2 */
	gpioExport(13);
	gpioDirection(13,1);
	
	/* For setting level shifter pin */
	gpioExport(34);
	gpioDirection(34,1);
	gpioSet(34,0);
	
	/* For setting the multiplexer */
	gpioExport(77);
	gpioSet(77,0);
	

	/* This is to intialize the gpio pins for receiving echo pulse from the sensor */
	/* To set the IO3 */
	gpioExport(14);
	gpioDirection(14,0);
	
	/* For setting Multilplexer1*/
	gpioExport(76);
	gpioSet(76,0);
	
	/* For setting Multiplexer2 */
	gpioExport(64);
	gpioSet(64,0);
	
		
	/**************************** To export and set the gpio pins required for sensor and LED screen display **********************************************/
	/* ------------- This is for setting SPI1_MOSI i.e., GPIO11 ------------- */	
	/* For setting level shifter pin */
	gpioExport(24);
	gpioDirection(24,1);
	gpioSet(24,0);
	
	/* To set the Multiplexer1 */
	gpioExport(44);
	gpioDirection(44,1);
	gpioSet(44,1);
	
	/* To set the Multiplexer2 */
	gpioExport(72);
	gpioSet(72,0);
	
	/* ------------- For setting SPI1_SCK, GPIO13 ------------- */		
	/* For setting level shifter pin */
	gpioExport(30);
	gpioDirection(30,1);
	gpioSet(30,0);

	/* To set the Multiplexer */	
	gpioExport(46);
	gpioDirection(46,1);
	gpioSet(46,1);

	/* To use IO12 as CS gpio pin */
	gpioExport(15);
	gpioDirection(15,1);
		
	/* For setting level shifter pin for chip select, IO12 */
	gpioExport(42);
	gpioDirection(42,1);
	gpioSet(42,0);
}

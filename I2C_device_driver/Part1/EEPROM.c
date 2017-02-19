#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include "EEPROM.h"
#include <time.h>
#include <stdint.h>
int fd, i2c_fd;
unsigned int page_position;    //to hold the current page position of EEPROM
char temp_page_position;

char buf[255];

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

/* To set value for output gpio pin */
void gpioSet(int gpio, int value)
{		
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	fd = open(buf,O_WRONLY);
	write(fd, "0", value);
	close(fd);
}

/* To open i2c bus and set slave address */
void i2cSet(){
	i2c_fd = open("/dev/i2c-0",O_RDWR);
	if(i2c_fd < 0) {
		printf("not able to open i2c");
	}
	ioctl(i2c_fd, I2C_SLAVE, SLAVE_ADDRESS);	
}

/* To switch ON and OFF LED */
void LED_Set(int status){    // status 1 for ON, 0 for OFF
	if(status == 1){
		gpioSet(10, 1);
	}else if(status == 0)
	{		
		gpioSet(10, 0);
	}
}

/* To set up the signals for following I2C bus operations and to initialize the current page position to 0 */
int EEPROM_init(){
	gpioExport(10);    //to set the IO10 LED
	gpioDirection(10,1);
	gpioSet(10,0);
	gpioExport(74);    //to set the IO10 LED
	gpioDirection(74,1);
	gpioSet(74,0);
	gpioExport(60);     //to set the MUX of SDA and SCL
	gpioDirection(60,1);
	gpioSet(60,0);
	i2cSet();
	page_position = 0x0000;
	temp_page_position=0x00;
	return 0;
}

/* To read a sequence of “count” pages from the EEPROM device into the user memory pointed by buf */
int EEPROM_read(void *buf, int count){
	LED_Set(1);
	int value, l = 0;
	char *address_buffer, *temp_buffer;
	temp_buffer = (char *)buf;
	
	if(page_position > 0x07FF)
		page_position = 0x0000;
		
	//to write the current page position from where data has to be read
	address_buffer = malloc(2);
	address_buffer[0] = (page_position>>8) & (0xFF);
	address_buffer[1] = page_position & 0xFF;
	value = write(i2c_fd, address_buffer, 2);
	sleep(1);
	
	//to read the sequence of count pages data from EEPROM
	value = read(i2c_fd, temp_buffer, count*PAGE_SIZE);

	while(l < count){
		page_position = page_position + 0x0040;
		l++;
	}
	
	LED_Set(0);
	
	if(value == -1){
		return -1;
	}else{
		return 0;
	}
}

/* To write a sequence of “count” pages to an EEPROM device starting from the current page position of the EEPROM */
int EEPROM_write(void *buf, int count){
	LED_Set(1);
	int value, i, j, byte_count = 0, l = 0;
	
	char *temp_buffer, *data_buffer;
	temp_buffer = (char *)buf;  
	
	data_buffer = malloc((PAGE_SIZE + 2)*sizeof(char));
	
	for(i = 2, j = 0; j < PAGE_SIZE; i++, j++){
		data_buffer[i] = temp_buffer[j]; 		 
	}	
	
	while(l < count){
		if(page_position > 0x7FFF)
			page_position = 0x0000;
		
		data_buffer[0] = (page_position>>8) & (0xFF);
		data_buffer[1] = page_position & 0xFF;	
		
		// to write one page data into EEPROM	
		value = write(i2c_fd, data_buffer, PAGE_SIZE+2);
		value = value - 2;
		
		if(value == PAGE_SIZE)
			byte_count = byte_count + value;
			
		//to update the page position after each page write
		page_position = page_position + 0x0040;
		l++;
		sleep(1);
	}
	
	LED_Set(0);

	if(byte_count != count*PAGE_SIZE){
		return -1;
	}else{
		return 0;
	}
}

/* To return current page position and set it to the new position */
int EEPROM_set(int new_position){
	page_position = new_position;
	
	//if given position is out of range
	if(new_position > 0x7FFF)
		return -1;
	return page_position;
}

/* To trigger an erase operation to the EEPROM */
int EEPROM_erase(){
	LED_Set(1);
	char *temp_buffer;
	int i, value;
	
	//to store 1's into the buffer
	temp_buffer = malloc((PAGE_SIZE)*sizeof(char));
	for(i = 0; i < PAGE_SIZE; i++){
		temp_buffer[i] = 1;
	}
	
	//to set the page position to start and write 1's into all pages of EEPROM
	page_position = 0x0000;
	value = EEPROM_write((void *)temp_buffer, PAGES);	
	LED_Set(0);
	if(value == -1){
		return -1;
	}else{
		return 0;
	}
}

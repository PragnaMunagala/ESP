#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "EEPROM.h"

/* Function to generate 64byte random string */
char *rand_string(char *rand_buff)
{
	int n, index;
	char *characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijkl";
	for (n = 0; n < PAGE_SIZE; n++) {
	    index = rand() % (strlen(characters));         //To generate random index value
	    rand_buff[n] = characters[index];
	}
    	return rand_buff;
}

int main(){
	int count, check;
	char *r_buffer,temp_buffer[PAGE_SIZE], *data_buffer;

	/* To setup the signals for i2c bus */
	check = EEPROM_init();

	/****************************  Write Operation Module Start   ******************************/
		
	printf("Enter the number of pages to be written\n");
	scanf("%d", &count);	
	if(count > PAGES){
		printf("Invalid page count");
		return 0;
	}
	
	/* To allocate the memory and generate random string of 64byte */
	data_buffer = malloc(PAGE_SIZE);
	data_buffer = rand_string(temp_buffer);
	
	/* To write count number of pages data to EEPROM */
	check = EEPROM_write((void *)data_buffer, count);
	if(check == -1)
		printf("Write operation is unsuccessful\n");
	else if(check == 0)
		printf("%d bytes of data is written successfully\n", count*PAGE_SIZE);
		
	/****************************  Write Operation Module End  **********************************/
	
	
	/****************************  Read Operation Module Start   ********************************/		
	
	printf("Enter the number of pages to be read\n");
	scanf("%d", &count);	
	if(count > PAGES){
		printf("Invalid page count");
		return 0;
	}
	
	r_buffer = malloc(count*PAGE_SIZE);
	
	/* To read count number of pages data from EEPROM */
	check = EEPROM_read(r_buffer, count);
	if(check == -1)
		printf("Read operation is unsuccessful\n");
	else if(check == 0)
		printf("%d bytes of data is read successfully\n", count*PAGE_SIZE);
	
	/****************************  Read Operation Module End   ***********************************/

	
	/****************************  Set new position Module Start   *******************************/
	
	check = EEPROM_set(0x0080);
	if(check != -1)
		printf("current page position = %4x\n", check);
		
	/****************************  Set new position Module End   *********************************/
		
	
	/****************************  Erase Operation Module Start   *********************************/
	
	printf("Erase operation is in progress\n");         //To indicate erase operation is going on
	/* To erase the data in EEPROM */
	check = EEPROM_erase();
	if(check == -1)
		printf("Erase operation is unsuccessful\n");
	else if(check == 0)
		printf("Erase operation is successful\n");
	
	/****************************  Erase Operation Module End   *********************************/
	
	
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#define PAGE_SIZE 64
#define PAGES 512
#define FLASHGETS 100
#define FLASHGETP 200
#define FLASHSETP 300
#define FLASHERASE 400

int i2c_fd;

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
	int count, check = 0;
	char *r_buffer,temp_buffer[PAGE_SIZE], *data_buffer, erase;	

	//to open the i2_flash device driver
	i2c_fd = open("/dev/i2c_flash", O_RDWR);
	if (i2c_fd < 0 ) {
		printf("Can not open device file.\n");		
	}

	/************************************** write module start *******************************/
	printf("Enter the number of pages to be written\n");
	scanf("%d", &count);
	
	if(count > PAGES){
		printf("Invalid page count");
		return 0;
	}
	check = 1;
	data_buffer = (char *)malloc((PAGE_SIZE)*sizeof(char));
	data_buffer = rand_string(temp_buffer);
	
	//checks till count pages of data is written to EEPROM
	while(check != 0){
		check = write(i2c_fd , data_buffer, count);
		printf("Written %d bytes of data successfully\n", count*PAGE_SIZE);		
	}
	usleep(3000000);
	
	
	/************************************** write module end *******************************/


	/************************************** read module start *******************************/

	printf("Enter the number of pages to be read\n");
	scanf("%d", &count);
	
	if(count > PAGES){
		printf("Invalid page count");
		return 0;
	}
	
	r_buffer = malloc(count*PAGE_SIZE);
	
	check = 1;
	//check and read count number of pages from EEPROM
	while(check != 0){
		check = read(i2c_fd , r_buffer, count);		
		usleep(1000000);
		printf("Read %d bytes of data successfully\n", count*PAGE_SIZE);
	}	
	usleep(1000000);

	/************************************** read module end *******************************/


	/************************************** ioctl operations start *******************************/

	usleep(1000000);
	
	//to get the current status of EEPROM
	ioctl (i2c_fd, FLASHGETS, NULL);
	
	//to get the current page position of EEPROM
	ioctl (i2c_fd, FLASHGETP, NULL);
	
	//to set the current page position of EEPROM
	ioctl (i2c_fd, FLASHSETP, 0x0080);

	//to perform erase operation on EEPROM
	printf("Do you want to erase the EEPROM - y/n\n");
	scanf("%c",&erase);
	if(erase == 'y'){
		check = 1;
		while(check != 0){
			check = ioctl (i2c_fd, FLASHERASE, NULL);
			printf("Erase operation is successful");
		}
		usleep(3000000);
	}
	
	usleep(5000000);

	/************************************** ioctl operations end *******************************/

	//to close the device file
	close(i2c_fd);
	return 0;
}

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include<linux/init.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/version.h>

#define SPIDEV_MAJOR 153 
#define PATTERN_WRITE 0x0000

struct spidev_data {      
        struct spi_device *spi;       
        char *transfer_buf;
        
}*spi_devp;

int flag = 0;
struct spidev_data *spidev;
static struct class *spi_dev_class;          /* Tie with the device model */
static struct device *spi_dev_device;
unsigned int kernel_pattern_list[10][8];
 
/* row addresses of LED Matrix */
unsigned int led_row_address[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/* Control register values for LED Matrix */
unsigned int control_reg_address[8] = {0x09, 0x00, 0x0A, 0x00, 0x0B, 0x07, 0x0C, 0x01};

static ssize_t led_driver_sync(struct spidev_data *spidev, struct spi_message *message)
{
        int status = 0;
        struct spi_device *spi;        
        spi = spidev->spi;

        if (spi == NULL)
                status = -ESHUTDOWN;
        else
                status = spi_sync(spi, message);

        if (status == 0)
                status = message->actual_length;

        return status;
}

static inline ssize_t led_driver_sync_write(struct spidev_data *spidev, size_t len)
{
        unsigned char data_in[2]={0};
	struct spi_transfer data = {
			.tx_buf = spidev->transfer_buf,
			.rx_buf = data_in,
			.len = 2,
			.speed_hz = 500000,
			.cs_change = 1,
			.delay_usecs = 0,
			.bits_per_word = 8,
	};
        struct spi_message m;
        spi_message_init(&m);
        spi_message_add_tail(&data, &m);
        return led_driver_sync(spidev, &m);
}

/* Write-only message with current device setup */
static ssize_t led_driver_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{	
        ssize_t status = 0;
        unsigned long missing;        
	int i, j, k, leng;
	int *seq_buf;
	char *data_out = kmalloc(2*sizeof(char), GFP_KERNEL);
	leng = 6;
	gpio_set_value_cansleep(15,1);
	
	seq_buf = kmalloc(6*sizeof(int),GFP_KERNEL);
		
        spidev->transfer_buf = kmalloc(2*sizeof(char), GFP_KERNEL);
        if (!spidev->transfer_buf)
        	return -ENOMEM;	
        
        spidev->transfer_buf = data_out;
       
       	/* To copy the sequence pattern from user */
        missing = copy_from_user((void *)seq_buf, (void *)buf, count);
        
        if(missing == 0){
        
		/* To set the values of control registers of LED Matrix */
		if (!flag)
		{
			for(i = 0; i < 8; i = i + 2){
				data_out[0] = control_reg_address[i];
				data_out[1] = control_reg_address[i+1];
				gpio_set_value_cansleep(15,0);
				status = led_driver_sync_write(spidev, 2*(sizeof(char)));
				gpio_set_value_cansleep(15,1);					
				flag++;
			}
		}
		/* To display the sequence of patterns */   
		for(j = 0; j < leng; j = j + 2){
			k = seq_buf[j];
			/* To check the termination of the sequence */
			if(seq_buf[j] == 0 && seq_buf[j+1] == 0){
				/* To switch off the LED display */
				for(i = 0; i < 8; i++){
					data_out[0] = led_row_address[i];
					data_out[1] = 0x00;
					gpio_set_value_cansleep(15,0);
					status = led_driver_sync_write(spidev, 2*(sizeof(char)));
					gpio_set_value_cansleep(15,1);
					msleep(5);	/* Delay to display the LED off state at the end of sequence */ 								
				} 
				return 0;      
			}else{
				/* To display the pattern received from user space */
				for(i = 0; i < 8; i++){
					data_out[0] = led_row_address[i];
					data_out[1] = kernel_pattern_list[k][i];
					gpio_set_value_cansleep(15,0);
					status = led_driver_sync_write(spidev, 2*(sizeof(char)));
					gpio_set_value_cansleep(15,1);								
				}             	       	
			}
			/* sleep for the time received from user sequence */
			msleep(seq_buf[j+1]);
		}	
        }	
        return 0;
}

/* To have the patterns in user space to be copied to kernel space buffer */
long led_driver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int i, j;
	unsigned int **pattern;
	pattern = (unsigned int **)arg; 
	switch(cmd){
		case PATTERN_WRITE: 
				for(i = 0; i < 10; i++)
					for(j = 0; j < 8; j++)
					{
				 		kernel_pattern_list[i][j] = pattern[i][j];  
				 	}	
				 break;
		default: asm volatile("nop"); 	
	}		
        return 0;
}

/*
 * Open led driver
 */
int led_driver_open(struct inode *inode, struct file *file)
{	
	printk("led device is opening\n");
	return 0;
}

/*
 * Release led driver
 */
int led_driver_release(struct inode *inode, struct file *file)
{	
	/* To free the gpio's */
        gpio_free(24);
        gpio_free(44);
        gpio_free(72);
        gpio_free(30);
        gpio_free(46);
        gpio_free(42);
        gpio_free(15);
	
	printk("\n led device is closing\n");
	return 0;
}

/*
 * Read to led driver
 */
ssize_t led_driver_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int bytes_read = 0;
	return bytes_read;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations spidev_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= led_driver_open,        /* Open method */
    .release		= led_driver_release,     /* Release method */
    .write		= led_driver_write,       /* Write method */
    .read		= led_driver_read,        /* Read method */
    .unlocked_ioctl	= led_driver_ioctl,       /* IO Control method */
};

static int led_driver_probe(struct spi_device *spi)
{
        /* Allocate driver data */
        spidev = kmalloc(sizeof(struct spidev_data), GFP_KERNEL);
        if (!spidev)
                return -ENOMEM;

        /* Initialize the driver data */
        spidev->spi = spi;
	
	/* Device creation */
	spi_dev_device = device_create(spi_dev_class, &spi->dev, MKDEV(SPIDEV_MAJOR, 0), spidev, "spidev%d.%d", spi->master->bus_num, spi->chip_select);

        return 0;
}

static int led_driver_remove(struct spi_device *spi){
	device_destroy(spi_dev_class, MKDEV(SPIDEV_MAJOR, 0));
	return 0;
}	

static struct spi_driver spidev_spi_driver = {	
        .driver = {
                .name = "spidev",
        },
        .probe = led_driver_probe,
        .remove = led_driver_remove,
};

/*
 * Driver Initialization
 */
int __init led_driver_init(void)
{
	int ret;

	/* Registering the device */
	ret = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);
        if (ret < 0)
                return ret;
        
        /* creating class for the module */
	spi_dev_class = class_create(THIS_MODULE, "spidev");
	if (IS_ERR(spi_dev_class)) {
		printk("spi dev class creation\n");
	}
        
        /* To register the device */        
        spi_register_driver(&spidev_spi_driver);
		
       	/* To export and set the gpio pins required for LED screen display*/
	/* ------------- This is for setting SPI1_MOSI i.e., GPIO11 ------------- */	
	/*For setting level shifter pin */
	gpio_request(24, "MOSI LS");
	gpio_direction_output(24,0);
	gpio_set_value_cansleep(24,0);
	
	/*To set the Multiplexer1 */
	gpio_request(44, "IO11 MUX1");
	gpio_direction_output(44,0);
	gpio_set_value_cansleep(44,1);
	
	/*To set the Multiplexer2 */
	gpio_request(72, "IO11 MUX2");
	gpio_direction_output(72,0);
	gpio_set_value_cansleep(72,0);
	
	/* ------------- For setting SPI1_SCK, GPIO13 ------------- */		
	/*For setting level shifter pin */
	gpio_request(30, "SCK LS");
	gpio_direction_output(30,0);
	gpio_set_value_cansleep(30,0);

	/*To set the Multiplexer */	
	gpio_request(46, "IO13 MUX");
	gpio_direction_output(46,0);
	gpio_set_value_cansleep(46,1);
	
	/* To use IO12 as CS gpio pin */
	gpio_request(15, "CS");
	gpio_direction_output(15,0);
	
	/*For setting level shifter pin for chip select, IO12 */
	gpio_request(42, "CS LS");
	gpio_direction_output(42,0);
	gpio_set_value_cansleep(42,0);
	
	printk("led driver initialized.\n");
	return 0;
}

/* Driver Exit */
void __exit led_driver_exit(void)
{
        /* to unregister the device */
	spi_unregister_driver(&spidev_spi_driver);
	
	/* to destroy the class */
	class_destroy(spi_dev_class);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);

	printk("led driver removed.\n");
}

/* calling module initialization and exit */
module_init(led_driver_init);
module_exit(led_driver_exit);
MODULE_LICENSE("GPL v2");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/jiffies.h>
#include<linux/init.h>
#include<linux/notifier.h>
#include<linux/moduleparam.h>
#include<linux/list.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#define DEVICE_NAME "i2c_flash"  // device name to be created and registered
#define PAGESIZE 64       
#define PAGES 512 
#define FLASHGETS 100
#define FLASHGETP 200
#define FLASHSETP 300
#define FLASHERASE 400

bool EEPROM_read_Status = false;      //to check if eeprom is busy with read operation
bool EEPROM_write_Status = false;	//to check if eeprom is busy with write operation
int read_bytes = 0,written_bytes = 0;	//to hold values of read and written bytes
unsigned int page_position;		//to hold the page position of EEPROM

/* EEPROM Device strcture */
struct i2c_flash {
	struct i2c_adapter *adap;
	struct device *dev;
	struct cdev cdev;
}*i2c_devp;

static struct workqueue_struct *my_wq;

/* work structure */
typedef struct {
	struct work_struct my_work;
	char *buf;
	struct file *fp;
	int count;	
}work_t;

work_t *work1, *work2, *work3;

/* work queue read function */
static void my_wq_read_function(struct work_struct *work) {
	work_t *my_work = (work_t *)work;
	int l;
	char *address_buffer;
	struct i2c_client *client;

	struct file *f;
	f = my_work->fp;
	client = f->private_data;

	my_work->buf = kmalloc((my_work->count)*PAGESIZE, GFP_KERNEL);

	EEPROM_read_Status = true;

	if(page_position > 0x7FFF)
		page_position = 0x0000;
		
	address_buffer = kmalloc(2, GFP_KERNEL);

	address_buffer[0] = 0x00;
	address_buffer[1] = 0x00;
	i2c_master_send(client, address_buffer, 2);     //to write the address from where read operation has to be done

	gpio_set_value_cansleep(10,1);  //to turn ON LED
	read_bytes = i2c_master_recv(client, my_work->buf, (my_work->count)*PAGESIZE);     //to read the count pages of data
	gpio_set_value_cansleep(10,0);   //to turn OFF LED

	for(l = 0; l < my_work->count; l++)
		page_position = page_position + 0x0040;

	EEPROM_read_Status = false;
}

static void my_wq_write_function(struct work_struct *work) {
	work_t *my_work = (work_t *)work;
	int i, j, byte_count, l = 0;
	struct i2c_client *client;
	char *data_buffer, *temp_buffer;
	struct file *f;
	f = my_work->fp;
	client = f->private_data;	
	temp_buffer = my_work->buf;
	data_buffer = kmalloc((PAGESIZE + 2)*sizeof(char), GFP_KERNEL);
	EEPROM_write_Status = true;
	gpio_set_value_cansleep(10,1);      //to turn on LED
	while(l < my_work->count){
		if(page_position > 0x7FFF)
			page_position = 0x0000;		

		data_buffer[0] = (page_position>>8) & (0xFF);
		data_buffer[1] = (page_position) & 0xFF;
		for(i = 2, j = 0; j < PAGESIZE; i++, j++){
			data_buffer[i] = temp_buffer[j];	 
		}
	 
		byte_count = i2c_master_send(client, data_buffer, PAGESIZE+2);    //to write the starting address and data to be written 												to EEPROM

		page_position = page_position + 0x0040;
		l++;
		written_bytes = byte_count + written_bytes;
        }
	gpio_set_value_cansleep(10,0);    //to turn OFF LED
	EEPROM_write_Status = false;
}

static dev_t i2c_dev_number;      /* Allotted device number */
struct class *i2c_dev_class;          /* Tie with the device model */
static struct device *i2c_dev_device;

/*
* Open i2c_flash driver
*/
int i2c_driver_open(struct inode *inode, struct file *file)
{
	struct i2c_client *client;
        struct i2c_adapter *adap;
        struct i2c_flash *i2c_dev;     
	
        adap = i2c_get_adapter(0);
        client = kzalloc(sizeof(*client), GFP_KERNEL);
        snprintf(client->name, I2C_NAME_SIZE, "i2c-flash %d", adap->nr);
        
        /* Get the per-device structure that contains this cdev */
	i2c_dev = container_of(inode->i_cdev, struct i2c_flash, cdev);

        client->adapter = adap;
	client->addr = 0x51;
        file->private_data = client;

        return 0;
}

/*
 * Release i2c_flash driver
 */
int i2c_driver_release(struct inode *inode, struct file *file)
{
	struct i2c_client *client = file->private_data;

	kfree(client);
	file->private_data = NULL;
	gpio_free(60);
	gpio_free(74);
	gpio_free(10);	
	printk("\n%s is closing\n", DEVICE_NAME);
	
	return 0;
}

/*		
 * Write to i2c_flash driver
 */
ssize_t i2c_driver_write(struct file *file, char *buf, size_t count, loff_t *ppos)
{	
	int value;    
	char *temp_buffer;

	//to check the status of EEPROM
	if(EEPROM_write_Status)	
		return -EBUSY;
	else{
		temp_buffer = kmalloc(PAGESIZE, GFP_KERNEL); 
		temp_buffer = memdup_user(buf,PAGESIZE);
		if(IS_ERR(temp_buffer))
			return PTR_ERR(temp_buffer);
		work1 = (work_t *)kmalloc(sizeof(work_t), GFP_KERNEL);
		INIT_WORK((struct work_struct *)work1, my_wq_write_function);     
		work1->fp = file;
		work1->buf = temp_buffer;
		work1->count = count;
		value = queue_work(my_wq, (struct work_struct *)work1);      //to queue the write work
		return 0;
	}
	return -1;
}
/*
 * Read to i2c_flash driver
 */
ssize_t i2c_driver_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int ret = 0, value;
	//to check if read data is ready and eeprom is not busy
	if(read_bytes != count*PAGESIZE && !EEPROM_read_Status){
		work2 = (work_t *)kmalloc(sizeof(work_t), GFP_KERNEL);
		INIT_WORK((struct work_struct *)work2, my_wq_read_function);
		work2->fp = file;
		work2->count = count;
		value = queue_work(my_wq, (struct work_struct *)work2);      //to queue the read work
		return -EAGAIN;
	}else if(read_bytes != count*PAGESIZE && EEPROM_read_Status){  //to check if read data is ready and eeprom is busy
		return -EBUSY;
	}else if(read_bytes == count*PAGESIZE){    //to check if read data is ready
		ret = copy_to_user(buf, work2->buf, count*PAGESIZE);
		read_bytes = 0;
		if(ret >= 0)
			return 0;
	}
	return -1;	
}

/* to perform erase operation on EEPROM */
int eeprom_Erase(struct file *file){
	char *data_buffer;
	int i, value;
	data_buffer = kmalloc(PAGESIZE*sizeof(char), GFP_KERNEL);
	for(i = 0; i < PAGESIZE; i++)
		data_buffer[i] = 1;
	page_position = 0x0000;
		
	if(EEPROM_write_Status)	
		return -EBUSY;
	else{
		work3 = (work_t *)kmalloc(sizeof(work_t), GFP_KERNEL);
		INIT_WORK((struct work_struct *)work3, my_wq_write_function);
		work3->fp = file;
		work3->buf = data_buffer;
		work3->count = PAGES;
		value = queue_work(my_wq, (struct work_struct *)work3);
		return 0;
	}
}

/* to perform io control operations */
int i2c_driver_ioctl(struct file *file, unsigned long request, unsigned char argp){
	switch(request){
		case FLASHGETS: {    //to check the status of eeprom
					if(EEPROM_read_Status || EEPROM_write_Status)
						return -EBUSY;
					else
						return 0;		
				}
		
		case FLASHGETP: {	//to return current page position of eeprom
					return page_position;
				}

		case FLASHSETP: {       //to set page position to new position
					if(argp > 0x7FFF) 
						return -ENOMEM;
					else{
						page_position = argp;
						return page_position;
					    }
				}

		case FLASHERASE: {	//to erase the data on eeprom
					return eeprom_Erase(file);
				 }
		default: return -1;
	}
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations i2c_fops = {
    .owner		= THIS_MODULE,            /* Owner */
    .open		= i2c_driver_open,        /* Open method */  
    .write		= i2c_driver_write,       /* Write method */
    .read		= i2c_driver_read,        /* Read method */
    .unlocked_ioctl	= i2c_driver_ioctl,	  /* IO control method */
    .release		= i2c_driver_release,     /* Release method */
};

/*
 * Driver Initialization
 */
int __init i2c_driver_init(void)
{
	int ret;
	struct i2c_adapter *adap;
	my_wq = create_workqueue("my_queue");

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&i2c_dev_number, 0, 1, DEVICE_NAME) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	i2c_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	i2c_devp = kmalloc(sizeof(struct i2c_flash), GFP_KERNEL);
		
	if (!i2c_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}
	
	adap = to_i2c_adapter(0);
	
	/* Connect the file operations with the cdev */
	cdev_init(&i2c_devp->cdev, &i2c_fops);
	i2c_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&i2c_devp->cdev, (i2c_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	i2c_dev_device = device_create(i2c_dev_class, NULL, MKDEV(MAJOR(i2c_dev_number), 0), NULL, DEVICE_NAME);		
	
	//to request the SDA, SCL and LED gpio pins
	gpio_request(60, "Mux1 GPIO");
	gpio_request(74, "Mux1 LED");
	gpio_request(10, "LED");

	//to set the direction of SDA, SCL and LED gpio pins
	gpio_direction_output(60,0);
	gpio_direction_output(74,0);
	gpio_direction_output(10,0);

	//to set the value of SDA, SCL and LED gpio pins
	gpio_set_value_cansleep(60,0);
	gpio_set_value_cansleep(74,0);
	gpio_set_value_cansleep(10,0);
	page_position = 0x0000;

	printk("i2c_flash driver initialized.\n");
	return 0;
}
/* Driver Exit */
void __exit i2c_driver_exit(void)
{
	struct i2c_adapter *adap;
	
	adap = to_i2c_adapter(0);
	/* Release the major number */
	unregister_chrdev_region((i2c_dev_number), 1);

	/* Destroy device */
	device_destroy (i2c_dev_class, MKDEV(MAJOR(i2c_dev_number), 0));
	cdev_del(&i2c_devp->cdev);
	kfree(i2c_devp);
		
	/* Destroy driver_class */
	class_destroy(i2c_dev_class);

	printk("i2c_flash driver removed.\n");
}

MODULE_AUTHOR("Sweetie");
MODULE_DESCRIPTION("TEST");
module_init(i2c_driver_init);
module_exit(i2c_driver_exit);
MODULE_LICENSE("GPL v2");

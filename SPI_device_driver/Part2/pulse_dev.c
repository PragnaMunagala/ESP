#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>
#define DEVICE_NAME "pulse_dev"  // device name to be created and registered
static DEFINE_MUTEX(pulse_width_lock);

/* per device structure */
struct pulse_dev {
	struct cdev cdev;               /* The cdev structure */
	char name[20];  		/* Name of device */
	unsigned long long width;                
} *pulse_devp;

static dev_t pulse_dev_number;      /* Allotted device number */
struct class *pulse_dev_class;          /* Tie with the device model */
static struct device *pulse_dev_device;
unsigned int irqNumber;
unsigned long long pulse_width, startTime, endTime;
bool check_echo_status = false;
bool edge_detect = false;

/* Time stamp counter */
static __inline__ unsigned long long rdtsc(void)
{
     unsigned a, d;
     asm volatile("rdtsc" : "=a" (a), "=d" (d));

     return (((unsigned long long)a) | (((unsigned long long)d) << 32));
}

/*
 * Open pulse driver
 */
int pulse_driver_open(struct inode *inode, struct file *file)
{
	struct pulse_dev *pulse_devp;

	/* Get the per-device structure that contains this cdev */
	pulse_devp = container_of(inode->i_cdev, struct pulse_dev, cdev);


	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = pulse_devp;
	printk("\n%s is openning \n", pulse_devp->name);
	return 0;
}

/*
 * Release pulse driver
 */
int pulse_driver_release(struct inode *inode, struct file *file)
{
	struct pulse_dev *pulse_devp = file->private_data;
	
	printk("\n%s is closing\n", pulse_devp->name);
	
	/* To free the GPIO's */
	gpio_free(13);
        gpio_free(34);
        gpio_free(77);
        gpio_free(14);
        gpio_free(76);
        gpio_free(64);
	return 0;
}

/* ISR to handle the interrupt on echo signal */
static irq_handler_t pulse_measurement(unsigned int irq, void *dev_id, struct pt_regs *regs){
	if(!check_echo_status)
	{
		startTime = rdtsc();			//to measure the start time of echo pulse
		irq_set_irq_type(irqNumber, IRQF_TRIGGER_FALLING);		//to set the irq_type to falling
		check_echo_status = true;
	}else if(check_echo_status){
		endTime = rdtsc();			//to measure the end time of echo pulse
		irq_set_irq_type(irqNumber, IRQF_TRIGGER_RISING);		//to set the irq_type to rising
		edge_detect = true;			//to indicate echo pulse is detected
		check_echo_status = false;	
	}	
	return (irq_handler_t) IRQ_HANDLED;
}

/*
 * Write to pulse driver
 */
ssize_t pulse_driver_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	/* to generate trigger signal */
	gpio_set_value_cansleep(13,1);
	udelay(10);
	gpio_set_value_cansleep(13,0);	
	
	return 0;
}

/*
 * Read to pulse driver
 */
ssize_t pulse_driver_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int i;
	unsigned long long count1 = 0;	
	struct pulse_dev *pulse_devp = file->private_data;
	
	/* To check if pulse is detected or not and calculate the pulse width */
	if(edge_detect){
		edge_detect = false;
		pulse_devp->width = endTime-startTime;
		for(i = 0; count1 < (endTime-startTime); i++)
			count1 = count1 + 400;
		
		/* To copy the pulse width value to the user space */
		//value = copy_to_user(buf, (char *)p, sizeof(long long));	
		return i - 1;		
	}
	else
		return -1;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations pulse_fops = {
    .owner		= THIS_MODULE,              /* Owner */
    .open		= pulse_driver_open,        /* Open method */
    .release		= pulse_driver_release,     /* Release method */
    .write		= pulse_driver_write,       /* Write method */
    .read		= pulse_driver_read,        /* Read method */
};

/*
 * Driver Initialization
 */
int __init pulse_driver_init(void)
{
	int ret;
	
	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&pulse_dev_number, 0, 1, DEVICE_NAME) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	pulse_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	pulse_devp = kmalloc(sizeof(struct pulse_dev), GFP_KERNEL);
		
	if (!pulse_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	/* Request I/O region */
	sprintf(pulse_devp->name, DEVICE_NAME);

	/* Connect the file operations with the cdev */
	cdev_init(&pulse_devp->cdev, &pulse_fops);
	pulse_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&pulse_devp->cdev, (pulse_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	pulse_dev_device = device_create(pulse_dev_class, NULL, MKDEV(MAJOR(pulse_dev_number), 0), NULL, DEVICE_NAME);	
	
	/* To initialize the IO pins */
	/******************** To intialize the gpio pins for sending trigger pulse to the sensor *******************/	
	/* To set the IO2 */
	gpio_request(13, "trigger");
	gpio_direction_output(13,0);
	
	/* For setting level shifter pin */
	gpio_request(34, "triggerLS");
	gpio_direction_output(34,0);
	gpio_set_value_cansleep(34,0);
	
	/* For setting the multiplexer */
	gpio_request(77, "triggerMUX");
	gpio_set_value_cansleep(77,0);
	

	/******************* This is to intialize the gpio pins for receiving echo pulse from the sensor ******************/
	/* To set the IO3 */
	gpio_request(14, "echo");
	gpio_direction_input(14);
	
	/* For setting Multilplexer1 */
	gpio_request(76, "echoMUX1");
	gpio_set_value_cansleep(76,0);
	
	/* For setting Multiplexer2 */
	gpio_request(64, "echoMux2");
	gpio_set_value_cansleep(64,0);	
	
	/* Request IRQ number for Echo signal pin */
	irqNumber = gpio_to_irq(14);
	
	/* Request IRQ for handling interrupts on echo singal */
	ret = request_irq(irqNumber, (irq_handler_t) pulse_measurement, IRQF_TRIGGER_RISING, "pulse measure", NULL);
	
	printk("pulse driver initialized.\n");
	return 0;
}

/* Driver Exit */
void __exit pulse_driver_exit(void)
{
	/* Release the major number */
	unregister_chrdev_region((pulse_dev_number), 1);

	/* Destroy device */
	device_destroy (pulse_dev_class, MKDEV(MAJOR(pulse_dev_number), 0));
	cdev_del(&pulse_devp->cdev);
	kfree(pulse_devp);
	
	/* Destroy driver_class */
	class_destroy(pulse_dev_class);

	printk("pulse driver removed.\n");
}

/* calling module initialization and exit */
module_init(pulse_driver_init);
module_exit(pulse_driver_exit);
MODULE_LICENSE("GPL v2");

#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/types.h>
#include<linux/slab.h>
#include<asm/uaccess.h>
#include<linux/string.h>
#include<linux/device.h>
#include<linux/jiffies.h>
#include<linux/errno.h>
#include<linux/init.h>
#include<linux/moduleparam.h>

#define DEVICE_NAME1 "bus_in_q"  // device name to be created and registered
#define DEVICE_NAME2 "bus_out_q1"
#define DEVICE_NAME3 "bus_out_q2"
#define DEVICE_NAME4 "bus_out_q3"

#define queue_length 10

struct squeue {
	struct cdev cdev;               /* The cdev structure */
	char name[20];                  /* Name of device*/			
	int start;                 
	int end;
	int size;
        struct message *in_message;                                      
};

struct squeue *bus_in_q,*bus_out_q1,*bus_out_q2,*bus_out_q3 ; 

struct message {
	int messageId;
	int sourceId;
	int destinationId;
	char message[80];
	float elapseTime;
};
	
static dev_t bus_in_q_dev_number;      /* Allotted device number */
struct class *bus_in_q_dev_class;          /* Tie with the device model */
static struct device *bus_in_q_dev_device;

static dev_t bus_out_q1_dev_number;      /* Allotted device number */
struct class *bus_out_q1_dev_class;          /* Tie with the device model */
static struct device *bus_out_q1_dev_device;

static dev_t bus_out_q2_dev_number;      /* Allotted device number */
struct class *bus_out_q2_dev_class;          /* Tie with the device model */
static struct device *bus_out_q2_dev_device;

static dev_t bus_out_q3_dev_number;      /* Allotted device number */
struct class *bus_out_q3_dev_class;          /* Tie with the device model */
static struct device *bus_out_q3_dev_device;

int squeue_driver_open(struct inode *inode, struct file *file)
{
	
	struct squeue *sq;
	/* Get the per-device structure that contains this cdev */
	sq = container_of(inode->i_cdev, struct squeue, cdev);


	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = sq;
	printk("\n%s is opening \n", sq->name);
	return 0;
}

int squeue_driver_release(struct inode *inode, struct file *file)
{
	struct squeue *sq;
	sq = file->private_data;
	
	printk("\n%s is closing\n", sq->name);
	
	return 0;
}

ssize_t squeue_driver_write(struct file *file, const char *buff,
           size_t count, loff_t *ppos)
{
	struct squeue *sq;	
	int errno = 0;
	struct message *buf = (struct message *)buff;
	sq = file->private_data;	
	printk("writing bus_in_q");
	if(sq->size == queue_length) {
		
		printk("error in write %d\n",sq->size);
		errno = EINVAL;
		return -1;
	}
	else { 
		sq->size++;
		sq->end++;
		/* As queue is filled in circular manner */
		if(sq->end == queue_length){
			sq->end = 0;
		}
				
        	copy_from_user(&(sq->in_message[sq->end]), buf, sizeof(struct message)); 
		return sq->end;
	}
	return 0;
	
}

ssize_t squeue_driver_read(struct file *file, char *buff,
           size_t count, loff_t *ppos)
{
	struct squeue *sq;	
	int errno = 0;
	struct message *buf = (struct message *)buff;
	sq = file->private_data;
	
	printk("reading bus_in_q");
	if(sq->size == 0){
		errno = EINVAL;
		return -1;
	}
	else{
		sq->size--;
		sq->start++;
		printk("daemon read sqsize=%d and sqstart=%d\n",sq->size,sq->start);
		/* As queue is filled in circular manner */
		if(sq->start == queue_length)
			sq->start = 0;

		copy_to_user(buf, &(sq->in_message[sq->start]), sizeof(struct message));
		return sq->start;
	}
}

static struct file_operations squeue_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= squeue_driver_open,        /* Open method */
    .release	        = squeue_driver_release,     /* Release method */
    .write		= squeue_driver_write,       /* Write method */
    .read		= squeue_driver_read,        /* Read method */
};

int __init squeue_driver_init(void)
{
	int ret;
	int time_since_boot;
	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&bus_in_q_dev_number, 0, 1, DEVICE_NAME1) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	if (alloc_chrdev_region(&bus_out_q1_dev_number, 1, 1, DEVICE_NAME2) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	if (alloc_chrdev_region(&bus_out_q2_dev_number, 2, 1, DEVICE_NAME3) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	if (alloc_chrdev_region(&bus_out_q3_dev_number, 3, 1, DEVICE_NAME4) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}
	
	/* Populate sysfs entries */
	bus_in_q_dev_class = class_create(THIS_MODULE, DEVICE_NAME1);
	bus_out_q1_dev_class = class_create(THIS_MODULE, DEVICE_NAME2);
	bus_out_q2_dev_class = class_create(THIS_MODULE, DEVICE_NAME3);
	bus_out_q3_dev_class = class_create(THIS_MODULE, DEVICE_NAME4);

	/* Allocate memory for the per-device structure */	
	bus_in_q = kmalloc(sizeof(struct squeue), GFP_KERNEL);
	bus_in_q->in_message = kmalloc(sizeof(struct message)*queue_length, GFP_KERNEL);

	bus_out_q1 = kmalloc(sizeof(struct squeue), GFP_KERNEL);
	bus_out_q1->in_message = kmalloc(sizeof(struct message)*queue_length, GFP_KERNEL);

	bus_out_q2 = kmalloc(sizeof(struct squeue), GFP_KERNEL);
	bus_out_q2->in_message = kmalloc(sizeof(struct message)*queue_length, GFP_KERNEL);

	bus_out_q3 = kmalloc(sizeof(struct squeue), GFP_KERNEL);
	bus_out_q3->in_message = kmalloc(sizeof(struct message)*queue_length, GFP_KERNEL);

        if (!(bus_in_q->in_message) || !(bus_out_q1->in_message) || !(bus_out_q2->in_message) || !(bus_out_q3->in_message)) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}
		
	if (!bus_in_q || !bus_out_q1 || !bus_out_q2 || !bus_out_q3) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}
		

	/* Request I/O region */
	sprintf(bus_in_q->name, DEVICE_NAME1);
	sprintf(bus_out_q1->name, DEVICE_NAME2);
	sprintf(bus_out_q2->name, DEVICE_NAME3);
	sprintf(bus_out_q3->name, DEVICE_NAME4);
	
	/* Connect the file operations with the cdev */
	cdev_init(&bus_in_q->cdev, &squeue_fops);
	bus_in_q->cdev.owner = THIS_MODULE;
	cdev_init(&bus_out_q1->cdev, &squeue_fops);
	bus_out_q1->cdev.owner = THIS_MODULE;
	cdev_init(&bus_out_q2->cdev, &squeue_fops);
	bus_out_q2->cdev.owner = THIS_MODULE;
	cdev_init(&bus_out_q3->cdev, &squeue_fops);
	bus_out_q3->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&bus_in_q->cdev, (bus_in_q_dev_number), 1);
	ret = cdev_add(&bus_out_q1->cdev, (bus_out_q1_dev_number), 1);
	ret = cdev_add(&bus_out_q2->cdev, (bus_out_q2_dev_number), 1);
	ret = cdev_add(&bus_out_q3->cdev, (bus_out_q3_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	bus_in_q_dev_device = device_create(bus_in_q_dev_class, NULL, MKDEV(MAJOR(bus_in_q_dev_number), 0), NULL, DEVICE_NAME1);
	bus_out_q1_dev_device = device_create(bus_out_q1_dev_class, NULL, MKDEV(MAJOR(bus_out_q1_dev_number), 1), NULL, DEVICE_NAME2);
	bus_out_q2_dev_device = device_create(bus_out_q2_dev_class, NULL, MKDEV(MAJOR(bus_out_q2_dev_number), 2), NULL, DEVICE_NAME3);
	bus_out_q3_dev_device = device_create(bus_out_q3_dev_class, NULL, MKDEV(MAJOR(bus_out_q3_dev_number), 3), NULL, DEVICE_NAME4);		
	
	time_since_boot=(jiffies-INITIAL_JIFFIES)/HZ;//since on some systems jiffies is a very huge uninitialized value at boot and saved.
	
        bus_in_q->start = -1;
        bus_in_q->end = -1;
        bus_in_q->size = 0;

	bus_out_q1->start = -1;
        bus_out_q1->end = -1;
        bus_out_q1->size = 0;

	bus_out_q2->start = -1;
        bus_out_q2->end = -1;
        bus_out_q2->size = 0;

	bus_out_q3->start = -1;
        bus_out_q3->end = -1;
        bus_out_q3->size = 0;

	printk("bus_driver initialized.\n");
	return 0;
}

void __exit squeue_driver_exit(void)
{
	
	/* Release the major number */
	unregister_chrdev_region((bus_in_q_dev_number), 1);
	unregister_chrdev_region((bus_out_q1_dev_number), 1);
	unregister_chrdev_region((bus_out_q2_dev_number), 1);
	unregister_chrdev_region((bus_out_q3_dev_number), 1);

	/* Destroy device */
	device_destroy (bus_in_q_dev_class, MKDEV(MAJOR(bus_in_q_dev_number), 0));
	cdev_del(&bus_in_q->cdev);
	device_destroy (bus_out_q1_dev_class, MKDEV(MAJOR(bus_out_q1_dev_number), 1));
	cdev_del(&bus_out_q1->cdev);
	device_destroy (bus_out_q2_dev_class, MKDEV(MAJOR(bus_out_q2_dev_number), 2));
	cdev_del(&bus_out_q2->cdev);
	device_destroy (bus_out_q3_dev_class, MKDEV(MAJOR(bus_out_q3_dev_number), 3));
	cdev_del(&bus_out_q3->cdev);
	
	kfree(bus_in_q->in_message);
	kfree(bus_in_q);
	kfree(bus_out_q1->in_message);
	kfree(bus_out_q1);
	kfree(bus_out_q2->in_message);
	kfree(bus_out_q2);
	kfree(bus_out_q3->in_message);
	kfree(bus_out_q3);
	
	/* Destroy driver_class */
	class_destroy(bus_in_q_dev_class);
	class_destroy(bus_out_q1_dev_class);
	class_destroy(bus_out_q2_dev_class);
	class_destroy(bus_out_q3_dev_class);

	printk("bus_driver removed.\n");
}

module_init(squeue_driver_init);
module_exit(squeue_driver_exit);
MODULE_LICENSE("GPL v2");



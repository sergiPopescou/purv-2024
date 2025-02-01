/***********************************************************************
*  \file       driver.c
*
*  \details    Simple Linux device driver (IOCTL)
*
*  \author     EmbeTronicX, AnjaDj
*
*  \note       Tested with Linux raspberrypi 6.6.62+rpt-rpi-v7
*
***********************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>                 
#include <linux/uaccess.h>              
#include <linux/ioctl.h>
#include <linux/err.h>
 

/**
 * Define the IOCTL command as
 *   #define "ioctl name" __IOX(magic numb, command numb, arg type)
 *       IOX = IO  : an ioctl with no parameters
 *       IOX = IOW : an ioctl with write parameters (copy_from_user)
 *       IOX = IOR : an ioctl with read  parameters (copy_to_user)
 *       IOX = IOWR: an ioctl with both write and read parameters
 */ 
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)
 
#define ALIVE 1
#define DEAD 0

int32_t value = 0;
int32_t newState = 0; 
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;


static int      __init etx_driver_init(void);
static void     __exit etx_driver_exit(void);
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static long     etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = etx_read,
        .write          = etx_write,
        .open           = etx_open,
        .unlocked_ioctl = etx_ioctl, // add ioctl function to driver
        .release        = etx_release,
};

// This function will be called when we open the Device file
static int etx_open(struct inode *inode, struct file *file)
{
        pr_info("Device File Opened...!!!\n");
        return 0;
}

// This function will be called when we close the Device file
static int etx_release(struct inode *inode, struct file *file)
{
        pr_info("Device File Closed...!!!\n");
        return 0;
}

// This function will be called when we read the Device file
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        pr_info("Read Function\n");
        return 0;
}

// This function will be called when we write the Device file
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        pr_info("Write function\n");
        return len;
}


/** 
 * Implement the IOCTL call we defined into the Device driver.
 *   @param file : is the file pointer to the file that was passed by the application.
 *   @param cmd  : is the ioctl command that was called from the userspace.
 *   @param arg  : are the arguments passed from the userspace
 */
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
        case WR_VALUE:
                if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                    pr_err("Data Write : Err!\n");
                
                pr_info("Value = %x\n", value);
                calculate_new_state();
                break;
                
        case RD_VALUE:
                if( copy_to_user((int32_t*) arg, &newState, sizeof(newState)) )
                    pr_err("Data Read : Err!\n");
                
                break;
        
        default:
                pr_info("Default\n");
                break;
    }
        return 0;
}


void calculate_new_state() 
{
    int alive_neighbors = 0;
	
    uint8_t susjed1 = value >> 8;
    uint8_t susjed2 = (value & 0x00000080) >> 7;
    uint8_t susjed3 = (value & 0x00000040) >> 6;
    uint8_t susjed4 = (value & 0x00000020) >> 5;
    uint8_t celija  = (value & 0x00000010) >> 4;
    uint8_t susjed5 = (value & 0x00000008) >> 3;
    uint8_t susjed6 = (value & 0x00000004) >> 2;
    uint8_t susjed7 = (value & 0x00000002) >> 1;
    uint8_t susjed8 = (value & 0x00000001);
    
    alive_neighbors = susjed1 + susjed2 + susjed3 + susjed4 +
                      susjed5 + susjed6 + susjed7 + susjed8;
        
    if (celija == ALIVE)
    {
        // Živa ćelija sa manje od 2 živa suseda ili vise od 3 ziva susjeda umire
        if (alive_neighbors < 2 || alive_neighbors > 3)
            newState = DEAD;
		    
        // Živa ćelija sa 2 ili 3 živa suseda preživljava.
        else if (alive_neighbors == 2 || alive_neighbors == 3)
            newState = ALIVE;
            
    }
    else
    {
        // Mrtva ćelija sa tačno 3 živa suseda oživljava
        if (alive_neighbors == 3)
            newState = ALIVE;
        else
            newState = DEAD;
    }
        
}

// Module Init function
static int __init etx_driver_init(void)
{
        /*Allocating Major number*/
        if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
                pr_err("Cannot allocate major number\n");
                return -1;
        }
        pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
        /*Creating cdev structure*/
        cdev_init(&etx_cdev,&fops);
 
        /*Adding character device to the system*/
        if((cdev_add(&etx_cdev,dev,1)) < 0){
            pr_err("Cannot add the device to the system\n");
            goto r_class;
        }
 
        /*Creating struct class*/
        if(IS_ERR(dev_class = class_create(THIS_MODULE,"etx_class"))){
            pr_err("Cannot create the struct class\n");
            goto r_class;
        }
 
        /*Creating device*/
        if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"etx_device"))){
            pr_err("Cannot create the Device 1\n");
            goto r_device;
        }
        pr_info("Device Driver Insert...Done!!!\n");
        return 0;
 
    r_device:
        class_destroy(dev_class);
    r_class:
        unregister_chrdev_region(dev,1);
        return -1;
}

// Module exit function
static void __exit etx_driver_exit(void)
{
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        pr_info("Device Driver Remove...Done!!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com> & AnjaDj");
MODULE_DESCRIPTION("Simple Linux device driver (IOCTL)");
MODULE_VERSION("1.5");

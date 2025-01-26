/***************************************************************************//**
*  \file       driver.c
*
*  \details    Simple Linux device driver (IOCTL)
*
*  \author     EmbeTronicX
*
*  \Tested with Linux raspberrypi 5.10.27-v7l-embetronicx-custom+
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>                 //kmalloc()
#include<linux/uaccess.h>              //copy_to/from_user()
#include <linux/ioctl.h>
#include <linux/err.h>
 
 
#define WR_VALUE _IOW('a','a',STATISTICS*)
#define RD_VALUE _IOR('a','b',STATISTICS*)

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

/*
** Function Prototypes
*/
static int      __init etx_driver_init(void);
static void     __exit etx_driver_exit(void);
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static long     etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static void     print_statistics_data(void);

/*
** File operation sturcture
*/
static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = etx_read,
        .write          = etx_write,
        .open           = etx_open,
        .unlocked_ioctl = etx_ioctl,
        .release        = etx_release,
};

/*
        User defined structure
*/

typedef struct statistics
{
        int32_t num_alive_cell;
        int32_t num_died_cell;
        int32_t num_revived_cell;
        int32_t num_evolution_cell;
        int8_t  matrix_cell[10][10];
} STATISTICS;

STATISTICS stats;
int8_t cell_initialized = 0;


void print_statistics_data ()
{
        int i = 0, j = 0;
        pr_info("statistics data: \n");
        for(i = 0; i < 10; i++)
        {
                for(j = 0; j < 10; j++)
                {
                        if(stats.matrix_cell[i][j])
                        {
                                pr_info("[#]");
                        }
                        else
                                pr_info("[ ]");
                }
                pr_info("\n");
        }
        pr_info("Alive: %d died: %d revived: %d evolution num: %d \n", 
                stats.num_alive_cell, stats.num_died_cell, 
                stats.num_revived_cell, stats.num_evolution_cell);
}


/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file)
{
        pr_info("Device File Opened...!!!\n");
        return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
        pr_info("Device File Closed...!!!\n");
        return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        pr_info("Read Function\n");
        return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        pr_info("Write function\n");
        return len;
}

/*
** This function will be called when we write IOCTL on the Device file
*/
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
      // pr_info("Size of stats: %d \n", sizeof(stats));
        stats.num_alive_cell = 0;
        stats.num_revived_cell = 0;
        stats.num_died_cell = 0;
        stats.num_evolution_cell = 0;
      
         switch(cmd) {
                case WR_VALUE:
                        if( copy_from_user(&stats ,(STATISTICS *)arg, sizeof(stats)) )
                        {
                                pr_err("Data Write : Err!\n");
                                break;

                        }
                        
                        cell_initialized = 1;
                        print_statistics_data();
                        break;

                case RD_VALUE:
                        if (!cell_initialized) 
                        {
                                pr_info("No data available to read\n");
                                return -EAGAIN;
                        }
                        if (copy_to_user((STATISTICS *)arg, &stats, sizeof(stats))) 
                        {
                                pr_err("Failed to send data to user space\n");
                                return -EFAULT;
                                break;

                        }

                        pr_info("Data read successfully\n");
                        print_statistics_data();
                        return sizeof(stats);
                default:
                pr_info("Default\n");
                break;
                }
        return 0;
}
 
/*
** Module Init function
*/
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

/*
** Module exit function
*/
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
MODULE_AUTHOR("Dejana Smiljanic <smiljanicdejana02@gmail.com>");
MODULE_DESCRIPTION("Simple Linux device driver (IOCTL) combined with conway game-of-life");
MODULE_VERSION("1.5");

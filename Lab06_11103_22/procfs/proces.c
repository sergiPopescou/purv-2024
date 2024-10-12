#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>                 //kmalloc()
#include <linux/uaccess.h>              //copy_to/from_user()
#include <linux/ioctl.h>
#include <linux/proc_fs.h>
#include <linux/version.h>

// #define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + ((c) > 255 ? 255 : (c)))

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define DOES_W_PROC_OPS
#endif

 
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)
 
int32_t value = 0;
char stat_array[5]="1234\0";
static int len = 1;

 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev stat_cdev;
 
static int __init stat_driver_init(void);
static void __exit stat_driver_exit(void);
/*************** Driver Functions **********************/
static int stat_open(struct inode *inode, struct file *file);
static int stat_release(struct inode *inode, struct file *file);
static ssize_t stat_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t stat_write(struct file *filp, const char *buf, size_t len, loff_t * off);
 
/***************** Procfs Functions *******************/
static int open_proc(struct inode *inode, struct file *file);
static int release_proc(struct inode *inode, struct file *file);
static ssize_t read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset);
static ssize_t write_proc(struct file *filp, const char *buff, size_t len, loff_t * off);
 
static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = stat_read,
        .write          = stat_write,
        .open           = stat_open,
        .release        = stat_release,
};
 
#ifdef DOES_W_PROC_OPS
static struct proc_ops proc_fops = {
        .proc_open = open_proc,
        .proc_read = read_proc,
        .proc_write = write_proc,
        .proc_release = release_proc
};
#else
static struct file_operations proc_fops = {
        .open = open_proc,
        .read = read_proc,
        .write = write_proc,
        .release = release_proc
};
#endif
static int open_proc(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "statisticki proc file otvoren.....\t");
    return 0;
}
 
static int release_proc(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "statisticki proc file zatvoren.....\n");
    return 0;
}
 
static ssize_t read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset)
{
    printk(KERN_INFO "citanje statistike\n");
    if(len)
        len=0;
    else{
        len=1;
        return 0;
    }
    copy_to_user(buffer,stat_array,5);
 
    return length;;
}
 
static ssize_t write_proc(struct file *filp, const char *buff, size_t len, loff_t * off)
{
    printk(KERN_INFO "upisano u statistiku.....\n");
    copy_from_user(stat_array,buff,len);
    return len;
}
 
static int stat_open(struct inode *inode, struct file *file)
{
        printk(KERN_INFO "Rukovaoc otvoren...!!!\n");
        return 0;
}
 
static int stat_release(struct inode *inode, struct file *file)
{
        printk(KERN_INFO "Rukovaoc zatvoren...!!!\n");
        return 0;
}
 
#define BUF_LEN 170
char* stat_driver_buffer;

static ssize_t stat_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        printk(KERN_INFO "Read function\n");
		if(*off == 0) {
			if(copy_to_user(buf, stat_driver_buffer, BUF_LEN)!=0)
				return -EFAULT;
			else {
				(*off) += BUF_LEN;
				return BUF_LEN;
			}
		}
        return 0;
}

static ssize_t stat_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        /* printk(KERN_INFO "Write function\n"); */
		/* printk(KERN_INFO "%s\n", buf); */
		memset(stat_driver_buffer, 0, BUF_LEN);
		if(copy_from_user(stat_driver_buffer, buf, len) != 0)
			return -EFAULT;
		else
			return len;
        return len;
}
 
 
 
static int __init stat_driver_init(void)
{
        /*Allocating Major number*/
        if((alloc_chrdev_region(&dev, 0, 1, "stat_Dev")) <0){
                printk(KERN_INFO "Cannot allocate major number\n");
                return -1;
        }
        printk(KERN_INFO "Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
        /*Creating cdev structure*/
        cdev_init(&stat_cdev,&fops);
 
        /*Adding character device to the system*/
        if((cdev_add(&stat_cdev,dev,1)) < 0){
            printk(KERN_INFO "Cannot add the device to the system\n");
            goto r_class;
        }
 
        /*Creating struct class*/
        if((dev_class = class_create("luka", "stat_class")) == NULL){
            printk(KERN_INFO "Cannot create the struct class\n");
            goto r_class;
        }
 
        /*Creating device*/
        if((device_create(dev_class,NULL,dev,NULL,"stat_device")) == NULL){
            printk(KERN_INFO "Cannot create the Statistic Device \n");
            goto r_device;
        }
 
        /*Creating Proc entry*/
        proc_create("stat_proc",0666,NULL,&proc_fops);
 
		stat_driver_buffer = kmalloc(BUF_LEN, GFP_KERNEL);
		if (!stat_driver_buffer) {
			goto r_class;
		}
        /* Initialize data buffer. */
        memset(stat_driver_buffer, 0, BUF_LEN);
        printk(KERN_INFO "Statistic Device Driver Insert...Done!!!\n");
    return 0;
 
r_device:
        class_destroy(dev_class);
r_class:
        unregister_chrdev_region(dev,1);
        return -1;
}
 
void __exit stat_driver_exit(void)
{
        remove_proc_entry("stat_proc",NULL);
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&stat_cdev);
		if(stat_driver_buffer)
			kfree(stat_driver_buffer);
        unregister_chrdev_region(dev, 1);
		printk(KERN_INFO "Device Driver Remove...Done!!!\n");
}
 
module_init(stat_driver_init);
module_exit(stat_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("PURV 2020");
MODULE_DESCRIPTION("A simple device driver with statistic on procfs");
MODULE_VERSION("1.0");

/***************************************************************************//**
*  \file       driver.c
*
*  \details    Simple Linux device driver (procfs)
*
*  \author     EmbeTronicX & AnjaDj
* 
*  \Tested with Linux raspberrypi 6.6.66-v7+
*
* *******************************************************************************/
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
#include <linux/proc_fs.h>
#include <linux/err.h>

 
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)
 
#define ALIVE 1
#define DEAD 0 

// za ioctl
int32_t value = 0;
int32_t newState = 0;

// za procfs
char statistic[21];
char input[2];
uint8_t bornCount = 0; // ukupan broj celija koje su se rodile (bile su mrtve pa promijenile stanje u zive)
uint8_t diedCount = 0; // ukupan broj celija koje su umrle (bile su zive pa promijenile stanje u mrtve)
uint8_t generationCount = 0; // broj generacija

static int len = 1;
 
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
static struct proc_dir_entry *parent;


static int      __init etx_driver_init(void);
static void     __exit etx_driver_exit(void);

/*************** Driver Functions **********************/
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static long     etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
 
/***************** Procfs Functions *******************/
static int      open_proc(struct inode *inode, struct file *file);
static int      release_proc(struct inode *inode, struct file *file);
static ssize_t  read_proc(struct file *filp, char __user *buffer, size_t length, loff_t* offset);
static ssize_t	write_proc(struct file *filp, const char __user *buff, size_t len, loff_t* off);

/*************** Game of Life algorithm **********************/
void calculate_new_state(void);
void calculate_statistics(void);


static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = etx_read,
        .write          = etx_write,
        .open           = etx_open,
        .unlocked_ioctl = etx_ioctl, // add ioctl function to driver
        .release        = etx_release,
};

static struct proc_ops proc_fops = {
        .proc_open    = open_proc,
        .proc_read    = read_proc,
        .proc_write   = write_proc,
        .proc_release = release_proc
};

void calculate_new_state(void) 
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

static int open_proc(struct inode *inode, struct file *file)
{
    pr_info("proc file opend.....\t");
    return 0;
}

static int release_proc(struct inode *inode, struct file *file)
{
    pr_info("proc file released.....\n");
    return 0;
}

static ssize_t read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset)
{
    pr_info("proc read...\n");
    
    if(len)
        len=0;
    else
    {
        len=1;
        return 0;
    }
    
    if( copy_to_user(buffer, statistic, 21) )
    {
        pr_err("Data Send : Err!\n");
    }
 
    return length;
}

static ssize_t write_proc(struct file *filp, const char __user *buff, size_t len, loff_t * off)
{
    pr_info("write proc...\n");
    
    if( copy_from_user(input, buff, 2) )
    {
        pr_err("Data Write : Err!\n");
    }
    
    calculate_statistics();
    
    return len;
}

static int etx_open(struct inode *inode, struct file *filp)
{
        pr_info("Device File Opened...!!!\n");
        return 0;
}

static int etx_release(struct inode *inode, struct file *filp)
{
        pr_info("Device File Closed...!!!\n");
        return 0;
}

static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off)
{
        pr_info("Read function\n");
        return 0;
}

static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        pr_info("Write Function\n");
        return 0;
}

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
 
void calculate_statistics(void)
{    
    uint8_t susjed1 = (input[0] & 0x01);
    uint8_t susjed2 = (input[0] & 0x02) >> 1;
    uint8_t susjed3 = (input[0] & 0x04) >> 2;
    uint8_t susjed4 = (input[0] & 0x08) >> 3;
    uint8_t celija  = (input[0] & 0x10) >> 4;
    uint8_t susjed5 = (input[0] & 0x20) >> 5;
    uint8_t susjed6 = (input[0] & 0x40) >> 6;
    uint8_t susjed7 = (input[0] & 0x80) >> 7;
    uint8_t susjed8 = (input[1] & 0x01);
    
    int alive_neighbors = susjed1 + susjed2 + susjed3 + susjed4 +
                          susjed5 + susjed6 + susjed7 + susjed8;
       
    int alive_cells = alive_neighbors + celija;
        
    if (celija == ALIVE)
    {
        // Živa ćelija sa manje od 2 živa suseda ili vise od 3 ziva susjeda umire
        if (alive_neighbors < 2 || alive_neighbors > 3){
            newState = DEAD;
            diedCount++;
        }
		    
        // Živa ćelija sa 2 ili 3 živa suseda preživljava.
        else if (alive_neighbors == 2 || alive_neighbors == 3)
            newState = ALIVE;
    }
    else
    {
        // Mrtva ćelija sa tačno 3 živa suseda oživljava
        if (alive_neighbors == 3){
            newState = ALIVE;
            bornCount++;
        }
        else
            newState = DEAD;
    }
    generationCount++;
    
    statistic[19] = newState+'0';
    statistic[18] = '=';
    statistic[17] = 's';
    statistic[16] = 'n';
    statistic[15] = ' ';
    statistic[14] = alive_cells+'0';
    statistic[13] = '=';
    statistic[12] = 'a';
    statistic[11] = ' ';
    statistic[10] = bornCount+'0';
    statistic[9] = '=';
    statistic[8] = 'b';
    statistic[7] = ' ';
    statistic[6] = diedCount+'0';
    statistic[5] = '=';
    statistic[4] = 'd';
    statistic[3] = ' ';
    statistic[2] = generationCount+'0';
    statistic[1] = '=';
    statistic[0] = 'g';
}

static int __init etx_driver_init(void)
{
        /*Allocating Major number*/
        if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0)
        {
                pr_info("Cannot allocate major number\n");
                return -1;
        }
        pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
        /*Creating cdev structure*/
        cdev_init(&etx_cdev,&fops);
 
        /*Adding character device to the system*/
        if((cdev_add(&etx_cdev,dev,1)) < 0)
        {
            pr_info("Cannot add the device to the system\n");
            goto r_class;
        }
 
        /*Creating struct class*/
        if(IS_ERR(dev_class = class_create("etx_class")))
        {
            pr_info("Cannot create the struct class\n");
            goto r_class;
        }
 
        /*Creating device*/
        if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"procfs_device")))
        {
            pr_info("Cannot create the Device 1\n");
            goto r_device;
        }
        
        /*Create proc directory. It will create a directory under "/proc" */
        parent = proc_mkdir("etx",NULL);
        
        if( parent == NULL )
        {
            pr_info("Error creating proc entry - unable to create a directory under /proc");
            goto r_device;
        }
        
        /*Creating Proc entry under "/proc/etx/" */
        proc_create("etx_proc", 0666, parent, &proc_fops);
        
        pr_info("Created Procfs Entry /proc/etx/etx_proc \n");
 
 
        pr_info("Device Driver Insert...Done!!!\n");
        return 0;
 
r_device:
        class_destroy(dev_class);
r_class:
        unregister_chrdev_region(dev,1);
        return -1;
}

static void __exit etx_driver_exit(void)
{
    proc_remove(parent);  // remove complete /proc/etx
        
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
MODULE_DESCRIPTION("Simple Linux device driver (procfs)");
MODULE_VERSION("1.6");

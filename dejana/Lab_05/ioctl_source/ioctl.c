/***************************************************************************//**
*  \file       ioctl.c
*
*  \details    Simple Linux device driver (IOCTL) for Conway game of life
*
*  \author     EmbeTronicX/sdejana
*
*  \Tested with Linux raspberrypi 4.19.71-rt24-v7+
*
*******************************************************************************/
/* **************************************************************************
      very sad i have to write comments in english, but it is what it is
   **************************************************************************
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>                 /* kmalloc() */
#include<linux/uaccess.h>              /* copy_to/from_user() */
#include <linux/ioctl.h>
#include <linux/err.h>
 

#define SIZE 3
#define INT_SIZE_STATES 16

#define WR_VALUE _IOW('a','a',uint16_t*)
#define RD_VALUE _IOR('a','b',int32_t*)

#define SET_BIT(value, bit)  ((value) |= (1 << (bit)))
#define CLEAR_BIT(value, bit) ((value) &= ~(1 << (bit)))
#define CHECK_BIT(value, bit)  (((value) >> (bit)) & 1)
 
uint16_t cell_states = 1; 
/* 
        I store cell states in an array, first bit will be 'error bit',
        the rest of them will keep value 1 or 0 which will indicate the state of that cell; 
        like this:
        [1][2][3]
        [4][5][6]
        [7][8][9]
        each index in array corresponds to an cell

        so:
        cell_array_initialized[0] = error bit (1 - error occured, 0 it didn't)
*/
int8_t central_cell_state = -1;
int8_t cell_array_initialized = 0;

char binary_representation_cell_states[INT_SIZE_STATES + 1];
 
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
static void     convert_to_binary (void);
static int      count_alive_neighbours(void);
static int      calculate_new_state(void);


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
**  User defined functions (additional)
*/

/*
** This function will convert cell state in binary, in form of character array,
** in order to write states.
 */
void convert_to_binary ()
{
        int i = INT_SIZE_STATES - 1;
        for(; i>=0; i--)
        {
                if(cell_states & (1 << i))
                {
                        binary_representation_cell_states[INT_SIZE_STATES - 1 - i] = '1';
                }
                else 
                {
                        binary_representation_cell_states[INT_SIZE_STATES - 1 - i] = '0';
                }
        }
        binary_representation_cell_states[INT_SIZE_STATES] = '\0';
}

int count_alive_neighbours ()
{
        /* */
        int alive_neighbours_counting = 0;
        int i = 1;

        for( ;i <= SIZE * SIZE; i++)
        {
                if(i == 5)
                {
                        continue;
                }
                if(CHECK_BIT(cell_states, i))
                {
                        alive_neighbours_counting++;
                }
        }
        return alive_neighbours_counting;
}

int calculate_new_state ()
{
        int alive_neighbours = count_alive_neighbours();
        
        /* 
        bit meaning:  [e] [1][2][3][4] [5] [6][7][8][9]
        bit position: |0| |1||2||3||4| |5| |6||7||8||9|                           
        */
        central_cell_state = CHECK_BIT(cell_states, 5);
        if(central_cell_state)
        {
                if(alive_neighbours < 2 || alive_neighbours > 3)
                {
                        return 0; /* cell dies from loneliness or overpopulation*/
                }
                return 1; /* cell lives */
        }
        else
        {
                if(alive_neighbours == 3)
                {
                        return 1; /* cell revives */
                }
                return 0; /* cell stays dead */
        }
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
        uint16_t input_state_array = 0;
        int8_t new_state = 0;

         switch(cmd) {
                case WR_VALUE:
                        if( copy_from_user(&input_state_array ,(uint16_t*) arg, sizeof(input_state_array)) )
                        {
                                pr_err("Data Write : Err!\n");
                                break;
                        }
                        

                        cell_states = input_state_array;
                        if(cell_states > 511)
                        {
                                SET_BIT(cell_states, 0);
                                pr_err("Invalid cell state(s) or error occured.\n");
                        }
                        cell_array_initialized = 1; /* states are initialized */
                        CLEAR_BIT(cell_states, 0); /* clear error bit */
                        convert_to_binary();
                        pr_info("Cell states written successfully. Their value: %s \n", binary_representation_cell_states);

                case RD_VALUE:
                /* solving problem about trying to read uninitialized state of cells */
                        if(cell_array_initialized == 0)
                        {
                                pr_err("Cell states are not initialized. \n"); 
                                return -EFAULT;
                        }
                        if(CHECK_BIT(cell_states, 0) == 1)
                        {
                                pr_err("Error occured. Error bit is set to '1'. \n");
                                return -EFAULT;
                        }

                        new_state = calculate_new_state();
                        if(new_state)
                        {
                                SET_BIT(cell_states, SIZE);
                        }
                        else
                        {
                                CLEAR_BIT(cell_states, 5);
                        }

                        if( copy_to_user((int32_t*) arg, &new_state, sizeof(new_state)) )
                        {
                                pr_err("Data Read : Err!\n");
                        }
                        break;

                        pr_info("Succesfully updated central cell state, cell is %s", new_state > 0 ? "alive" : "dead");
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
MODULE_AUTHOR("smiljanicdejana02@gmail.com");
MODULE_DESCRIPTION("Simple Linux device driver (IOCTL), used for conway game-of-life cells manipulation. ");
MODULE_VERSION("1.5");

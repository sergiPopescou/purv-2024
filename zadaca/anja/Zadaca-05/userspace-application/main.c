#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
 
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)
 
int main()
{
 
    /* OPENING DRIVER FILE */
    printf("Opening Device Driver file.\n");
    int fd = open("/dev/etx_device", O_RDWR);
    if(fd < 0)
    {
        printf("ERROR: Cannot open device file...\n");
        return -1;
    }
    printf("Device opened successfully!\n");
 

 
    /* WRITTING */
    int32_t i = 0;
    int32_t states = 0x00000000;
    unsigned char input[10];
    
    printf("Enter state for 9 cells (ALIVE-1   DEAD-0): ");
    while(i < 9)
        input[i++] = getchar();
        
    
    states = (input[0] - '0') << 8 | (input[1] - '0') << 7 | (input[2] - '0') << 6 |  
             (input[3] - '0') << 5 | (input[4] - '0') << 4 | (input[5] - '0') << 3 |
             (input[6] - '0') << 2 | (input[7] - '0') << 1 | input[8] - '0';
    
    printf("Writing Value to Driver\n");
    ioctl(fd, WR_VALUE, &states); 
 
 
 
    /* READING */
    int32_t value = 0;
    printf("Reading Value from Driver\n");
    ioctl(fd, RD_VALUE, &value);
    
    printf("Value is %d\n", value);
 
 
    /* CLOSING DRIVER FILE */
    close(fd);
    printf("Device closed successfully\n");
}






















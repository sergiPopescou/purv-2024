#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

#define WR_VALUE _IOW('a', 'a', uint16_t*)
#define RD_VALUE _IOR('a', 'b', int32_t*)

int main() {
    int fd;
    uint16_t cell_states = 0b001010101; 
    /* 
    this equals:
        [0][1][0]
        [1][1][0]
        [1][0][1]  
    central cell should die
    */
    int32_t central_cell_state = 0;

    printf("Opening device...\n");
    fd = open("/dev/etx_device", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }
    printf("Device opened successfully.\n");

    printf("Writing cell states...\n");
    if (ioctl(fd, WR_VALUE, &cell_states) < 0) {
        perror("Failed to write cell states");
        close(fd);
        return -1;
    }
    printf("Cell states written successfully.\n");

    printf("Reading central cell state...\n");
    if (ioctl(fd, RD_VALUE, &central_cell_state) < 0) {
        perror("Failed to read central cell state");
        close(fd);
        return -1;
    }
    printf("Central cell state: %s\n", central_cell_state ? "Alive" : "Dead");

    close(fd);
    printf("Device closed successfully.\n");
    return 0;
}

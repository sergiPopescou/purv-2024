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
    uint16_t cell_states = 0b0010101000;
    char input[10];

    printf("Unesite stanje celija: devet bita tacno: ");
    scanf("%9s", input);
    printf("Debug: Uneseni string: %s\n", input);
    input[9] = '\0';
    for(int i = 0; i < 9; i++)
    {
	if(input[i] != '0' && input[i] != '1')
		{
			return -1;
		}
	cell_states = (cell_states << 1) | (input[i] - '0');
    }
    
    printf("Uneseni biti u decimalnom broju: 0b%09u\n", cell_states);
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

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

static void pabort(const char* s)
{
    printf("Aborting with %s\n", s);
    abort();
}


int main(int argc, char *argv[])
{
    printf("Starting\n");
    // Default parameters
    char *device = "/dev/spidev0.0";
    uint8_t mode=0;
    uint8_t bits = 8;
    uint32_t speed = 500000;
    uint16_t delay=0;
    uint8_t spi_mode = SPI_MODE_0;


    int ret = 0;
    int fd;

    printf("Opening device %s\n", device);
    fd = open(device, O_RDWR);
    if (fd < 0) {
        pabort("can't open device");
    }

    // Set mode of SPI device
    ret = ioctl(fd, SPI_IOC_WR_MODE, &spi_mode);
    if (ret == -1) {
        pabort("can't set spi mode");
    }

    ret = ioctl(fd, SPI_IOC_RD_MODE, &spi_mode);
    if (ret == -1) {
        pabort("can't get spi mode");
    }

    /*
     * bits per word
     */
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("can't set bits per word");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("can't get bits per word");

    /*
     * max speed hz
     */
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("can't set max speed hz");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("can't get max speed hz");

    printf("Mode: %i, Bits: %i, Speed: %i, Delay: %i\n", mode, bits, speed, delay);

    close(fd);
}

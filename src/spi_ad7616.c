#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

#include <pigpio.h>

#define RESETPin 23         // Broadcom pin 23 (Pi pin 16)

#define ADC_BUSY_Pin 24     // Broadcom pin 24 (Pi pin 18)
#define ADC_CONVST_Pin 25   // Broadcom pin 25 (Pi pin 22)
#define ADC_SER1W_Pin 4     // Broadcom pin 4  (Pi pin 7)

#define SPI0_CS0_Pin 8      // Broadcom pin 8  (Pi pin 24)
#define SPI0_CS1_Pin 7      // Broadcom pin 7  (Pi pin 26)
#define SPI0_SCLK_Pin 11    // Broadcom pin 11 (Pi pin 23)
#define SPI0_MOSI_Pin 10    // Broadcom pin 10 (Pi pin 19)
#define SPI0_MISO_Pin 9     // Broadcom pin 9  (Pi pin 21)  Also called by the ADC SDOA.
#define ADC_SDOB_Pin 22     // Broadcom pin 22 (Pi pin 15)

#define SPI1_CS0_Pin 18     // Broadcom pin 18 (Pi pin 12)
#define SPI1_CS1_Pin 17     // Broadcom pin 17 (Pi pin 11)
#define SPI1_SCLK_Pin 21    // Broadcom pin 21 (Pi pin 40)
#define SPI1_MOSI_Pin 20    // Broadcom pin 20 (Pi pin 38)
#define SPI1_MISO_Pin 19    // Broadcom pin 19 (Pi pin 35)

typedef struct {
    unsigned spi_cs_pin;
    unsigned spi_sclk_pin;
    unsigned spi_mosi_pin;
    unsigned spi_miso_pin;

    unsigned max_speed_hz;
    unsigned min_period_usec;
    unsigned mode;
} self_t;

void initialize(self_t* self)
{
    // Default to bus 0, device 0
    self->spi_cs_pin = SPI0_CS0_Pin;
    self->spi_sclk_pin = SPI0_SCLK_Pin;
    self->spi_mosi_pin = SPI0_MOSI_Pin;
    self->spi_miso_pin = SPI0_MISO_Pin;

    self->max_speed_hz = 100000;
    self->min_period_usec = 1000000 / self->max_speed_hz;
    self->mode = 0;   // 0000 00<polarity><phase>  2 LSBs determine polarity and phase of clock.

    printf("Resetting the A/D");
    gpioSetMode(RESETPin, PI_OUTPUT);
    gpioSetMode(ADC_SER1W_Pin, PI_OUTPUT);

    gpioWrite(ADC_SER1W_Pin, 1);        // 0 for 1-wire, 1 for 2-wire (doesn't seem to work, always 2-wire)
    usleep(100);
    gpioWrite(RESETPin, 0);
    usleep(100);
    gpioWrite(RESETPin, 1);
    usleep(100);
}

void spi_idle(self_t* self)
{
    // Set defaults for output pins
    gpioWrite(ADC_CONVST_Pin, 0);
    gpioWrite(self->spi_cs_pin, 1);
    gpioWrite(self->spi_sclk_pin, 1);
    gpioWrite(self->spi_mosi_pin, 0);
}

void spi_open(self_t* self, unsigned bus, unsigned device)
{
    if (bus == 0)
    {
        self->spi_cs_pin = (device == 0) ? SPI0_CS0_Pin : SPI0_CS1_Pin;
        self->spi_sclk_pin = SPI0_SCLK_Pin;
        self->spi_mosi_pin = SPI0_MOSI_Pin;
        self->spi_miso_pin = SPI0_MISO_Pin;
    }
    else
    {
        self->spi_cs_pin = (device == 0) ? SPI1_CS0_Pin : SPI1_CS1_Pin;
        self->spi_sclk_pin = SPI1_SCLK_Pin;
        self->spi_mosi_pin = SPI1_MOSI_Pin;
        self->spi_miso_pin = SPI1_MISO_Pin;
    }

    gpioSetMode(ADC_BUSY_Pin, PI_INPUT);
    gpioSetMode(ADC_CONVST_Pin, PI_OUTPUT);
    gpioSetMode(self->spi_cs_pin, PI_OUTPUT);
    gpioSetMode(self->spi_sclk_pin, PI_OUTPUT);
    gpioSetMode(self->spi_mosi_pin, PI_OUTPUT);
    gpioSetMode(self->spi_miso_pin, PI_INPUT);
    gpioSetMode(ADC_SDOB_Pin, PI_INPUT);

    spi_idle(self);
}

void xfer2(self_t* self, unsigned count, unsigned short* outbuffer, unsigned short* inbuffer0, unsigned short* inbuffer1)
{
    // Always start with a conversion.
    printf("Starting transaction with a conversion\n");
    gpioWrite(ADC_CONVST_Pin, 1);
    //while (gpioRead(ADC_BUSY_Pin) == 1)
    //    usleep(1);
    gpioWrite(ADC_CONVST_Pin, 0);
    while (gpioRead(ADC_BUSY_Pin) != 0)
        usleep(1);

    printf("Selecting ADC board for transaction with sclk period %u us\n", self->min_period_usec);
    struct timespec tpStart;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tpStart);
    clock_t start = clock();

    gpioWrite(self->spi_mosi_pin, 1);

    for (unsigned _ = 0; _ < count; _++)
    {
        gpioWrite(self->spi_cs_pin, 0);

        unsigned value = *outbuffer;
        printf("Sending: %04x", value);
        unsigned short result0 = 0;
        unsigned short result1 = 0;

        unsigned short bitmask = 1 << 15;
        for (unsigned __ = 0; __ < 16; __++)
        {
            unsigned bit_setting = (value & bitmask) != 0 ? 1 : 0;
            gpioWrite(self->spi_mosi_pin, bit_setting);
            gpioWrite(self->spi_sclk_pin, 0);
            //usleep(self->min_period_usec / 2);
            if (gpioRead(self->spi_miso_pin) != 0)
                result0 |= bitmask;
            if (gpioRead(ADC_SDOB_Pin) != 0)
                result1 |= bitmask;
            gpioWrite(self->spi_sclk_pin, 1);
            //usleep(self->min_period_usec / 2);

            bitmask = bitmask >> 1;
        }
        gpioWrite(self->spi_cs_pin, 1);

        *inbuffer0 = result0;
        *inbuffer1 = result1;
        printf("   Receiving   0: %04x  1: %04x\n", result0, result1);

        outbuffer++;
        inbuffer0++;
        inbuffer1++;
    }

    //spi_idle(self);

    struct timespec tpEnd;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tpEnd);
    clock_t end = clock();
	long t_diff = ((tpEnd.tv_sec-tpStart.tv_sec)*(1000*1000*1000) + (tpEnd.tv_nsec-tpStart.tv_nsec)) / 1000 ;

    double elapsed = (double)(end - start);
    printf("Deselecting ADC board, transaction done in %lf ms, %lu us\n\n", elapsed * 1000.0 / (double)CLOCKS_PER_SEC, t_diff);
}
/*
*/
int main(int argc, char *argv[])
{
   printf("Starting\n");

   if (gpioInitialise()<0) return 1;
   printf("Initialized pigpio\n");

   self_t self;
   initialize(&self);
   spi_open(&self, 1, 0);
   self.max_speed_hz = 10000;
   self.mode = 0;   // 0000 00<polarity><phase>  2 LSBs determine polarity and phase of clock.

    unsigned short outbuffer[16];
    unsigned short inbuffer0[16];
    unsigned short inbuffer1[16];

    // Change channel register to select the self-test channel (0xbbbb and 0x5555)
    outbuffer[0] = 0x86bb;
    xfer2(&self, 1, outbuffer, inbuffer0, inbuffer1);

    // Read all registers back, just for display
    outbuffer[0] = 0x0400;
    outbuffer[1] = 0x0600;
    outbuffer[2] = 0x0800;
    outbuffer[3] = 0x0a00;
    outbuffer[4] = 0x0c00;
    outbuffer[5] = 0x0e00;
    outbuffer[6] = 0x0e00;
    xfer2(&self, 7, outbuffer, inbuffer0, inbuffer1);

    // Read the selected self-test conversion a few times
    outbuffer[0] = 0x0000;
    outbuffer[1] = 0x0000;
    outbuffer[2] = 0x0000;
    xfer2(&self, 3, outbuffer, inbuffer0, inbuffer1);
    
    // Change channel register to select the channel 0 for both A and B
    outbuffer[0] = 0x8600;
    xfer2(&self, 1, outbuffer, inbuffer0, inbuffer1);

    outbuffer[0] = 0x0000;
    outbuffer[1] = 0x0000;
    outbuffer[2] = 0x0000;
    outbuffer[3] = 0x0000;
    xfer2(&self, 4, outbuffer, inbuffer0, inbuffer1);

    // Set up a sequence to read all 8 pairs of registers.  Enable burst mode (all conversion with one CONVST) and sequnce mode (sequencer enabled)
    outbuffer[0] = 0xc000;
    outbuffer[1] = 0xc211;
    outbuffer[2] = 0xc422;
    outbuffer[3] = 0xc633;
    outbuffer[4] = 0xc844;
    outbuffer[5] = 0xca55;
    outbuffer[6] = 0xcc66;
    outbuffer[7] = 0xcf77;
    outbuffer[8] = 0x8460;
    xfer2(&self, 9, outbuffer, inbuffer0, inbuffer1);

    // Read the selected conversion continuously
    while (1)
    {
        outbuffer[0] = 0x0000;
        outbuffer[1] = 0x0000;
        outbuffer[2] = 0x0000;
        outbuffer[3] = 0x0000;
        outbuffer[4] = 0x0000;
        outbuffer[5] = 0x0000;
        outbuffer[6] = 0x0000;
        outbuffer[7] = 0x0000;
        xfer2(&self, 8, outbuffer, inbuffer0, inbuffer1);
        usleep(500000);
    }

   gpioTerminate();
}
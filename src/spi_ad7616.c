//
// Low-level bit-banged SPI interface to the AD7616 ADC chip.
// If this is written in Python, it is too slow.  Much too slow.
// If either Python or C/C++ use the SPI drivers at the O/S level,
// they are limited to 8-bit transfer.  The AD7616 needs 32-bit
// transfers in order to pack two 16-bit conversions into each read.
//
// Build with
//gcc -Wall -pthread -o spi spi_ad7616.c -lpigpio -lrt
//
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
} self_t;

int spi_initialize(self_t* self)
{
    int returnValue = gpioInitialise();
    if (returnValue < 0)
    {
        printf("Initialization of pigpio failed with error %d\n", returnValue);
        return returnValue;
    }
    printf("Initialized pigpio\n");

    // Default to bus 1, device 0
    self->spi_cs_pin = SPI1_CS0_Pin;
    self->spi_sclk_pin = SPI1_SCLK_Pin;
    self->spi_mosi_pin = SPI1_MOSI_Pin;
    self->spi_miso_pin = SPI1_MISO_Pin;

    printf("Resetting the A/D");
    gpioSetMode(RESETPin, PI_OUTPUT);
    gpioSetMode(ADC_SER1W_Pin, PI_OUTPUT);

    gpioWrite(ADC_SER1W_Pin, 0);        // 0 for 1-wire, 1 for 2-wire (doesn't seem to work, always 2-wire)
    usleep(100);
    gpioWrite(RESETPin, 0);
    usleep(100);
    gpioWrite(RESETPin, 1);
    usleep(100);

    return 0;
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

void spi_terminate(self_t* self)
{
    gpioTerminate();
}

void spi_writeregister(self_t* self, unsigned address, unsigned value)
{
    // Always start with a conversion.
    printf("Starting Write to register %d (%d) with a conversion\n", address, value);
    gpioWrite(ADC_CONVST_Pin, 1);
    gpioWrite(ADC_CONVST_Pin, 0);
    while (gpioRead(ADC_BUSY_Pin) != 0)
        usleep(1);

    // Instrument for elapsed time.
    struct timespec tpStart;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tpStart);
    clock_t start = clock();

    gpioWrite(self->spi_mosi_pin, 1);
    gpioWrite(self->spi_cs_pin, 0);

    unsigned result = 0;
    unsigned bitmask = 1 << 15;
    unsigned senddata = ((address & 0x3f) | 0x40) << 9 | (value & 0x1ff);

    gpioWrite(self->spi_cs_pin, 0);
    for (unsigned _ = 0; _ < 16; _++)
    {
        unsigned bit_setting = (senddata & bitmask) != 0 ? 1 : 0;
        gpioWrite(self->spi_mosi_pin, bit_setting);
        gpioWrite(self->spi_sclk_pin, 0);
        if (gpioRead(self->spi_miso_pin) != 0)
            result |= bitmask;
        gpioWrite(self->spi_sclk_pin, 1);

        bitmask = bitmask >> 1;
    }
    gpioWrite(self->spi_cs_pin, 1);

    // Instrument for elapsed time.
    struct timespec tpEnd;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tpEnd);
    clock_t end = clock();
    double elapsed = (double)(end - start);
	long tpElapsed = ((tpEnd.tv_sec-tpStart.tv_sec)*(1000*1000*1000) + (tpEnd.tv_nsec-tpStart.tv_nsec)) / 1000 ;

    printf("Register write used %lf ms CPU, done in %lu us\n\n", elapsed * 1000.0 / (double)CLOCKS_PER_SEC, tpElapsed);
}

unsigned spi_readregister(self_t* self, unsigned address)
{
    unsigned result = 0;
    unsigned bitmask = 1 << 15;
    unsigned senddata = (address & 0x3f) << 9;

    gpioWrite(self->spi_cs_pin, 0);
    for (unsigned _ = 0; _ < 16; _++)
    {
        unsigned bit_setting = (senddata & bitmask) != 0 ? 1 : 0;
        gpioWrite(self->spi_mosi_pin, bit_setting);
        gpioWrite(self->spi_sclk_pin, 0);
        if (gpioRead(self->spi_miso_pin) != 0)
            result |= bitmask;
        gpioWrite(self->spi_sclk_pin, 1);

        bitmask = bitmask >> 1;
    }
    gpioWrite(self->spi_cs_pin, 1);

    spi_idle(self);

    printf("Read register %d: %04x\n", address, result);
    return result;
}

void spi_readreg(self_t* self, unsigned count, unsigned* addresses, unsigned* values)
{
    // Always start with a conversion.
    printf("Starting Read from %d registers\n", count);
    gpioWrite(ADC_CONVST_Pin, 1);
    gpioWrite(ADC_CONVST_Pin, 0);
    while (gpioRead(ADC_BUSY_Pin) != 0)
        usleep(1);

    gpioWrite(self->spi_mosi_pin, 1);
    gpioWrite(self->spi_cs_pin, 0);

    unsigned* registeraddress = addresses;
    unsigned* registervalue = values;
    int throwaway = 1;
    for (unsigned registerIndex = 0; registerIndex < count; registerIndex++, registeraddress++)
    {
        unsigned value = spi_readregister(self, *registeraddress);
        if (!throwaway)
        {
            *registervalue = value;
            registervalue++;
        }
        throwaway = 0;
    }

    // We retrieve the last register value with a no-op read.
    *registervalue = spi_readregister(self, 0);
    registervalue++;
}

void spi_readconversion(self_t* self, unsigned count, unsigned* conversions)
{
    // Always start with a conversion.
    gpioWrite(ADC_CONVST_Pin, 1);
    gpioWrite(ADC_CONVST_Pin, 0);
    while (gpioRead(ADC_BUSY_Pin) != 0)
        usleep(1);

    // Instrument for elapsed time.
    struct timespec tpStart;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tpStart);
    clock_t start = clock();

    gpioWrite(self->spi_mosi_pin, 1);
    gpioWrite(self->spi_cs_pin, 0);

    unsigned* conversion = conversions;
    for (unsigned _ = 0; _ < count; _++)
    {
        unsigned result = 0;
        unsigned bitmask = 1 << 31;

        gpioWrite(self->spi_mosi_pin, 0);
        for (unsigned __ = 0; __ < 32; __++)
        {
            gpioWrite(self->spi_sclk_pin, 0);
            if (gpioRead(self->spi_miso_pin) != 0)
                result |= bitmask;
            gpioWrite(self->spi_sclk_pin, 1);

            bitmask = bitmask >> 1;
        }

        *conversion = result;
        conversion++;
    }

    spi_idle(self);

    // Instrument for elapsed time.
    struct timespec tpEnd;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tpEnd);
    clock_t end = clock();
    double elapsed = (double)(end - start);
	long tpElapsed = ((tpEnd.tv_sec-tpStart.tv_sec)*(1000*1000*1000) + (tpEnd.tv_nsec-tpStart.tv_nsec)) / 1000 ;

    printf("%d conversions used %lf ms CPU, done in %lu us\n\n", count, elapsed * 1000.0 / (double)CLOCKS_PER_SEC, tpElapsed);
}

void spi_definesequence(self_t* self, unsigned count, unsigned* Achannels, unsigned* Bchannels)
{
    if (count > 32)
    {
        printf("spi_definesequence cannot define a sequence with %d elements, 32 max\n", count);
        return;
    }

    unsigned sequencer = 0x20;

    for (unsigned i = 0; i < count; i++, sequencer++, Achannels++, Bchannels++)
    {
        unsigned ssren = 0;
        if (i + 1 == count)
            ssren = 0x100;

        unsigned channeldata = (*Bchannels & 0xf) << 4 | (*Achannels & 0xf) | ssren;
        spi_writeregister(self, sequencer, channeldata);
    }

    // Read the configuration register, set BURSTEN and SEQEN, write it back.
    unsigned configuration = spi_readregister(self, 2);
    configuration |= (0x40 | 0x20);     // BURSTEN with SEQEN.
    spi_writeregister(self, 2, configuration);
}

unsigned spi_convertpair(self_t* self, unsigned channelA, unsigned channelB)
{
    unsigned channeldata = (channelB & 0xf) << 4 | (channelA & 0xf);
    spi_writeregister(self, 3, channeldata);

    unsigned conversion;
    spi_readconversion(self, 1, &conversion);

    return conversion;
}

/*
*/
int main(int argc, char *argv[])
{
    printf("Starting\n");

    self_t self;
    if (spi_initialize(&self) < 0) return 1;
    spi_open(&self, 1, 0);

    unsigned  outbuffer[16];
    unsigned  inbuffer[16];

    spi_writeregister(&self, 4, 0x55);
    spi_writeregister(&self, 5, 0x55);
    spi_writeregister(&self, 6, 0x55);
    spi_writeregister(&self, 7, 0x55);
    spi_writeregister(&self, 3, 0x00);

    outbuffer[0] = 0x02;
    outbuffer[1] = 0x03;
    outbuffer[2] = 0x04;
    outbuffer[3] = 0x05;
    outbuffer[4] = 0x06;
    outbuffer[5] = 0x07;
    spi_readreg(&self, 6, outbuffer, inbuffer);

    {
        unsigned channelA = 0;
        unsigned channelB = 0;
        printf("Reading conversion pair (A%d, B%d)\n", channelA, channelB);
        unsigned conversion = spi_convertpair(&self, channelA, channelB);
        unsigned aconv = (conversion >> 16) & 0x0000ffff;
        aconv = (aconv + 0x8000) & 0x0000ffff;
        unsigned bconv = (conversion & 0x0000ffff);
        bconv = (bconv + 0x8000) & 0x0000ffff;
        printf("Channel %dA = %d (%04x),  %dB = %d (%04x)\n", channelA, aconv, aconv, channelB, bconv, bconv);
    }

    printf("Defining conversion sequence\n");
    unsigned Achannels[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    unsigned Bchannels[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    spi_definesequence(&self, 8, Achannels, Bchannels);

    for (int _ = 0; _ < 50; _++)
    {
        spi_readconversion(&self, 8, inbuffer);

        for (int i = 0; i < 8; i++)
        {
            unsigned conversion = inbuffer[i];

            unsigned aconv = (conversion >> 16) & 0x0000ffff;
            aconv = (aconv + 0x8000) & 0x0000ffff;
            unsigned bconv = (conversion & 0x0000ffff);
            bconv = (bconv + 0x8000) & 0x0000ffff;
            printf("Channel %dA = %d (%04x),  %dB = %d (%04x)\n", i, aconv, aconv, i, bconv, bconv);
        }

        usleep(100000);
    }

    spi_terminate(&self);
}

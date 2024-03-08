//
// Manage the low-level bit-banged SPI interface to the AD7616.
// While something close to the SPI interface is used, there are
// possible pathways to read information from the chip, both of 
// which are difficult or impossible with the Raspberry Pi hardware.
// 1. 2-wire SPI.  The AD7616 A/D chip is really two A/D converters
//    operating in parallel.  The fastest way to get the conversion
//    values out of the chip using SPI is to signal it to send the
//    A side conversions over one MISO pin, and the B side conversions
//    over a second MISO pin.  This ability to use two MISO pins, both
//    clocked by the same SCLK, is not standard SPI, but can be 
//    eumulated with a bit-banged approach.
// 2. 1-wire SPI.  Alternatively, the A/D chip may be configured to send
//    both A side and B side conversions over a single MISO pin.  While
//    this is standard SPI, it requires using the SPI interface with
//    a 32-bit word length.  Unfortunately, due to limitations in either
//    the Raspberry Pi SPI hardware or the driver software, only 8-bit
//    word lengths are allowed.
//
// After best attempts to get either of the above modes to work with SPI
// hardware, it was determined that the best approach is to bit-bang four
// Raspberry Pi GPIO pins in software.  This was attempted in Python, but
// was 25-50 times too slow for the requirements.  Thus, the bit-bang
// layer is implemented in the C file.
//
// The strategy is to build this C file as a loadeable library, ad7616_driver.so,
// which can easily be called from either C, C++, or Python programs.
//
// To build on a Raspberry Pi, use this command in a terminal prompt after changing
// to the directory with this file in it:
//
//gcc -Wall -pthread -fpic -shared -o ad7616_driver.so ad7616_driver.c -lpigpio -lrt
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
#define SPI0_MISO_Pin 9     // Broadcom pin 9  (Pi pin 21)  Also called SDOA by the ADC.
#define ADC_SDOB_Pin 22     // Broadcom pin 22 (Pi pin 15)

#define SPI1_CS0_Pin 18     // Broadcom pin 18 (Pi pin 12)
#define SPI1_CS1_Pin 17     // Broadcom pin 17 (Pi pin 11)
#define SPI1_SCLK_Pin 21    // Broadcom pin 21 (Pi pin 40)
#define SPI1_MOSI_Pin 20    // Broadcom pin 20 (Pi pin 38)
#define SPI1_MISO_Pin 19    // Broadcom pin 19 (Pi pin 35)

//
// This type is the handle returned by spi_initialize(), and
// required by all subsequent calls.
//
typedef struct {
    unsigned spi_cs_pin;
    unsigned spi_sclk_pin;
    unsigned spi_mosi_pin;
    unsigned spi_miso_pin;
    unsigned spi_errorcode;
} self_t;

static self_t spidef = {};
static self_t spidefault = {0, 0, 0, 0, 0};

//
// A call to spi_nitialize() is required before any other call.
// Initialize memory and the GPIO library, and condition the chip for operation.
//
self_t spi_initialize()
{
    spidef = spidefault;                // Reset the handle to default values.

    // Before we can use pigpio, we have to initialize it.
    int errorcode = gpioInitialise();
    if (errorcode < 0)
    {
        printf("Initialization of pigpio failed with error %d\n", errorcode);
        spidef.spi_errorcode = errorcode;
        return spidef;
    }
    printf("Initialized pigpio\n");

    // Default to bus 1, device 0
    spidef.spi_cs_pin = SPI1_CS0_Pin;
    spidef.spi_sclk_pin = SPI1_SCLK_Pin;
    spidef.spi_mosi_pin = SPI1_MOSI_Pin;
    spidef.spi_miso_pin = SPI1_MISO_Pin;

    printf("Resetting the A/D\n");
    gpioSetMode(RESETPin, PI_OUTPUT);
    gpioSetMode(ADC_SER1W_Pin, PI_OUTPUT);

    gpioWrite(ADC_SER1W_Pin, 0);        // 0 for 1-wire, 1 for 2-wire (doesn't seem to work, always 2-wire)
    usleep(100);
    gpioWrite(RESETPin, 0);
    usleep(100);
    gpioWrite(RESETPin, 1);
    usleep(100);

    return spidef;
}

//
// Internal method used after public calls to ensure
// the GPIO pins controlling the SPI interface are
// returned to idle state.
//
static void spi_idle(self_t* self)
{
    // Set defaults for output pins
    gpioWrite(ADC_CONVST_Pin, 0);
    gpioWrite(self->spi_cs_pin, 1);
    gpioWrite(self->spi_sclk_pin, 1);
    gpioWrite(self->spi_mosi_pin, 0);
}

//
// Before using the AD7616 chip, the driver must be opened.
//
// Parameters:
// self: A copy of the opaque handle that was provided by spi_initialize().
// bus: The Raspberry Pi SPI hardware provides two SPI interfaces, bus 0 and 1.
// device: On the Raspberry Pi SPI hardware, bus 0 provides two Chip Select (CS-)
//         pins, allowing two devices to be addressed on bus 0.  Bus 1 provides
//         three CS pins.
//
// Returns: Nothing.
//
// Currently, only bus 1, CS 0 is supported for the AD7616 chip.
//
void spi_open(self_t self, unsigned bus, unsigned device)
{
    if (bus == 0)
    {
        self.spi_cs_pin = (device == 0) ? SPI0_CS0_Pin : SPI0_CS1_Pin;
        self.spi_sclk_pin = SPI0_SCLK_Pin;
        self.spi_mosi_pin = SPI0_MOSI_Pin;
        self.spi_miso_pin = SPI0_MISO_Pin;
    }
    else
    {
        self.spi_cs_pin = (device == 0) ? SPI1_CS0_Pin : SPI1_CS1_Pin;
        self.spi_sclk_pin = SPI1_SCLK_Pin;
        self.spi_mosi_pin = SPI1_MOSI_Pin;
        self.spi_miso_pin = SPI1_MISO_Pin;
    }

    gpioSetMode(ADC_BUSY_Pin, PI_INPUT);
    gpioSetMode(ADC_CONVST_Pin, PI_OUTPUT);
    gpioSetMode(self.spi_cs_pin, PI_OUTPUT);
    gpioSetMode(self.spi_sclk_pin, PI_OUTPUT);
    gpioSetMode(self.spi_mosi_pin, PI_OUTPUT);
    gpioSetMode(self.spi_miso_pin, PI_INPUT);
    gpioSetMode(ADC_SDOB_Pin, PI_INPUT);

    spi_idle(&self);
}

//
// When done using the AD7616 chip, call this method.  This will close everything
// and release the GPIO pins owned by the GPIO library.
//
void spi_terminate(self_t self)
{
    gpioTerminate();
}


//
// Write a single value to a single register.  The first step after
// initializing and opening this driver will be to configure the AD7616
// chip by setting the conversion ranges for all channels, plus anything
// else you need.
//
// Parameters:
// self: A copy of the opaque handle that was provided by spi_initialize().
// address: A valid register address (2-7 and 32-64) for a register within
//          the AD7616 chip.
// value:   The 9-bit value to write to the register.
//
// Returns: Nothing.
//
void spi_writeregister(self_t self, unsigned address, unsigned value)
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

    gpioWrite(self.spi_mosi_pin, 1);
    gpioWrite(self.spi_cs_pin, 0);

    unsigned result = 0;
    unsigned bitmask = 1 << 15;
    unsigned senddata = ((address & 0x3f) | 0x40) << 9 | (value & 0x1ff);

    gpioWrite(self.spi_cs_pin, 0);
    for (unsigned _ = 0; _ < 16; _++)
    {
        unsigned bit_setting = (senddata & bitmask) != 0 ? 1 : 0;
        gpioWrite(self.spi_mosi_pin, bit_setting);
        gpioWrite(self.spi_sclk_pin, 0);
        if (gpioRead(self.spi_miso_pin) != 0)
            result |= bitmask;
        gpioWrite(self.spi_sclk_pin, 1);

        bitmask = bitmask >> 1;
    }
    gpioWrite(self.spi_cs_pin, 1);

    // Instrument for elapsed time.
    struct timespec tpEnd;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tpEnd);
    clock_t end = clock();
    double elapsed = (double)(end - start);
	long tpElapsed = ((tpEnd.tv_sec-tpStart.tv_sec)*(1000*1000*1000) + (tpEnd.tv_nsec-tpStart.tv_nsec)) / 1000 ;

    printf("Register write used %lf ms CPU, done in %lu us\n\n", elapsed * 1000.0 / (double)CLOCKS_PER_SEC, tpElapsed);
}

//
// Read the value from a single register.  This is not frequently
// needed, other than to confirm previously-written values.
//
// Parameters:
// self: A copy of the opaque handle that was provided by spi_initialize().
// address: A valid register address (2-7 and 32-64) for a register within
//          the AD7616 chip.
//
// Returns: The 9-bit value read from the register.
//
unsigned spi_readregister(self_t self, unsigned address)
{
    unsigned result = 0;
    unsigned bitmask = 1 << 15;
    unsigned senddata = (address & 0x3f) << 9;

    gpioWrite(self.spi_cs_pin, 0);
    for (unsigned __ = 0; __ < 2; __++)
    {
        result = 0;
        bitmask = 1 << 15;
        for (unsigned _ = 0; _ < 16; _++)
        {
            unsigned bit_setting = (senddata & bitmask) != 0 ? 1 : 0;
            gpioWrite(self.spi_mosi_pin, bit_setting);
            gpioWrite(self.spi_sclk_pin, 0);
            if (gpioRead(self.spi_miso_pin) != 0)
                result |= bitmask;
            gpioWrite(self.spi_sclk_pin, 1);

            bitmask = bitmask >> 1;
        }
    }
    gpioWrite(self.spi_cs_pin, 1);

    spi_idle(&self);

    printf("Read register %d: %04x\n", address, result);
    return result;
}

//
// Read the values from multiple registers.  This is not frequently
// needed, other than to confirm previously-written values.
//
// Parameters:
// self: A copy of the opaque handle that was provided by spi_initialize().
// count: The size of the addresses and values arrays in unsigned short integers.
// addresses: A pointer to an array of valid register address (2-7 and 32-64) 
//            for registers within the AD7616 chip.
// values:    A pointer to an array of unsigned short integers to return the
//            register values in.
//
// NOTE: The memory for the addresses and values arrays is allocated by and owned
//       by the caller.  It is the caller's responsibility to ensure they are at
//       least as large as indicated by count.
//
// Returns: Nothing.
//
void spi_readregisters(self_t self, unsigned count, unsigned* addresses, unsigned* values)
{
    // Always start with a conversion.
    printf("Starting Read from %d registers\n", count);
    gpioWrite(ADC_CONVST_Pin, 1);
    gpioWrite(ADC_CONVST_Pin, 0);
    while (gpioRead(ADC_BUSY_Pin) != 0)
        usleep(1);

    gpioWrite(self.spi_mosi_pin, 1);
    gpioWrite(self.spi_cs_pin, 0);

    unsigned* registeraddress = addresses;
    unsigned* registervalue = values;
    for (unsigned registerIndex = 0; registerIndex < count; registerIndex++, registeraddress++)
    {
        unsigned value = spi_readregister(self, *registeraddress);
        *registervalue = value;
        registervalue++;
    }

}

//
// Tell the AD7616 A/D chip to perform a conversion operation, which may be a single
// A side and B side pair of values, or many A and B pairs, depending on whether 
// spi_definesequence has previously been called.
// It is up to the caller to ensure that the number of conversions read back,
// as controlled by the count parameter, is a match for the number of conversions
// the AD7616 chip is configured to perform in a single operation.
//
// Parameters:
// self: A copy of the opaque handle that was provided by spi_initialize().
// count: The size of the conversions array in unsigned short integers.
//        This will read and return count number of values from the chip.
//        If count is larger than the chip is configured to convert, most likely
//        all excess values will be copies of the last conversion, but there
//        are no guarantees, as this is undefined.
// conversions: A pointer to an array of unsigned short integers to return the
//            conversion values in.  Note that each value in the returned array
//            will be a 32-bit value containing an A side value and a B side value.
//
// NOTE: The memory for the conversions array is allocated by and owned
//       by the caller.  It is the caller's responsibility to ensure it is at
//       least as large as indicated by count.
//
// Returns: Nothing.
//
void spi_readconversion(self_t self, unsigned count, unsigned* conversions)
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

    gpioWrite(self.spi_mosi_pin, 1);
    gpioWrite(self.spi_cs_pin, 0);

    unsigned* conversion = conversions;
    for (unsigned _ = 0; _ < count; _++)
    {
        unsigned result = 0;
        unsigned bitmask = 1 << 31;

        gpioWrite(self.spi_mosi_pin, 0);
        for (unsigned __ = 0; __ < 32; __++)
        {
            gpioWrite(self.spi_sclk_pin, 0);
            if (gpioRead(self.spi_miso_pin) != 0)
                result |= bitmask;
            gpioWrite(self.spi_sclk_pin, 1);

            bitmask = bitmask >> 1;
        }

        *conversion = result;
        conversion++;
    }

    spi_idle(&self);

    // Instrument for elapsed time.
    struct timespec tpEnd;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tpEnd);
    clock_t end = clock();
    double elapsed = (double)(end - start);
	long tpElapsed = ((tpEnd.tv_sec-tpStart.tv_sec)*(1000*1000*1000) + (tpEnd.tv_nsec-tpStart.tv_nsec)) / 1000 ;

    printf("%d conversions used %lf ms CPU, done in %lu us\n\n", count, elapsed * 1000.0 / (double)CLOCKS_PER_SEC, tpElapsed);
}

//
// Define a sequence of channels to be converted by the AD7616 chip in a single conversion
// operation.  After calling this, any subsequent calls to spi_readconversion() should be called
// with the same value for count as that used in this call.  That is, this call will define
// the number of conversions produced by the chip when spi_readconversion() is controlled by
// this method, so the sizes of the arrays must be equal.
//
// Parameters:
// self: A copy of the opaque handle that was provided by spi_initialize().
// count: The size of the Achannels and Bchannels arrays in unsigned short integers.
//        This will configure the sequencer stack within the AD7616 chip, defining
//        count number of A side and B side conversion pairs.  The chip will then
//        perform the full sequence of conversions in hardware each time it is requested
//        to convert.
// Achannels: A pointer to an array of unsigned short integers that defines count
//            number of A side channels to convert.  These A side channels will be
//            combined pairwise with the B side channels in the Bchannels array,
//            so the chip will know how to convert both the A side and B side.
// Bchannels: A pointer to an array of unsigned short integers that defines count
//            number of B side channels to convert.  These B side channels will be
//            combined pairwise with the A side channels in the Achannels array,
//            so the chip will know how to convert both the A side and B side.
//
// NOTE: The memory for the Achannels and Bchannels arrays is allocated by and owned
//       by the caller.  It is the caller's responsibility to ensure they are at
//       least as large as indicated by count.
//
// Returns: Nothing.
//
void spi_definesequence(self_t self, unsigned count, unsigned* Achannels, unsigned* Bchannels)
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

//
// Perform a conversion on a single A side and B side channel pair.
//
// Warning: This method can only be used before any calls to spi_definesequence().
//          After calling spi_definesequence() at least once, the AD7616 chip will
//          be configured to take its channel from the sequencer stack registers,
//          rather than those provided in this call.
//
// Parameters:
// self: A copy of the opaque handle that was provided by spi_initialize().
// channelA: The A side channel to convert.  The A side channel will be
//            combined with the B side channel, so the chip will know how to 
//            convert both the A side and B side.
// channelB: The B side channel to convert.  The B side channel will be
//            combined with the A side channel, so the chip will know how to 
//            convert both the A side and B side.
//
// NOTE: The returned value will be a 32-bit value containing an A side value and a B side value.
//
// Returns: The converted A side and B side channels, with A side in the high word.
//
unsigned spi_convertpair(self_t self, unsigned channelA, unsigned channelB)
{
    unsigned channeldata = (channelB & 0xf) << 4 | (channelA & 0xf);
    spi_writeregister(self, 3, channeldata);

    unsigned conversion;
    spi_readconversion(self, 1, &conversion);

    return conversion;
}



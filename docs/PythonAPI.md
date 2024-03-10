# Data Acquisition system for Oregon State Ice Ocean T-Rake

The t-rake temperature data acquisition system is a 16-channel Analog-to-Digital converter (A/D) with 16-bit resolution per channel and a high-throughput capability, allowing for high-speed high-resolution data capture.

In addition, the system includes an optional thermistor carrier board designed to carry four quad-channel thermistor conditioning boards.  When used in conjunction, these two boards provide 16-bit resolution of up to 16 thermistors at millisecond time scales.

The thermistor carrier board can be disconnected from the A/D board, allowing a flexible design for future systems that may need high-resolution high-data-rate data acquisition.

## AD7616 A/D chip Features

The AD7616 A/D chip is controlled using six general-purpose registers plus a bank of 32 sequencer stack registers.  See the AD7616 data sheet for details, but the high point needed for programming covered here.

The important things to know about the A/D chip are that it converts 16 channels of analog input using two 8-channel A/D converters in parallel.  These two A/D converters are called the 'A side' and the 'B side'.  The chip always converts a sample from each side at once, providing the results of both conversions in a single conversion.

Some registers must be configured before using the A/D.  They are covered briefly here, again with more detail available in the data sheet.

### Register 2 - Configuration Register

The configuration register contains 8 bits, which are of interest only to the low-level transport layer.  No programming details are of interest to this document.

### Register 3 - Channel Register

The channel register is used only when single conversions are being performed.  In high-speed continuous mode, channel selection is performed using the sequencer stack registers.

One A side channel and one B side channel are selected.  The two selected channels will be converted simultaneously during the next conversion, and and subsequent conversions until new channels a selected.

```
 7  6  5  4  3  2  1  0
+-----------+----------+
|  B  Side  |  A  Side |
+-----------+----------+
```
The four-bit nybbles work the same for A side and B side:  
`0x00-07` - Select A side or B side channel 0-7 for the next conversion.  
`0x08` - Select V<sub>cc</sub> for the next conversion.  
`0x09` - Select V<sub>aldo</sub> for the next conversion.   
`0x0a` - Not used, reserved.  
`0x0b` - Self test.  For the A side, this returns 0xaaaa, and for the B side, it returns 0x5555.  
`0x0c-0f` - These addresses are not used, reserved.  
  

### Register 4 - Input Range Register A1

### Register 5 - Input Range Register A2

### Register 6 - Input Range Register B1

### Register 7 - Input Range Register B2

These four registers work identically, selecting the input range for four channels each.

For each 2-bit field -- four fields in each of the four registers -- the same pattern applies:
`0b00` - +- 10V
`0b01` - +- 2.5V
`0b10` - +- 5V
`0b11` - +- 10V

For the T-rake thermistor application, the negative side of each channel is fixed at 2.5V, so the 0-5V swing provided by the thermistor conditioning board is interpreted as -2.5V - +2.5V, relative to the negative side.  Thus, for the T-rake, always configure all channels with the '01' pattern, indicating +- 2.5V.  The 8-bit pattern for all registers is thus 0b0101 0101 or 0x55.

The fields are allocated to the four registers like this:
```
Register 4 - Range A1:
  7  6  5  4  3  2  1  0
+-----------+-----------+
|  3A |  2A |  1A |  0A |
+-----------+-----------+

Register 5 - Range A2:
  7  6  5  4  3  2  1  0
+-----------+-----------+
|  7A |  6A |  5A |  4A |
+-----------+-----------+

Register 6 - Range B1:
  7  6  5  4  3  2  1  0
+-----------+-----------+
|  3B |  2B |  1B |  0B |
+-----------+-----------+

Register 7 - Range B2:
  7  6  5  4  3  2  1  0
+-----------+-----------+
|  7B |  6B |  5B |  4B |
+-----------+-----------+
```

### Registers 32-63 (0x20-0x3f) - Sequencer Stack Registers

The sequencer stack is an array of 32 9-bit registers that can be configured with a sequence of channels to be converted.  When using the sequencer, a single conversion command allows the hardware to convert up to 32 A/B pairs of channel conversions.

The format of the 32 sequencer stack registers is the same as the channel register, as described elsewhere.  The sole difference is that multiple channel selections are described, starting with register 32, and sequencing forward from there.  The last channel to be converted contains a 1 in bit 8 (the 0x100 bit).  So, the conversion will contain all registers described in register 32, 33, 34, ..., N, where register N has the 0x100 bit set.

```
   8   7  6  5  4  3  2  1  0
+-----+-----------+----------+
| End |  B  Side  |  A  Side |
+-----+-----------+----------+
```

## Python Programming Interface

The Python API is implemented as methods on a class.  The following sections will detail these instance methods.

*NOTE:* Since these are all instance methods in Python, the first `self` parameter is shown in the signatures below, but is not normally explicitly supplied by a caller.

*IMPORTANT NOTE* This code makes use of a C program that directly controls the GPIO pins, and requires admin privileges.  Always run programs making use of this code with `sudo`.

### `The AD7616 Class`
To use the API, you will need to create an instance of the AD7616 class.  This class is developed with `__enter__` and `__exit__` methods to make it appropriate to use the `with` Python syntax.

Best practice:
```py
with AD7616() as chip:
  chip.method1()
  chip.method2()

# After falling off the end of the indented code, the AD7616 object will be destroyed, and the 'chip' variable will no longer be available.
```


### `WriteRegister(self, address, value) : None`

<b>Parameters:</b>  
`self`: The instance of the AD7616 class object.  Typically supplied by the compiler, not the caller.  
`address`: The address of an internal register on the AD7616 A/D chip, within the range 0x02 - 0x3f (2 - 63) inclusive.  Addresses 0, 1, and 8-31 are not valid, and will cause undefined behavior.  
`value`: The 9-bit value to be written to the addressed register.  
<b>Returns:</b> ***None***

Registers within the A/D chip may be written one at a time by specifying the address and value to write.  See the **AD7616 A/D chip Features** section above for details on what registers exist and what values they take.

### `ReadRegister(self, address) : value`

<b>Parameters:</b>  
`self`: The instance of the AD7616 class object.  Typically supplied by the compiler, not the caller.  
`address`: The address of an internal register on the AD7616 A/D chip, within the range 0x02 - 0x3f (2 - 63) inclusive.  Addresses 0, 1, and 8-31 are not valid, and will cause undefined behavior.  
<b>Returns:</b> `value`: The 9-bit value read from the addressed register.  

Registers within the A/D chip may be read one at a time by specifying the address.  See the **AD7616 A/D chip Features** section above for details on what registers exist and what values they may return.

### `ReadRegisters(self, addresses[]) : values[]`

<b>Parameters:</b>  
`self`: The instance of the AD7616 class object.  Typically supplied by the compiler, not the caller.  
`addresses[]`: An array of address of internal registers on the AD7616 A/D chip, within the range 0x02 - 0x3f (2 - 63) inclusive.  Addresses 0, 1, and 8-31 are not valid, and will cause undefined behavior.  
<b>Returns:</b> `values[]`: An array of 9-bit values read from the addressed registers.  

Registers within the A/D chip may be read as a group by specifying an array with a list of addresses.  See the **AD7616 A/D chip Features** section above for details on what registers exist and what values they may return.

The values of the addressed registers are read and returned in an array of the same length.

### `ConvertPair(self, AChannel, BChannel) : (AValue, BValue)`

<b>Parameters:</b>  
`self`: The instance of the AD7616 class object.  Typically supplied by the compiler, not the caller.  
`AChannel`: The channel number for the A side to convert.  
`BChannel`: The channel number for the B side to convert.  
<b>Returns:</b> A tuple containing the 16-bit raw conversion of the A side followed by the B side.  

See the **Register 3 - Channel Register** section above for details on channel selection codes.

### `DefineSequence(self, AChannels[], BChannels[]) : None`

<b>Parameters:</b>  
`self`: The instance of the AD7616 class object.  Typically supplied by the compiler, not the caller.  
`AChannels[]`: An array of A side channels to be converted in a single hardware conversion operation.  
`BChannels[]`: An array of B side channels to be converted in a single hardware convresion operation.  
<b>Returns:</b> ***None***

Write to the sequence stack registers in the A/D chip.  These 32 9-bit registers contain two 4-bit fields each, plus a high bit indicating the end of the sequence:  
```
   8   7  6  5  4  3  2  1  0
+-----+-----------+----------+
| End |  B  Side  |  A  Side |
+-----+-----------+----------+
```

See the sections on **Register 3 - Channel Register** and **Registers 32-63 (0x20-0x3f) - Sequencer Stack Registers** above for details.

### `ReadConversions(self) : values[]`

<b>Parameters:</b>  
`self`: The instance of the AD7616 class object.  Typically supplied by the compiler, not the caller.  
<b>Returns:</b> `values[]`: The array of converted values [AChannels[0], AChannels[1], ..., AChannels[N], BChannels[0], BChannels[1], ..., BChannels[N]]

Before calling ReadConversions(), a channel sequence must be established using DefineSequence().  This function will then perform a single conversion command to the A/D chip, which will convert all channels in the sequence, and return them.

The A/D chip hardware will select the first pair of registers [AChannels[0], BChannels[0]] provided by DefineSequence() and convert them in a single operation, then sequence to the second pair of registers [AChannels[1], BChannels[1]] and so on through [AChannels[N], BChannels[N]].  The sequence is performed in hardware at high speed, so all pairs in the sequence stack are converted as nearly as possible (within 10s of microseconds of each other) at the same time.

Only after the hardware conversion sequence is complete, the low-level code will read back the results and make them available in the values[] array.

The converted samples are returned in the values[] array with all A side samples returned first, followed by all B side samples.

### `Start(self, period, path, filename) : None`

<b>Parameters:</b>  
`self`: The instance of the AD7616 class object.  Typically supplied by the compiler, not the caller.  
`period`: The time in milliseconds between ADC conversion cycles.  
`path`: A string containing the full path to the folder where data acquisition files will be stored.  
`filename`: A string containing the file name (including extension) of the data acquisition file.  
<b>Returns:</b> ***None***

*NOTE:* Before calling Start(), a channel sequence must be established using DefineSequence().

*NOTE:* The path described in the `path` parameter must exist before calling `Start()`.

Typically, the caller will determine a path, and use the same path for all calls to Start().

The string passed in the `filename` parameter will be used to create the data acquisition file.  It will also appear inside the file for reference by later data analysis.  The file name should be unique within `path`.  It is expected that the file name will be based on a time stamp in some way.

Calling Start() allows:
- A file is created on a specified path, and opened for writing.  
- Periodic conversion starts in the background, and continues until Stop() is called.  The conversions produce the same values returned by ReadConversions().
- A new record is written to the file for each conversion, including an indication of the time the conversion was made.

*Note:* It is acceptable to call `Start()` more than once with no `Stop()`.  Only the first call will have any effect.

### `Stop(self) : None`

<b>Parameters:</b>  
`self`: The instance of the AD7616 class object.  Typically supplied by the compiler, not the caller.  
<b>Returns:</b> ***None***

This function will stop the background data acquisition and writing of the file.

*Note:* It is acceptable to call `Stop()` more than once with no `Start()`.  Only the first call will have any effect.

### Example
The following is a test program the exercises all the functionality of the AD7616 API.  It is intended to provide a starting point for future applications.

```py
import time
from ad7616_api import AD7616

def SetConversionScaleForAllChannels(chip):
  # Write an input range of +-2.5V to all channels.
  chip.WriteRegister(4, 0x55)   # Input range for A-side channels 0-3.
  chip.WriteRegister(5, 0x55)   # Input range for A-side channels 4-7.
  chip.WriteRegister(6, 0x55)   # Input range for B-side channels 0-3.
  chip.WriteRegister(7, 0x55)   # Input range for B-side channels 4-7.

def SelectChannelForConversion(chip, AChannel, BChannel):
  # Write two 4-bit fields selecting the channel for the A side (lower nybble) and B side (upper nybble).
  chip.WriteRegister(3, ((BChannel & 0xf) << 4) | (AChannel & 0xf))

def DisplayAllRegisters(chip):
  # Read all registers other than the sequencer stack registers.
  # The configuration register (0x02) should be zero because we have not written to it yet.
  # The other 5 (channel select 0x03, and input range registers) should be as left during any previous write.
  # Retrieve the register values using both the single-read method and the multi-read method.
  reg2 = chip.ReadRegister(2)
  reg3 = chip.ReadRegister(3)
  reg4 = chip.ReadRegister(4)
  reg5 = chip.ReadRegister(5)
  reg6 = chip.ReadRegister(6)
  reg7 = chip.ReadRegister(7)

  print(f"Register 2: {reg2:04x}, Register 3: {reg3:04x}, register 4: {reg4:04x}, register 5: {reg5:04x}, register 6: {reg6:04x}, register 7: {reg7:04x}")

  registers = chip.ReadRegisters([2, 3, 4, 5, 6, 7])
  for v in registers:
    print(f"{v:04x}", end=" ")
  print()

def ConvertAChannelPair(chip, AChannel, BChannel):
  # Programmatically convert a single channel for A side and one for B side.
  # This is used for calibration or other setup, but not for data acquisition,
  # as it is much slower than DefineConversionSequence() / Start().
  (aconv, bconv) = chip.ConvertPair(AChannel, BChannel)

  # The raw value from the ADC is a signed value between -32768 - 32767.
  # Correct for single-sided interpretation by adding 32768 and dropping any overflow.
  aconvunsigned = (aconv + 0x8000) & 0x0000ffff
  bconvunsigned = (bconv + 0x8000) & 0x0000ffff

  print(f"A channel: {aconv} ({aconvunsigned}U), B channel {bconv} ({bconvunsigned}U)")

def DefineConversionSequence(chip):
  # Normal acquisition mode is started by defining the channels to be read
  # on the A side and the B side.  Typically, just read all 8 channels in order
  # on each side.
  # Note that diagnostic channel 8 (Vcc) is added to the A side, 
  #      and diagnostic channel 9 (Valdo) is added to the B side.
  # Note that the two arrays must be the same size.
  # Note that all values in both arrays are limited to physical channels 0-7 plus
  #      8 (Vcc), 9 (Valdo), and 11 (self-test).  Thus, they fit in a 4-bit field.
  print("Defining conversion sequence")
  Achannels = [0, 1, 2, 3, 4, 5, 6, 7, 8]
  Bchannels = [0, 1, 2, 3, 4, 5, 6, 7, 9]
  chip.DefineSequence(Achannels, Bchannels)

  reg2 = chip.ReadRegister(2)
  print(f"After defining a sequence, configuration register is {reg2:04x}")

def ConvertSequence(chip):
  # Convert all A side and B side channels previously configured through a
  # call to DefineConversionSequence().
  # Note that this mechanism is not used for production data acquisition,
  # since it is slow, but can be useful for setup and calibration.
  conversionvalues = chip.ReadConversions()

  linecounter = 4
  for i, conversion in enumerate(conversionvalues):
    conversion_differential = (conversion - 65536) if (conversion & 0x8000) != 0 else conversion
    conversion_single_ended = (conversion + 0x8000) & 0x0000ffff
    print(f"Channel {i:2}: {conversion:4x} = {conversion_differential:5} -> {conversion_single_ended:5}", end=" ")
    linecounter = linecounter - 1
    if linecounter <= 0:
      print()
      linecounter = 4

  conversion = conversionvalues[8]

  aconv = (conversion >> 16) & 0x0000ffff
  bconv = (conversion & 0x0000ffff)
  print(f"Vcc {aconv},  Vldo {bconv}")

# Exercise all the capabilities described in the functions above.
# Note the 'with' syntax that allows the AD7616 object to be closed
# automatically when it goes out of scope when control falls off the
# indented code.
with AD7616() as chip:
  SetConversionScaleForAllChannels(chip)
  SelectChannelForConversion(chip, AChannel=0, BChannel=0)
  DisplayAllRegisters(chip)

  ConvertAChannelPair(chip, AChannel=0, BChannel=0)

  # Define a conversion sequence.  This will apply from this point on.
  # Note that calls to ConvertAChannelPair() are not valid after this.
  # The same converstions are performed by ConvertSequence() (once)
  # and Start() (many times on a timer basis).
  DefineConversionSequence(chip)

  for _ in range(5):
    ConvertSequence(chip)
    time.sleep(.100000)

  # Start a periodic conversion, specifying the period in ms.  Sub-millisecond
  # values are not allowed, only integer multiples of one millisecond.
  # After Start(), and code can be run, such as examining the file system
  # for a signal to stop, or accepting input from the user.  Sleep() is just for example.
  chip.Start(10, "./", "trake.csv")
  time.sleep(10)
  chip.Stop()

```

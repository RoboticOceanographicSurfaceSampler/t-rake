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

### `Initialize() : ADCHandle`

<b>Paramters:</b> ***None***  
<b>Returns:</b> An opaque object that acts as a handle to the ADC instance.  This handle must be passed in to all subsequent calls.

A call to Initialize() is required before any other call.


### `Open(ADCHandle, bus, device) : None`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
`bus`: The bus to open on, either 0 or 1.  Currently, only bus 1 is supported.  
`device`: Bus 0 supports two devices, 0 and 1.  Bus 1 supports 3 devices, 0, 1, and 2.  Currently, only bus 1 device 0 is supported.  
<b>Returns:</b> ***None***

Before the ADC may be used, it must be opened.

### `Terminate(ADCHandle) : None`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
<b>Returns:</b> ***None***

In order to clean up cleanly, and release all devices and memory that may be owned, the final call to `Terminate()` must be made.  After this call, the ADC is no useable.

### `WriteRegister(ADCHandle, address, value) : None`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
`address`: The address of an internal register on the AD7616 A/D chip, within the range 0x02 - 0x3f (2 - 63) inclusive.  Addresses 0, 1, and 8-31 are not valid, and will cause undefined behavior.  
`value`: The 9-bit value to be written to the addressed register.  
<b>Returns:</b> ***None***

Registers within the A/D chip may be written one at a time by specifying the address and value to write.  See the **AD7616 A/D chip Features** section above for details on what registers exist and what values they take.

### `ReadRegister(ADCHandle, address) : value`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
`address`: The address of an internal register on the AD7616 A/D chip, within the range 0x02 - 0x3f (2 - 63) inclusive.  Addresses 0, 1, and 8-31 are not valid, and will cause undefined behavior.  
<b>Returns:</b> `value`: The 9-bit value read from the addressed register.  

Registers within the A/D chip may be read one at a time by specifying the address.  See the **AD7616 A/D chip Features** section above for details on what registers exist and what values they may return.

### `ReadRegisters(ADCHandle, addresses[]) : values[]`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
`addresses[]`: An array of address of internal registers on the AD7616 A/D chip, within the range 0x02 - 0x3f (2 - 63) inclusive.  Addresses 0, 1, and 8-31 are not valid, and will cause undefined behavior.  
<b>Returns:</b> `values[]`: An array of 9-bit values read from the addressed registers.  

Registers within the A/D chip may be read as a group by specifying an array with a list of addresses.  See the **AD7616 A/D chip Features** section above for details on what registers exist and what values they may return.

The values of the addressed registers are read and returned in an array of the same length.

### `DefineSequence(ADCHandle, AChannels[], BChannels[]) : None`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
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

### `ReadConversions(ADCHandle) : values[]`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
<b>Returns:</b> `values[]`: The array of converted values [AChannels[0], AChannels[1], ..., AChannels[N], BChannels[0], BChannels[1], ..., BChannels[N]]

Before calling ReadConversions(), a channel sequence must be established using DefineSequence().  This function will then perform a single conversion command to the A/D chip, which will convert all channels in the sequence, and return them.

The A/D chip hardware will select the first pair of registers [AChannels[0], BChannels[0]] provided by DefineSequence() and convert them in a single operation, then sequence to the second pair of registers [AChannels[1], BChannels[1]] and so on through [AChannels[N], BChannels[N]].  The sequence is performed in hardware at high speed, so all pairs in the sequence stack are converted as nearly as possible (within 10s of microseconds of each other) at the same time.

Only after the hardware conversion sequence is complete, the low-level code will read back the results and make them available in the values[] array.

The converted samples are returned in the values[] array with all A side samples returned first, followed by all B side samples.

### `Start(ADCHandle, period) : None`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
`period`: The time in milliseconds between ADC conversion cycles  
<b>Returns:</b> ***None***

Before calling Start(), a channel sequence must be established using DefineSequence().

Calling Start() allows:
- A file is created on a configured path, and opened for writing.  The file name is based on the time it is created.
- Periodic conversion starts in the background, and continues until Stop() is called.  The conversions produce the same values returned by ReadConversions().
- A new record is written to the file for each conversion, including an indication of the time the conversion was made.

### `Stop(ADCHandle) : None`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
<b>Returns:</b> ***None***

This function will stop the background data acquisition and writing of the file.

### Examples

```py
import adc

adcHandle = adc.Initialize()
adc.open(adcHandle, 1, 0)

# Write an input range of +-2.5V to all channels.
adc.WriteRegister(adcHandle, 4, 0x55)   # Input range for A-side channels 0-3.
adc.WriteRegister(adcHandle, 5, 0x55)   # Input range for A-side channels 4-7.
adc.WriteRegister(adcHandle, 6, 0x55)   # Input range for B-side channels 0-3.
adc.WriteRegister(adcHandle, 7, 0x55)   # Input range for B-side channels 4-7.

# Write two 4-bit fields selecting the channel for the A side (lower nybble) and B side (upper nybble).
adc.WriteRegister(adcHandle, 3, 0x00)

# Read all registers other than the sequencer stack registers.
# The configuration register (0x02) should be zero because we have not written to it yet.
# The other 5 (channel select 0x03, and input range registers) should be as we wrote them above.
values = adc.ReadRegisters(adcHandle, [0x02, 0x03, 0x04, 0x05, 0x06, 0x07])
print(values)

# Normal acquisition mode is started by defining the channels to be read
# on the A side and the B side.  Typically, just read all 8 channels in order
# on each side.
# Note that diagnostic channel 8 (Vcc) is added to the A side, 
#      and diagnostic channel 9 (Valdo) is added to the B side.
# Note that the two arrays must be the same size.
# Note that all values in both arrays are limited to physical channels 0-7 plus
#      8 (Vcc), 9 (Valdo), and 11 (self-test).  Thus, they fit in a 4-bit field.
adc.DefineSequence(adcHandle, [0, 1, 2, 3, 4, 5, 6, 7, 8], [0, 1, 2, 3, 4, 5, 6, 7, 9])

# Read the nine sequencer stack registers that we used (of the available 32).
# Note that they should all reflect the two arrays written, where each A and B
# array element are merged using the form B[i] << 4 | A[i].  The high 4-bit nybble
# reflects what was written in the B-side array, and the low 4-bit nybble is the A-side.
# Note the last value should have the 0x100 bit set (adding 256 to its decimal value),
#      which indicates it is the final sequencer register used in the stack.
#      The hardware stops converting channels after this register.
values = adc.ReadRegisters(adcHandle, [0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28])
print(values)

# Read all values previously set up by DefineSequence().  The size of the returned array
# is the sum of the sizes of the A-side and B-side arrays used in DefineSequence().  All A-side
# values are returned, followed by all B-side values.
# Note the numeric values of the conversions are adjusted to be single-sided rather than
#      the native bipolar conversions done by the chip.  The smallest analog value presented
#      to the chip will be converted to 0x0000, and the largest analog value will be converted to 0xffff.
values = adc.ReadConversions(adcHandle)
print(values)

adc.terminate(adcHandle)
```

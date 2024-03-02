# Data Acquisition system for Oregon State Ice Ocean T-Rake

The t-rake temperature data acquisition system is a 16-channel Analog-to-Digital converter (A/D) with 16-bit resolution per channel and a high-throughput capability, allowing for high-speed high-resolution data capture.

In addition, the system includes an optional thermistor carrier board designed to carry four quad-channel thermistor conditioning boards.  When used in conjunction, these two boards provide 16-bit resolution of up to 16 thermistor boards at millisecond time scales.

The thermistor carrier board can be disconnected from the A/D board, allowing a flexible design for future systems that may need high-resolution high-data-rate data acquisition.

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

### `ReadRegister(ADCHandle, address) : value`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
`address`:   
<b>Returns:</b> `value`

### `ReadRegisters(ADCHandle, addresses[]) : values[]`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
`addresses[]`:   
<b>Returns:</b> `values[]`

### `DefineSequence(ADCHandle, AChannels[], BChannels[]) : None`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
`AChannels[]`:   
`BChannels[]`:   
<b>Returns:</b> ***None***

Write to the sequence stack registers in the A/D chip.  These 32 8-bit registers contain two 4-bit fields each:  
```
 7  6  5  4  3  2  1  0
+-----------+----------+
|  B  Side  |  A  Side |
+-----------+----------+
```

### `ReadConversions(ADCHandle) : values[]`

<b>Parameters:</b>  
`ADCHandle`: The opaque ADC handle returned from a call to Initialize().  This identifies the ADC instance.  
<b>Returns:</b> `values[]`

Before calling ReadConversions(), a channel sequence must be established using DefineSequence().

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

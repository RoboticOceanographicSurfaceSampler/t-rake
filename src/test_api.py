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

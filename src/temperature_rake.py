import time
from ad7616_api import AD7616

class TemperatureRake:
  def __init__(self, runstate, debug):
    self.runstate = runstate
    self.debug = debug

  def Run(self):
    with AD7616(print_diagnostic=False) as chip:
      self.SetConversionScaleForAllChannels(chip)

      # Define a conversion sequence.  This will apply from this point on.
      # Note that calls to ConvertAChannelPair() are not valid after this.
      self.DefineConversionSequence(chip)

      # Start a periodic conversion, specifying the period in ms.  Sub-millisecond
      # values are not allowed, only integer multiples of one millisecond.
      # After Start(), and code can be run, such as examining the file system
      # for a signal to stop, or accepting input from the user.
      utcDateTime = time.gmtime()
      data_file = "{year:04d}-{month:02d}-{day:02d}_{hour:02d}.{minute:02d}.{second:02d}".format(year=utcDateTime.tm_year, month=utcDateTime.tm_mon, day=utcDateTime.tm_mday, hour=utcDateTime.tm_hour, minute=utcDateTime.tm_min, second=utcDateTime.tm_sec)

      chip.Start(10, "/trake/data", data_file)

      try:
        power_low = 0

        while power_low == 0 and self.runstate.is_running():
          time.sleep(10)
          power_low = chip.ReadPowerLow()

          if power_low != 0:
            self.runstate.voltageLow = True
      except KeyboardInterrupt:
        pass

      chip.Stop()


  def SetConversionScaleForAllChannels(self, chip):
    # Write an input range of +-2.5V to all channels.
    range = AD7616.Range.PLUS_MINUS_2_5V.value << 6 | AD7616.Range.PLUS_MINUS_2_5V.value << 4 | AD7616.Range.PLUS_MINUS_2_5V.value << 2 | AD7616.Range.PLUS_MINUS_2_5V.value
    chip.WriteRegister(AD7616.Register.RANGEA_0_3.value, range)   # Input range for A-side channels 0-3.
    chip.WriteRegister(AD7616.Register.RANGEA_4_7.value, range)   # Input range for A-side channels 4-7.
    chip.WriteRegister(AD7616.Register.RANGEB_0_3.value, range)   # Input range for B-side channels 0-3.
    chip.WriteRegister(AD7616.Register.RANGEB_4_7.value, range)   # Input range for B-side channels 4-7.

  def DefineConversionSequence(self, chip):
    # Normal acquisition mode is started by defining the channels to be read
    # on the A side and the B side.  Typically, just read all 8 channels in order
    # on each side.
    # Note that diagnostic channel 8 (Vcc) is added to the A side, 
    #      and diagnostic channel 9 (Valdo) is added to the B side.
    # Note that the two arrays must be the same size.
    # Note that all values in both arrays are limited to physical channels 0-7 plus
    #      8 (Vcc), 9 (Valdo), and 11 (self-test).  Thus, they fit in a 4-bit field.
    print("Defining conversion sequence")
    # Default mapping, including Vcc and Valdo
    #Achannels = [0, 1, 2, 3, 4, 5, 6, 7, 8]
    #Bchannels = [0, 1, 2, 3, 4, 5, 6, 7, 9]

    # A mapping that accounts for convenience trace routing on the board, resulting in:
    # 1. Channels 0-3 and 4-7 are reversed on the A-side relative to the B-side
    # 2. Board3 and Board4 are reversed
    Achannels = [3, 2, 1, 0, 6, 7, 5, 4]
    Bchannels = [4, 5, 6, 7, 0, 1, 2, 3]

    chip.DefineSequence(Achannels, Bchannels)

    configRegister = chip.ReadRegister(AD7616.Register.CONFIGURATION.value)
    configRegister |= 0x1c
    chip.WriteRegister(AD7616.Register.CONFIGURATION.value, configRegister)


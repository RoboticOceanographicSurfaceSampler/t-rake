import pathlib
from enum import Enum
from ctypes import *

""" This shim allows Python programs to easily access the C driver shared library
    ad7616_driver.so, performing housekeeping tasks like loading the library,
    locating entry points, and marshaling parameters and return values.

    NOTE: This shim assumes that the ad7616_driver.so shared library is in the
          same directory as the shim Python file itself.  See instructions to generate
          ad7616_driver.so in the comments of the ad7616_driver.c file.

    For API details, see the file docs/PythonAPI.md, also available on github at 
    https://github.com/RoboticOceanographicSurfaceSampler/t-rake/blob/main/docs/PythonAPI.md
"""

class SPIDEF(Structure):
    """ The handle returned by driver.spi_initialize(),
        and passed in to all other driver methods.
    """
    _fields_ = [("spi_cs_pin",   c_uint32),
                ("spi_sclk_pin", c_uint32),
                ("spi_mosi_pin", c_uint32),
                ("spi_miso_pin", c_uint32),
                ("spi_errorcode", c_int32),
                ("spi_flags", c_int32)]



class AD7616:
    """ A shim that represents the ad7616_driver.so library, but handles
        all the details of locating and loading the shared library, resolving
        method calls, and marshaling method parameters and return values.
    """
    bus = 1
    device = 0
    driver = None
    handle = None
    print_diagnostic = False
    sequenceLength = 0

    class Register(Enum):
        """ The accessible registers within the AD7616 chip.
        """
        CONFIGURATION = 2
        CHANNELSEL = 3
        RANGEA_0_3 = 4
        RANGEA_4_7 = 5
        RANGEB_0_3 = 6
        RANGEB_4_7 = 7
        
    class Range(Enum):
        """ The allowed values of the 2-bit fields in RANGEAxx/RANGEBxx registers.
            Pack these by shifting four of them by successive 2-bit shifts left.
        """
        PLUS_MINUS_10V = 0
        PLUS_MINUS_2_5V = 1
        PLUS_MINUS_5V = 2

    def __init__(self, bus=1, device=0, print_diagnostic=False):
        """ Constructor for an AD7616 object.
        """
        self.bus = bus
        self.device = device
        self.print_diagnostic = print_diagnostic
        self.sequenceLength = 0

    def __enter__(self):
        """ Context manager.  It is required that the calling program use the 'with' idiom:
            with AD7616 as chip:
              # Use the chip object as needed.

            The __enter__ method functions to connect to the hardware, using scarce resources
            in a way that can be automatically disposed when they are no longer needed.
        """
        libname = pathlib.Path().absolute() / "ad7616_driver.so"
        self.driver = CDLL(libname)

        self.driver.spi_initialize.restype = SPIDEF

        self.handle = self.driver.spi_initialize()
        if (self.print_diagnostic):
            self.handle.spi_flags |= 1
        self.driver.spi_open(self.handle, self.bus, self.device)

        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        """ Context manager.  It is required that the calling program use the 'with' idiom:
            with AD7616 as chip:
              # Use the chip object as needed.
            
            The __exit__ method releases scarce resources that were acquired with the __enter__ method,
            and is invoked automatically by the Python language when the 'with' idiom goes out of scope.
        """
        self.driver.spi_terminate(self.handle)

    def WriteRegister(self, address, value):
        """ Write the specified value to the specified AD7616 register address.
            The address should be specified as a value of the Register Enum, e.g.
            Register.CONFIGURATION.value
        """
        self.driver.spi_writeregister(self.handle, address, value)

    def ReadRegister(self, address):
        """ Read the value of an AD7616 register address and return it.
            The address should be specified as a value of the Register Enum, e.g.
            Register.CONFIGURATION.value
        """
        registerValue = self.driver.spi_readregister(self.handle, address)
        return registerValue

    def ReadRegisters(self, addresses):
        """ Read the values of a list of AD7616 register addresses and return them as a list.
            The addresses should be specified as values of the Register Enum, e.g.
            Register.CONFIGURATION.value
        """
        registerss_array = c_uint32 * len(addresses)
        sequenceaddresses = registerss_array()
        sequencevalues = registerss_array()
        for i in range(len(addresses)):
            sequenceaddresses[i] = addresses[i]
            
        self.driver.spi_readregisters(self.handle, len(addresses), sequenceaddresses, sequencevalues)

        values = []
        for value in sequencevalues:
            values.append(value)

        return values

    def ConvertPair(self, AChannel, BChannel):
        """ The AD7616 chip always converts a pair of channels, one A-side and one B-side.
            Specify the two channles as integers from 0-7.
            The two conversion are returned as a tuple (Aconv, Bconf).
        """
        conversion = self.driver.spi_convertpair(self.handle, AChannel, BChannel)

        aconv = (conversion >> 16) & 0xff
        bconv = conversion & 0xff

        return (aconv, bconv)
    
    def DefineSequence(self, AChannels, BChannels):
        self.sequenceLength = len(AChannels)
        channels_array = c_uint32 * self.sequenceLength
        AchannelArray = channels_array()
        BchannelArray = channels_array()
        for i in range(len(AChannels)):
            AchannelArray[i] = AChannels[i]
            BchannelArray[i] = BChannels[i]

        self.driver.spi_definesequence(self.handle, self.sequenceLength, AchannelArray, BchannelArray)

    def ReadConversions(self):
        conversions_array = c_uint32 * self.sequenceLength
        conversionvalues = conversions_array()
        self.driver.spi_readconversion(self.handle, self.sequenceLength, conversionvalues)
        for conversionvalue in conversionvalues: print(f"{conversionvalue} ", end=" ")
        print()

        conversions = []
        # First, append all the A side conversions.
        for conversion in conversionvalues:
            conversions.append(((conversion >> 16) & 0xffff))
        # Second, append all the B side conversions.
        for conversion in conversionvalues:
            conversions.append(conversion & 0xffff)

        return conversions

    def Start(self, period, path, filename):
        self.driver.spi_start.argtypes = [SPIDEF, c_uint32, c_char_p, c_char_p]
        self.driver.spi_start(self.handle, period, c_char_p(bytes(path, "ASCII")), c_char_p(bytes(filename, "ASCII")))

    def Stop(self):
        self.driver.spi_stop(self.handle)

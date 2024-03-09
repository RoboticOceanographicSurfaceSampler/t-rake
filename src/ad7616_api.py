import pathlib
import time
from ctypes import *

class SPIDEF(Structure):
    _fields_ = [("spi_cs_pin",   c_uint32),
                ("spi_sclk_pin", c_uint32),
                ("spi_mosi_pin", c_uint32),
                ("spi_miso_pin", c_uint32),
                ("spi_errorcode", c_int32)]



class AD7616:
    bus = 1
    device = 0
    driver = None
    handle = None
    sequenceLength = 0

    def __init__(self, bus=1, device=0):
        self.bus = bus
        self.device = device
        self.sequenceLength = 0

    def __enter__(self):
        libname = pathlib.Path().absolute() / "ad7616_driver.so"
        self.driver = CDLL(libname)

        self.driver.spi_initialize.restype = SPIDEF

        self.handle = self.driver.spi_initialize()
        self.driver.spi_open(self.handle, self.bus, self.device)

        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        self.driver.spi_terminate(self.handle)

    def WriteRegister(self, address, value):
        self.driver.spi_writeregister(self.handle, address, value)

    def ReadRegister(self, address):
        registerValue = self.driver.spi_readregister(self.handle, address)
        return registerValue

    def ReadRegisters(self, addresses):
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

    def Start(self, period):
        self.driver.spi_start(self.handle, period)

    def Stop(self):
        self.driver.spi_stop(self.handle)

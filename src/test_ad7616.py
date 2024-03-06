from ctypes import *
import time

class SPIDEF(Structure):
    _fields_ = [("spi_cs_pin",   c_uint32),
                ("spi_sclk_pin", c_uint32),
                ("spi_mosi_pin", c_uint32),
                ("spi_miso_pin", c_uint32),
                ("spi_errorcode", c_int32)]

testlib = CDLL("/home/louis/github/t-rake-c/src/test_ad7616.so")

testlib.spi_initialize.restype = SPIDEF
handle = testlib.spi_initialize()
print(handle.spi_cs_pin, handle.spi_sclk_pin, handle.spi_mosi_pin, handle.spi_miso_pin, handle.spi_errorcode)

testlib.spi_open(handle, 1, 0)

testlib.spi_writeregister(handle, 4, 0x55)
testlib.spi_writeregister(handle, 5, 0x55)
testlib.spi_writeregister(handle, 6, 0x55)
testlib.spi_writeregister(handle, 7, 0x55)
testlib.spi_writeregister(handle, 3, 0x00)

reg3 = testlib.spi_readregister(handle, 3)
reg4 = testlib.spi_readregister(handle, 4)
reg5 = testlib.spi_readregister(handle, 5)
reg6 = testlib.spi_readregister(handle, 6)
reg7 = testlib.spi_readregister(handle, 7)

print("Register 3: " + str(reg3) + ", register 4: " + str(reg4) + ", register 5: " + str(reg5) + ", register 6: " + str(reg6) + ", register 7: " + str(reg7))

addr_array = c_uint32 * 5
addresses = addr_array(3, 4, 5, 6, 7)
values = addr_array(0,0,0,0,0)
testlib.spi_readregisters(handle, 5, addresses, values)
for v in values: print(v, end=" ")
print()

conversion = testlib.spi_convertpair(handle, 0, 0)
print(conversion)
aconv = (conversion >> 16) & 0xff
aconv = (aconv + 0x8000) & 0x0000ffff
bconv = conversion & 0xff
bconv = (bconv + 0x8000) & 0x0000ffff

print("A channel: " + str(aconv) + ", B channel: " + str(bconv))

print("Defining conversion sequence")
channels_array = c_uint32 * 9
Achannels = channels_array(0, 1, 2, 3, 4, 5, 6, 7, 8)
Bchannels = channels_array(0, 1, 2, 3, 4, 5, 6, 7, 9)
testlib.spi_definesequence(handle, 9, Achannels, Bchannels)

reg2 = testlib.spi_readregister(handle, 2)
print("After defining a sequence, configuration register is " + hex(reg2))


sequenceaddresses = channels_array(0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28)
sequencevalues = channels_array()
testlib.spi_readregisters(handle, 9, sequenceaddresses, sequencevalues)
print("Read back of sequence stack")
for seq in sequencevalues:
    Achannel = (seq >> 4) & 0x0f
    Bchannel = seq & 0x0f
    endflag = " End" if (seq & 0x100) != 0 else ""
    print("A: " + str(Achannel) + " B: " + str(Bchannel) + endflag)


conversionvalues = channels_array()
for _ in range(5):
    testlib.spi_readconversion(handle, 9, conversionvalues)

    for i in range(8):
        conversion = conversionvalues[i]

        aconv = (conversion >> 16) & 0x0000ffff
        aconv = (aconv + 0x8000) & 0x0000ffff
        bconv = (conversion & 0x0000ffff)
        bconv = (bconv + 0x8000) & 0x0000ffff
        print(f"Channel {i}A = {aconv} ({hex(aconv)}),  {i}B = {bconv} ({hex(bconv)})")

    conversion = conversionvalues[8]

    aconv = (conversion >> 16) & 0x0000ffff
    bconv = (conversion & 0x0000ffff)
    print(f"Vcc {aconv},  Vldo {bconv}")

    time.sleep(.100000)



testlib.spi_terminate(handle)
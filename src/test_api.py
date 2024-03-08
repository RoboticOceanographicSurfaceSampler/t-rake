import time
from ad7616_api import AD7616

def SetConversionScaleForAllChannels(chip):
    chip.WriteRegister(4, 0x55)
    chip.WriteRegister(5, 0x55)
    chip.WriteRegister(6, 0x55)
    chip.WriteRegister(7, 0x55)

def SelectChannelForConversion(chip, AChannel, BChannel):
    chip.WriteRegister(3, ((BChannel & 0xf) << 4) | (AChannel & 0xf))


def DisplayAllRegisters(chip):
    reg3 = chip.ReadRegister(3)
    reg4 = chip.ReadRegister(4)
    reg5 = chip.ReadRegister(5)
    reg6 = chip.ReadRegister(6)
    reg7 = chip.ReadRegister(7)

    print("Register 3: " + str(reg3) + ", register 4: " + str(reg4) + ", register 5: " + str(reg5) + ", register 6: " + str(reg6) + ", register 7: " + str(reg7))

    registers = chip.ReadRegisters([3, 4, 5, 6, 7])
    for v in registers:
        print(v, end=" ")
    print()

def ConvertAChannelPair(chip, AChannel, BChannel):
    (aconv, bconv) = chip.ConvertPair(AChannel, BChannel)
    aconvunsigned = (aconv + 0x8000) & 0x0000ffff
    bconvunsigned = (bconv + 0x8000) & 0x0000ffff

    print(f"A channel: {aconv} ({aconvunsigned}U), B channel {bconv} ({bconvunsigned}U)")

def DefineConversionSequence(chip):
    print("Defining conversion sequence")
    Achannels = [0, 1, 2, 3, 4, 5, 6, 7, 8]
    Bchannels = [0, 1, 2, 3, 4, 5, 6, 7, 9]
    chip.DefineSequence(Achannels, Bchannels)

    reg2 = chip.ReadRegister(2)
    print("After defining a sequence, configuration register is " + hex(reg2))

def ConvertSequence(chip):
    conversionvalues = chip.ReadConversions()

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


with AD7616() as chip:
    SetConversionScaleForAllChannels(chip)
    SelectChannelForConversion(chip, AChannel=0, BChannel=0)
    DisplayAllRegisters(chip)

    ConvertAChannelPair(chip, AChannel=0, BChannel=0)

    DefineConversionSequence(chip)

    for _ in range(5):
        ConvertSequence(chip)

        time.sleep(.100000)



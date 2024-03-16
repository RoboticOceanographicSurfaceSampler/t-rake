import time
from collections import namedtuple
import RPi.GPIO as GPIO
from smbus import SMBus
from rtc_define import *

RTCDateTime = namedtuple('RTCDateTime', ['second', 'minute', 'hour', 'day', 'month', 'year', 'century'])

GPIO.setmode(GPIO.BCM)
GPIO.setup(Hardware.RTCENBPin.value, GPIO.OUT)


GPIO.output(Hardware.RTCENBPin.value, GPIO.HIGH)
i2cbus = SMBus(1)

def HexadecimalDecode(hex):
  low = hex & 0xf
  high = (hex >> 4) & 0xf
  return (high * 10) + low

def HexadecimalEncode(dec):
  high = int(dec / 10)
  low = dec - (high * 10)
  return (high << 4) | low

def WriteHexadecimalToRTC(address, dec):
  hex = HexadecimalEncode(dec)
  i2cbus.write_byte_data(Hardware.RTCAddress.value, address, hex)

  
def SetUTCTimeIntoRTC():
  gmtime = time.gmtime()
  year = gmtime.tm_year
  month = gmtime.tm_mon
  day = gmtime.tm_mday
  hour24 = gmtime.tm_hour
  minute = gmtime.tm_min
  second = gmtime.tm_sec

  print(f"Setting UTC time into RTC: {month:02}-{day:02}-{year:02} {hour24:2}:{minute:02}:{second:02}")

  RTCcentury = 0
  RTCyear = year - 2000
  if RTCyear > 99:
    RTCyear = RTCyear - 100
    RTCcentury = RTCMonthMasks.RTCCenturyMask.value
  
  WriteHexadecimalToRTC(RTCRegister.RTCYear.value, RTCyear)
  WriteHexadecimalToRTC(RTCRegister.RTCMonth.value, month | RTCcentury)
  WriteHexadecimalToRTC(RTCRegister.RTCDayOfMonth.value, day)
  WriteHexadecimalToRTC(RTCRegister.RTCHours.value, hour24 & ~(RTCHourMasks.RTC12_24Mask.value))
  WriteHexadecimalToRTC(RTCRegister.RTCMinutes.value, minute)
  WriteHexadecimalToRTC(RTCRegister.RTCSeconds.value, second)

def GetUTCTimeFromRTC():
  second = HexadecimalDecode(i2cbus.read_byte_data(Hardware.RTCAddress.value, RTCRegister.RTCSeconds.value))
  minute = HexadecimalDecode(i2cbus.read_byte_data(Hardware.RTCAddress.value, RTCRegister.RTCMinutes.value))
  hour = HexadecimalDecode(i2cbus.read_byte_data(Hardware.RTCAddress.value, RTCRegister.RTCHours.value))
  day = HexadecimalDecode(i2cbus.read_byte_data(Hardware.RTCAddress.value, RTCRegister.RTCDayOfMonth.value))
  RTCmonth = i2cbus.read_byte_data(Hardware.RTCAddress.value, RTCRegister.RTCMonth.value)
  month = HexadecimalDecode(RTCmonth & RTCMonthMasks.RTCMonthMask.value)
  century = RTCmonth & RTCMonthMasks.RTCCenturyMask.value
  year = HexadecimalDecode(i2cbus.read_byte_data(Hardware.RTCAddress.value, RTCRegister.RTCYear.value))

  return RTCDateTime(second, minute, hour, day, month, year, century)

SetUTCTimeIntoRTC()
RTCtime = GetUTCTimeFromRTC()

century = 20 + 1 if RTCtime.century != 0 else 20
print(f"RTC time is now:           {RTCtime.month:02}-{RTCtime.day:02}-{century:02}{RTCtime.year:02} {RTCtime.hour:2}:{RTCtime.minute:02}:{RTCtime.second:02}")

i2cbus.close()
GPIO.output(Hardware.RTCENBPin.value, GPIO.LOW)
GPIO.cleanup()

import os

import RPi.GPIO as GPIO
from smbus import SMBus
from rtc_define import *

GPIO.setmode(GPIO.BCM)
GPIO.setup(Hardware.RTCENBPin.value, GPIO.OUT)


GPIO.output(Hardware.RTCENBPin.value, GPIO.HIGH)
i2cbus = SMBus(1)


def HexadecimalDecode(hex):
  low = hex & 0xf
  high = (hex >> 4) & 0xf
  return (high * 10) + low

def ReadHexadecimalFromRTC(address):
  global i2cbus
  hexvalue = i2cbus.read_byte_data(Hardware.RTCAddress.value, address)
  decvalue = HexadecimalDecode(hexvalue)
  return decvalue


def SetUTCTimeFromRTC():
  year = ReadHexadecimalFromRTC(RTCRegister.RTCYear.value) + 2000
  month = ReadHexadecimalFromRTC(RTCRegister.RTCMonth.value)
  day = ReadHexadecimalFromRTC(RTCRegister.RTCDayOfMonth.value)
  hour24 = ReadHexadecimalFromRTC(RTCRegister.RTCHours.value) & RTCHourMasks.RTC24HourMask.value
  minute = ReadHexadecimalFromRTC(RTCRegister.RTCMinutes.value)
  second = ReadHexadecimalFromRTC(RTCRegister.RTCSeconds.value)

  print(f"Setting system UTC time from RTC: {month:02}-{day:02}-{year:02} {hour24:2}:{minute:02}:{second:02}")

  datetime = f"{year}-{month}-{day} {hour24}:{minute}:{second}"
  os.system(f'sudo date --set "{datetime}"')

SetUTCTimeFromRTC()

i2cbus.close()
GPIO.output(Hardware.RTCENBPin.value, GPIO.LOW)
GPIO.cleanup()


from enum import Enum

class Hardware(Enum):
  RTCENBPin = 26        # Broadcom pin 23 (Pi pin 16), 1 enables the RTC I2C bus.
  RTCAddress = 0x68     # The I2C device address

class RTCRegister(Enum):
  RTCSeconds = 0x00     # Two BCD digits, 00-59
  RTCMinutes = 0x01     # Two BCD digits, 00-59
  RTCHours = 0x02       # Two BCD digits plus RTC12_24Mask, either 00-23 or 01-12
  RTCDayOfWeek = 0x03   # One BCD digit, 1-7
  RTCDayOfMonth = 0x04  # Two BCD digits, 01-31
  RTCMonth = 0x05       # Two BCD digits plus RTCCenturyMask, 01-12
  RTCYear = 0x06        # Two BCD digits, 00-99

class RTCHourMasks(Enum):
  RTC12_24Mask = 0x40   # 12/24 hour field in RTCHour, 0 is 24, 1 is 12
  RTCAM_PMMask = 0x20   # When RTC12_24Mask is 1 (12), this is AM/PM, 0 is AM, 1 is PM
  RTC24HourMask = 0x3f  # When RTC12_24Mask is 0 (24), this is the BCD digit mask for hours.
  RTC12HourMask = 0x1f  # When RTC12_24Mask is 1 (12), this is the BCD digit mask for hours.

class RTCMonthMasks(Enum):
  RTCCenturyMask = 0x80 # Adds an extra century to the years field, rolls over to 1 after century == 99.
  RTCMonthMask = 0x1f   # The Month field.


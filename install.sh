#!/bin/bash

apt install -y python3-smbus
apt install -y python3-watchdog
mkdir /trake
mkdir /trake/configuration
mkdir /trake/data
cp ./start-trake.sh /usr/local/bin
chmod 744 /usr/local/bin/start-trake.sh
(crontab -l ; echo "@reboot /usr/local/bin/start-trake.sh") 2>&1 | grep -v "no crontab" | sort | uniq | crontab -
cd src
python3 ./set_rtc_datetime.py >> /home/trake/trake.log
gcc -Wall -pthread -fpic -shared -o ad7616_driver.so ad7616_driver.c -lpigpio -lrt
cd ..


#!/bin/bash

cd /home/trake/github/t-rake/src
python3 ./set_time_from_rtc.py >> /home/trake/trake.log
echo "Deploying Temperature Rake data acquisition" >> /home/trake/trake.log
python3 deploy.py
echo "Temperature Rake data acquisition complete" >> /home/trake/trake.log
echo "Stopping system" >> /home/trake/trake.log
/usr/sbin/poweroff

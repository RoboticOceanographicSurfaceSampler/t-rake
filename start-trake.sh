#!/bin/bash

cd /home/trake/github/t-rake/src
echo "Deploying Temperature Rake data acquisition" >> /home/trake/trake.log
python3 deploy.py
echo "Temperature Rake data acquisition complete" >> /home/trake/trake.log
echo "Stopping system" >> /home/trake/trake.log
shutdown -h now
#!/bin/bash

echo "Deploying Temperature Rake data acquisition" >> /home/trake/trake.log
python3 /home/trake/github/t-rake/src/deploy.py
echo "Temperature Rake data acquisition complete" >> /home/trake/trake.log
echo "Stopping system" >> /home/trake/trake.log
shutdown -h now
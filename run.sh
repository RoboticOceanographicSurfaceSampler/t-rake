#!/bin/bash

cd /home/trake/github/t-rake/src
echo "Test Deploying Temperature Rake data acquisition" >> /home/trake/trake.log
python3 deploy.py driver
echo "Test Temperature Rake data acquisition complete" >> /home/trake/trake.log

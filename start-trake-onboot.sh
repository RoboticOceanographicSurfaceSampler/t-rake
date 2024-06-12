#!/bin/bash

cd /home/trake/github/t-rake/src
echo "Starting t-rake operation on power up" >> /home/trake/trake.log
python3 deployautostart.py
echo "t-rake power up autostart complete" >> /home/trake/trake.log

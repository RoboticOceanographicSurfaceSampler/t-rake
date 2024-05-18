#!/bin/bash

sudo cp test-trake.sh /usr/local/bin/start-trake.sh
sudo echo "Changed startup script to test-trake.sh" >> /home/trake/trake.log
echo "Reboot to invoke test-mode trake acquisition"

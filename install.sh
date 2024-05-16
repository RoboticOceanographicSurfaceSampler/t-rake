#!/bin/bash

apt install -y python3-watchdog
mkdir /trake
mkdir /trake/configuration
mkdir /trake/data
cp ./start-trake.sh /usr/local/bin
chmod 744 /usr/local/bin/start-trake.sh
(crontab -l ; echo "@reboot /usr/local/bin/start-trake.sh") 2>&1 | grep -v "no crontab" | sort | uniq | crontab -

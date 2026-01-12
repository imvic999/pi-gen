#!/bin/sh

sudo /home/pi/freqset6_915 -chn=3
gpioset gpiochip0 24=1
gpioset gpiochip0 25=1
/usr/bin/python3 /home/pi/udp_freqReWrite.py 1701 192.168.70.73 1700 

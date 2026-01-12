#!/bin/bash

ip link show uap0 > /dev/null 2>&1
if [ $? -eq 0 ]; then
    iw dev uap0 del
fi

iw dev wlan0 interface add uap0 type __ap
ip link set uap0 up
ip addr add 10.0.0.1/24 dev uap0

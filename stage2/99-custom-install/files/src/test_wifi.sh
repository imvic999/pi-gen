#!/bin/bash

if [ "$#" != "3" ]; then
    echo  "failed"
    echo "usage: test_wifi.sh 2.4g/5g ssid password"
    exit 1
fi

wifi_type=$1
if [[ "$wifi_type" != "2.4g" && "$wifi_type" != "5g" ]]; then
    echo  "failed"
    echo "error wifi_type="$wifi_type
    exit 1
fi

wlan0=`nmcli connection show | grep wlan0`
if [ "$wlan0" != "" ]; then
    disconnect_cmd="sudo nmcli device disconnect wlan0"
    result=`$disconnect_cmd`
    if [ "$?" != "0" ]; then
        echo "failed"
	echo $reulst
    fi
fi

ssid="$2"
passwd="$3"
connect_cmd="sudo nmcli dev wifi connect $ssid password $passwd"
result=`$connect_cmd`
if [ "$?" != "0" ]; then
    echo "failed"
    echo "$result"
fi

echo "===== OK"
result=`ip a | grep wlan0`
ip=`echo $result | awk '{print $15}'` 
echo $ip
exit 0

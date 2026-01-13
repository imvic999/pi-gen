#!/bin/bash -e

sudo apt update && sudo apt full-upgrade -y
#sudo apt install -y rpi-sb-provisioner
sudo apt install -y /home/pi/rpi-sb-provisioner_2.1.1_arm64.deb
sed -i 's|^ExecStart=/usr/bin/rpi-provisioner-ui|& -a 0.0.0.0|' /lib/systemd/system/rpi-provisioner-ui.service

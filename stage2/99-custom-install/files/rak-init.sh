#!/bin/bash
exec > /var/log/rak-init.log 2>&1
echo "Starting RAK first-boot configuration..."

cd /home/pi/rak_common_for_gateway/sysconf
./install.sh

systemctl disable rak-first-boot.service
#rm -f /etc/systemd/system/rak-first-boot.service
#echo "Configuration complete. Rebooting..."
#sleep 2
#reboot
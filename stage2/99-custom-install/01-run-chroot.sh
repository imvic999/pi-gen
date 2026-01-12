#!/bin/bash -e

raspi-config nonint do_i2c 0
raspi-config nonint do_spi 0
raspi-config nonint do_serial_hw 0
raspi-config nonint do_serial_cons 1

cd /home/pi

rm -rf rak_common_for_gateway
git clone https://github.com/RAKWireless/rak_common_for_gateway.git
cd rak_common_for_gateway

#Disable sysconf configuration here and run it only in first boot
sed -i '/pushd sysconf/,/popd/ s/^/#/' install.sh
#Disable chirpstack installation, device not Pi 3 or 4 will force install.
sed -i '/if \[ \$rpi_model -ne 3 \] && \[ \$rpi_model -ne 4 \];/,/fi/ s/^/#/' install.sh

mkdir -p /usr/local/rak
cat <<JSON > rak/rak/rak_gw_model.json
{
	"gw_model": "RAK5146",
	"spi": "1"
}
JSON

cat <<JSON > rak/rak/gateway-config-info.json
{
	"install_lte": "0",
	"install_chirpstack": "0"
}
JSON

#instal RAK gateway software without chirpstack
#./install.sh --img --chirpstack=not_install
echo 11 | ./install.sh --chirpstack=not_install


#Enable first boot service for execute sysconf configuration
systemctl enable rak-first-boot.service
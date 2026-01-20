#!/bin/bash -e

install -m 755 files/src/AS915/TXRX\ test\ tool/freqset6_915 "${ROOTFS_DIR}/home/pi/"
install -m 755 files/src/AS915/TXRX\ test\ tool/freqset8_915 "${ROOTFS_DIR}/home/pi/"
install -m 644 files/src/AS915/global_conf.json.915	"${ROOTFS_DIR}/opt/ttn-gateway/packet_forwarder/lora_pkt_fwd/global_conf.json"
#install -m 644 files/udp_freqReWrite.py	"${ROOTFS_DIR}/home/pi/"
install -m 644 files/src/AS915/udp_freqReWrite_915.py	"${ROOTFS_DIR}/home/pi/udp_freqReWrite.py"
install -m 755 files/udp_freqReWrite.sh	"${ROOTFS_DIR}/home/pi/"
install -m 755 files/udp_freqReWrite_dqa.sh	"${ROOTFS_DIR}/home/pi/"

#install -m 644 files/global_conf.json "${ROOTFS_DIR}/opt/ttn-gateway/packet_forwarder/lora_pkt_fwd/"
install -m 644 files/udpRewrite.service	"${ROOTFS_DIR}/lib/systemd/system/"

mkdir -p "${ROOTFS_DIR}/home/pi/src/"
install -m 755 files/src/power_detect "${ROOTFS_DIR}/home/pi/src/"
install -m 644 files/src/battery-monitor.service "${ROOTFS_DIR}/lib/systemd/system/"

install -m 755 files/src/uart_test "${ROOTFS_DIR}/home/pi/src/"
install -m 755 files/src/vicTX "${ROOTFS_DIR}/home/pi/src/"
install -m 755 files/src/vicRX "${ROOTFS_DIR}/home/pi/src/"

on_chroot << EOF
systemctl enable udpRewrite.service
systemctl enable battery-monitor.service
systemctl enable uap0-setup.service
EOF
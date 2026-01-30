#!/bin/bash -e

install -m 755 files/rak-init.sh	"${ROOTFS_DIR}/usr/local/bin/"
install -m 644 files/rak-first-boot.service	"${ROOTFS_DIR}/etc/systemd/system/"
install -m 600 files/eth0.nmconnection	"${ROOTFS_DIR}/etc/NetworkManager/system-connections/"
install -m 600 files/manufacture.nmconnection	"${ROOTFS_DIR}/etc/NetworkManager/system-connections/"
install -m 600 files/99-unmanaged-ap.conf	"${ROOTFS_DIR}/etc/NetworkManager/conf.d/"
install -m 755 files/create_uap0.sh	"${ROOTFS_DIR}/usr/local/bin/"
install -m 644 files/uap0-setup.service	"${ROOTFS_DIR}/etc/systemd/system/"
install -m 644 files/hostapd.conf "${ROOTFS_DIR}/etc/hostapd/"
install -m 644 files/dnsmasq.conf "${ROOTFS_DIR}/etc/"
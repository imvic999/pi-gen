#!/bin/bash -e

install -m 755 files/rak-init.sh	"${ROOTFS_DIR}/usr/local/bin/"
install -m 644 files/rak-first-boot.service	"${ROOTFS_DIR}/etc/systemd/system/"
install -m 600 files/eth0.nmconnection	"${ROOTFS_DIR}/etc/NetworkManager/system-connections/"
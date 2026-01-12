#!/bin/bash -e

install -m 600 files/eth0.nmconnection	"${ROOTFS_DIR}/etc/NetworkManager/system-connections/"
install -m 644 files/config	"${ROOTFS_DIR}/etc/rpi-sb-provisioner/"
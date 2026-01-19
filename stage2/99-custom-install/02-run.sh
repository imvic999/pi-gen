#!/bin/bash -e

install -m 600 files/eth0.nmconnection	"${ROOTFS_DIR}/etc/NetworkManager/system-connections/"
install -m 644 files/config	"${ROOTFS_DIR}/etc/rpi-sb-provisioner/"

mkdir -p "${ROOTFS_DIR}/etc/rpi-sb-provisioner/scripts"
install -m 755 files/sb-provisioner-rootfs-mounted.sh	"${ROOTFS_DIR}/etc/rpi-sb-provisioner/scripts"
#!/bin/sh

# This script runs when rootfs-mounted for sb-provisioner
# Arguments:
# $1 - Path to mounted boot image
# $2 - Path to mounted rootfs image

BOOT_MOUNT="$1"
ROOTFS_MOUNT="$2"

echo "Running rootfs-mounted customisation"
echo "Boot mount: ${BOOT_MOUNT}"
echo "Rootfs mount: ${ROOTFS_MOUNT}"

# Example: Modify rootfs files
echo "Extracting and Copying test app to hosts file"
tar -xvf /home/pi/production_tools.tar -C ${ROOTFS_MOUNT}/home/pi
chmod 755 -R ${ROOTFS_MOUNT}/home/pi
chown -R pi:pi ${ROOTFS_MOUNT}/home/pi

echo "Copying WiFi AP profile to hosts file"
cp /home/pi/manufacture.nmconnection ${ROOTFS_MOUNT}/etc/NetworkManager/system-connections
chmod 600 ${ROOTFS_MOUNT}/etc/NetworkManager/system-connections/manufacture.nmconnection
chown root:root ${ROOTFS_MOUNT}/etc/NetworkManager/system-connections/manufacture.nmconnection

# Exit with success
exit 0

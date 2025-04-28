#!/bin/bash

source /etc/init_functions

EnableTracing

# Do not load modules if this is a test image
TESTIMAGE=0
[ $TESTIMAGE -eq 0 ] || exit 0

# Remove stale map files
rm -f /var/support/map_*

if cat /proc/cmdline | grep -wq single_pre_sysinit ; then
    echo
    echo "Single user mode (after switch_root, before 2nd hwdetect)"
    echo "Exit shell to continue booting"
    # leverage the efforts of systemd to goto console
    #/bin/sh
    # exit 0
fi

# Backup the existing settings.sys to aide debugging issues
cp /etc/settings.sys /etc/settings.sys.bak

# Depending on disk change or reordering, /dev/sdx mapping could change during reboots
ACTUAL_ROOT_HDD="/dev/$(lsblk -no pkname $(findmnt -n -o SOURCE / ))"
source hex_tuning /etc/settings.sys sys.install.hdd
[ "x$T_sys_install_hdd" = "x$ACTUAL_ROOT_HDD" ] || sed -i "s;^sys.install.hdd.*;sys.install.hdd=$ACTUAL_ROOT_HDD;" /etc/settings.sys

# HP Smart Array SCSI driver
modprobe hpsa

# megaraid driver
modprobe megaraid_sas

# adaptec raid driver
modprobe aacraid

# smartpqi raid driver
modprobe smartpqi

# nvme support
modprobe nvme

# sata support
modprobe ahci

# efi boot partition (vfat 32)
modprobe vfat

# required by mount iso
modprobe isofs

# required by mount usb
modprobe nls_iso8859-1

# required by usb storage devices
if [ ! -e /etc/appliance/state/configured ] ; then
    usb_conf=/etc/modprobe.d/usb.conf
    usb_temp=/tmp/usb.conf
    rm -f $usb_temp
    [ ! -e $usb_conf ] || mv -f $usb_conf $usb_temp
    modprobe usbhid
    modprobe usb_storage
    modprobe uas
    [ ! -e $usb_temp ] || mv -f $usb_temp $usb_conf
else
    modprobe usbhid
    modprobe usb_storage
    modprobe uas
fi

# required by LCAP support
modprobe bonding mode=4 miimon=100 lacp_rate=1

# required by VLAN support
modprobe 8021q

# Perform hardware detection and/or load hardware-specific drivers
Source /etc/rc.hwdetect

# Update NIC settings
Source /etc/rc.nicdetect

# Mount extra partitions if necessary
# e.g. /boot, enable swap when booting from hdd
#   or mount boot media when booting from installer or live mode
Source /etc/rc.mount

# Perform additional post-install setup on first boot after install or update
SourceAndLog /etc/rc.postinstall /var/log/postinstall.log
rm -f /etc/rc.postinstall

# Perform project-specific boot time initialization
SourceAndLog /etc/rc.projinit /var/log/projinit.log

# Place core files in /var/support (if enabled)
echo "/var/support/core_%e.%p" > /proc/sys/kernel/core_pattern

# Update grub default boot partition
for item in $(cat /proc/cmdline); do
    case $item in
        HexSaveDefault*) /usr/sbin/grub-set-default $(echo $item | cut -d "=" -f 2) ;;
    esac
done

# Update last boot time if root dir is mounted
if [ -n "$(/usr/sbin/grub-get-default 2>/dev/null)" ]; then
    GRUB_PATH=/boot/grub2
    sed -i '/^last_boot/d' $GRUB_PATH/info$(/usr/sbin/grub-get-default)
    # Time since Epoch (must be reformatted by CLI/LMI)
    echo "last_boot = "$(/bin/date '+%s') >> $GRUB_PATH/info$(/usr/sbin/grub-get-default)
fi

for T in $(dmesg | grep "console .* enabled" | grep -o "tty.*]" | cut -d "]" -f1); do
    timeout 3 bash -c "systemctl restart serial-getty@$T" || true
done

#!/bin/sh

source hex_tuning /etc/settings.sys sys.install.hdd
if [ -n "$T_sys_install_hdd" ]; then
    echo "Wiping master boot record..."
    dd if=/dev/zero of=$T_sys_install_hdd bs=512 count=1 >/dev/null 2>&1
    echo "Rebooting..."
    reboot
else
    echo "Error: sys.install.hdd not set"
    exit 1
fi

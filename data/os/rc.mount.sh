#!/bin/bash

# Mount boot media for live mode
(
    mnt_dir=/mnt/install
    source hex_tuning /etc/settings.sys sys.live.
    mode=$T_sys_live_mode
    case "$mode" in
        usb)
            for usbdev in $(hex_sdk WaitForUsb)
            do
                mount -t vfat -o ro $usbdev $mnt_dir 2>/dev/null || true
                if ls $mnt_dir/*.pkg >/dev/null 2>&1; then
                    break
                else
                    umount $mnt_dir 2>/dev/null || true
                fi
            done
            ;;
        iso)
            hex_sdk WaitForCdrom
            for srdev in $(hex_sdk WaitForCdrom)
            do
                mount -t iso9660 -o ro $srdev $mnt_dir 2>/dev/null || true
                if ls $mnt_dir/*.pkg >/dev/null 2>&1; then
                    break
                else
                    umount $mnt_dir 2>/dev/null || true
                fi
            done
            ;;
    esac

    # If neither usb nor iso succeeded to detect firmware, probe all disks as a last resort
    if ! (ls $mnt_dir/*.pkg >/dev/null 2>&1); then
        for diskdev in $(lsblk -dn -o NAME,TYPE | grep disk | tr -s ' ' | cut -d' ' -f1)
        do
            mount -o loop,offset=32256 /dev/$diskdev $mnt_dir 2>/dev/null || true
            if ls $mnt_dir/*.pkg >/dev/null 2>&1; then
                break
            else
                umount $mnt_dir 2>/dev/null || true
            fi
        done
    fi
)

#!/bin/sh

GRUBCFG="/boot/grub2/grub.cfg"

if [ $# -lt 1 -o $# -gt 2 ]; then
    echo "Usage: $(basename $0) <1 or 2> [--once]" >&2
    exit 1
fi

if grep "/boot.*" /etc/fstab | grep -q "^UUID="; then
    BOOTUUID=$(cat /etc/fstab | grep "/boot.*" | tr -s " " | cut -d" " -f1 | cut -d"=" -f2)
    BOOTDEV=$(blkid --uuid $BOOTUUID)
else
    BOOTDEV=$(cat /etc/fstab | grep "/boot.*" | awk -F'/' '{print $3}' | awk '{$1=$1;print}')
fi

if [ "$(cat /proc/mounts | grep $BOOTDEV | awk '{print $2}')" != "/boot" ] ; then
    source /etc/init_functions
    WaitForDev /dev/$BOOTDEV
    mount /dev/$BOOTDEV /boot
    UMOUNTBOOT="/boot"
fi

# removing only tailing digits instead of all digits. ex nvme0n1p1 -> nvme0n1p
DEV=$(echo $BOOTDEV | sed 's/[0-9]\+$//')
case $(cat /proc/cmdline | grep -o 'HexSaveDefault=.' | cut -d "=" -f 2) in
    1) OTHERPART="${DEV}6" ;;
    2) OTHERPART="${DEV}5" ;;
    *) echo "$0: Could not determine current root partition" ;;
esac

otherpart=$(mktemp -d /tmp/mount.XXXXXX)
mount -o ro $OTHERPART $otherpart

grub2-editenv /boot/grub2/grubenv set next_entry=$(expr $1 - 1)

umount $otherpart 2>/dev/null || true

[ -z "$UMOUNTBOOT" ] || umount $UMOUNTBOOT

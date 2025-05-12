#!/bin/sh
# HEX SDK

. /etc/init_functions

MountProcSysDev
EnableTracing

if cat /proc/cmdline | grep -wq single_user_init ; then
    echo
    echo "Single user mode (before running hwdetect)"
    echo "Exit shell to continue booting"
    setsid cttyhack /bin/sh
fi

# Detect hardware and load device specific kernel modules
TRACE_FLAG=
[ $TRACE_INIT -eq 0 ] || TRACE_FLAG=-x

depmod
modprobe ext4
/usr/sbin/hex_hwdetect -i $TRACE_FLAG

# Parse kernel command line
INIT="/sbin/init"
BOOTDEV="/dev/sda1"
ROOTDEV="/dev/sda5"
# Backup destination partition
DESTDEV=
# Grub default boot partition
DEFAULT=
for item in $(cat /proc/cmdline); do
    case $item in
        HexBootDevice*)
            if [[ $item =~ UUID ]]; then
                BOOTUUID=$(echo $item | cut -d "=" -f 3)
                WaitForUuid $BOOTUUID
                BOOTDEV=$(blkid | grep $BOOTUUID | cut -d":" -f1)
            else
                BOOTDEV=$(echo $item | cut -d "=" -f 2)
            fi
            ;;
        root*)
            if [[ $item =~ UUID ]]; then
                ROOTUUID=$(echo $item | cut -d "=" -f 3)
                WaitForUuid $ROOTUUID
                ROOTDEV=$(blkid | grep $ROOTUUID | cut -d":" -f1)
            else
                ROOTDEV=$(echo $item | cut -d "=" -f 2)
            fi
            ;;
        init*)
            INIT=$(echo $item | cut -d "=" -f 2)
            ;;
        HexBackupTo*)
            if [[ $item =~ UUID ]]; then
                DESTUUID=$(echo $item | cut -d "=" -f 3)
                WaitForUuid $DESTUUID
                DESTDEV=$(blkid | grep $DESTUUID | cut -d":" -f1)
            else
                DESTDEV=$(echo $item | cut -d "=" -f 2)
            fi
            ;;
        HexSaveDefault*)
            DEFAULT=$(echo $item | cut -d "=" -f 2)
            ;;
    esac
done

if cat /proc/cmdline | grep -wq single_mount_root ; then
    echo
    echo "Single user mode (before mounting rootfs)"
    echo "Exit shell to continue booting"
    setsid cttyhack /bin/sh
fi

WaitForDev $BOOTDEV
WaitForDev $ROOTDEV

# Mount root partition
mount -o noatime $ROOTDEV /mnt/hdd

DoBackup()
{
    SRC=$1
    DST=$2
    SRCPART=$(WhichPart $ROOTDEV)
    DESTPART=$(WhichPart $DESTDEV)

    # fetch old root partition UUID
    eval $(blkid | grep $DESTDEV | cut -d" " -f2)

    echo -n "Formatting partition $DESTPART ... "
    /usr/sbin/mkfs.ext4 -O ^has_journal -E lazy_itable_init=1 -q -F $DESTDEV >/dev/null 2>&1 || {
        echo "Failed"
        return 1
    }
    echo "Done"

    echo -n "Backing up partition $SRCPART to partition $DESTPART... "
    mount $DESTDEV $DST || return 2
    # exclude pattern is relative
    rsync -aAX --delete --exclude=/dev/* --exclude=/proc/* --exclude=/sys/* --exclude=/tmp/* --exclude=/run/* --exclude=/media/* --exclude="lost+found" $SRC/ $DST/ >/dev/null 2>&1 || {
        echo "Failed"
        return 3
    }
    echo "Done"

    # Fixup fstab so it mounts the correct partition
    DESTUUID=$(blkid | grep $DESTDEV | cut -d" " -f2 | cut -d"=" -f2 | sed 's/"//g')
    sed -i "/ \/ .*ext4/d" $DST/etc/fstab || return 4
    echo "UUID=${DESTUUID}     /       ext4     noatime    0 0" >> $DST/etc/fstab
    sync
    umount $DST || return 5

    # Modify grub info & comment files
    mount $BOOTDEV $SRC/boot || return 6

    GRUBCFG="$SRC/boot/grub2/grub.cfg"
    titles=$(sed -n -e '/menuentry/p' $GRUBCFG | grep -v single | head -4 | grep -o "'.*'" | sed -e "s/'//g")

    sed -i "s/$UUID/$DESTUUID/g" $GRUBCFG

    TITLE="NEWTITLE"
    OLDTITLE="OLDTITLE"
    case $SRCPART in
        1) TITLE=$(echo $titles | awk '{ print $1 }')
           OLDTITLE=$(echo $titles | awk '{ print $2 }')
           ;;
        2) TITLE=$(echo $titles | awk '{ print $2 }')
           OLDTITLE=$(echo $titles | awk '{ print $1 }')
           ;;
    esac

    # Copy info and comment files
    cp -p $SRC/boot/grub2/comment${SRCPART} $SRC/boot/grub2/comment${DESTPART}
    cp -p $SRC/boot/grub2/info${SRCPART} $SRC/boot/grub2/info${DESTPART}

    # Update install type and add/update backup date
    sed -i '/^install_type/d' $SRC/boot/grub2/info${DESTPART}
    sed -i '/^backup_date/d' $SRC/boot/grub2/info${DESTPART}
    echo "install_type = Backup" >> $SRC/boot/grub2/info${DESTPART}
    # Time since Epoch (must be reformatted by CLI)
    echo "backup_date = "$(/bin/date '+%s') >> $SRC/boot/grub2/info${DESTPART}

    # Modify grub title for backup
    sed -i -e "s:${OLDTITLE}:${TITLE}:" $GRUBCFG

    # Backup kernel and ramdisk
    rm -rf $SRC/boot/Part$DESTPART
    cp -rfp $SRC/boot/Part$SRCPART $SRC/boot/Part$DESTPART
    sync

    # On Virtual appliacne, initramfs.cgz is not properly copied the first time
    rm -rf $SRC/boot/Part$DESTPART
    cp -rfp $SRC/boot/Part$SRCPART $SRC/boot/Part$DESTPART
    sync
    umount $SRC/boot || return 8

    return 0
}

# Perform backup if requested
if [ -n "$DESTDEV" ] ; then
    DoBackup /mnt/hdd /mnt/install
    if [ $? -eq 0 ] ; then
        echo -e "Backup complete\n"
    else
        echo -e "Backup failed (error: $?)\n"
    fi
fi

FIXED_NIC_RULE=/mnt/hdd/etc/udev/rules.d/70-persistent-net.rules
rm -f $FIXED_NIC_RULE
IDX=0
for PCIID in $(for D in $(ls /sys/class/pci_bus/*\:*/device/*\:*/class); do echo "$D:$(cat $D)"; done | grep 0x020000 | awk -F'/' '{print $7}' | sort)
do
    echo "# PCI Net Device $PCIID" >> $FIXED_NIC_RULE
    echo "SUBSYSTEM==\"net\", ACTION==\"add\", KERNELS==\"$PCIID\", NAME=\"ifn$IDX\"" >> $FIXED_NIC_RULE
    IDX=$(( $IDX + 1 ))
done

if cat /proc/cmdline | grep -wq single_switch_root ; then
    echo
    echo "Single user mode (before switch_root)"
    echo "Exit shell to continue booting"
    setsid cttyhack /bin/sh
fi

exec switch_root /mnt/hdd $INIT

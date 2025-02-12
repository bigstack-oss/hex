#!/bin/sh
# HEX SDK

PROG=$(basename $0)

KDUMP_ENABLE=0

Usage() {
    echo "Usage: $PROG enable/disable"
    exit 1
}

if [ $# -eq 1  ]; then
    if [ "$1" == "enable" ]; then
        KDUMP_ENABLE=1
    fi
else
    Usage
fi

Log()
{
    logger -t $PROG "$*"
}

if [ -L /boot/grub2/grub.cfg ]; then
    GRUBCFG="/boot/grub/menu.lst"
else
    GRUBCFG="/boot/grub2/grub.cfg"
fi

ACTIVE_PARTITION=$(/usr/sbin/grub-get-default 2>/dev/null)
if [ $? -ne 0 ]; then
    Log "Error: Failed to get active parition"
    exit 1
fi

# Returns 0 or non-zero
# Do not compare to 1!
CRASHKERNEL_PRESENT=$(cat $GRUBCFG | grep "kernel /Part$ACTIVE_PARTITION" | grep -wc crashkernel)

if [ $KDUMP_ENABLE -eq 1 ]; then
    Log "kdump enabled"

    if [ $CRASHKERNEL_PRESENT -eq 0 ]; then
        Log "Adding crashkernel to kernel args (reboot required)"
        /bin/sed -i -e "/kernel \/Part$ACTIVE_PARTITION/s/$/ crashkernel=auto/" $GRUBCFG
        if [ $? -ne 0 ]; then
            Log "Error: Failed to add crashkernel setting to kernel parameter"
        fi
    else
        KDUMP_READY=`cat /sys/kernel/kexec_crash_loaded`
        if [ $KDUMP_READY -ne 1 ]; then
            Log "Loading dump-capture kernel"
            /sbin/kexec -p --command-line="$(cat /proc/cmdline) irqpoll nr_cpus=1 reset_devices cgroup_disable=memory" --initrd=/boot/Part$ACTIVE_PARTITION/initramfs.cgz /boot/Part$ACTIVE_PARTITION/bzImage >/dev/null 2>&1
        fi
        KDUMP_READY=`cat /sys/kernel/kexec_crash_loaded`
        if [ $KDUMP_READY -ne 1 ]; then
            Log "Error: Failed to load dump-capture kernel"
            exit 1
        fi
    fi
else
    Log "kdump disabled"

    if [ $CRASHKERNEL_PRESENT -gt 0 ]; then
        Log "Removing crashkernel from kernel args (reboot required)"
        sed -i -e "/kernel \/Part$ACTIVE_PARTITION/s/ crashkernel=[^ ]*//" $GRUBCFG
    fi

    KDUMP_READY=`cat /sys/kernel/kexec_crash_loaded`
    if [ $KDUMP_READY -eq 1 ]; then
        Log "Unloading dump-capture kernel"
        /sbin/kexec -p -u
        KDUMP_READY=`cat /sys/kernel/kexec_crash_loaded`
        if [ $KDUMP_READY -ne 0 ]; then
            Log "Error: Failed to unload dump-capture kernel"
            exit 1
        fi
    fi
fi

exit 0

#!/bin/sh

TRACE_INIT=0
TRACE_MODPROBE=0
echo $PATH | grep -q 'sbin:' || export PATH="/sbin:${PATH}"

# Get whitespace separated list of console args on the kernel cmdline
# Returns: tty0 ttyS0 ...
GetKernelConsoles()
{
    (
        local consoles=
        set dummy $(cat /proc/cmdline)
        shift 1
        while [ $# -gt 0 ]; do
            case "$1" in
                console=*)
                    chardev=$(echo $1 | sed -e 's/console=//' | cut -d',' -f1)
                    if timeout 1 bash -c "echo -n ' ' > /dev/$chardev" ; then
                        consoles="$consoles $chardev"
                    fi
                    ;;
            esac
            shift 1
        done
        echo $consoles
    )
}

# Get whitespace separated list of console devices on the kernel cmdline
# Returns: /dev/tty0 /dev/ttyS0 ...
GetKernelConsoleDevices()
{
    local devices=
    for C in $(GetKernelConsoles) ; do
        devices="$devices /dev/$C"
    done
    echo $devices
}

# Mount the proc filesystem so we can examine the kernel args
MountProcSysDev()
{
    [ -d /dev ] || mkdir -m 0755 /dev
    [ -d /sys ] || mkdir /sys
    [ -d /proc ] || mkdir /proc
    [ -d /run ] || mkdir /run

    mkdir -p /var/lock
    mount -t sysfs -o nodev,noexec,nosuid sysfs /sys
    mount -t proc -o nodev,noexec,nosuid proc /proc

    # Note that this only becomes /dev on the real filesystem if udev's scripts
    # are used; which they will be, but it's worth pointing out
    if ! mount -t devtmpfs -o mode=0755 udev /dev; then
        echo "W: devtmpfs not available, falling back to tmpfs for /dev"
        mount -t tmpfs -o mode=0755 udev /dev
    fi

    mkdir /dev/pts
    mount -t devpts -o noexec,nosuid,gid=5,mode=0620 devpts /dev/pts || true
    mount -t tmpfs -o "noexec,nosuid,size=10%,mode=0755" tmpfs /run

    # Some things don't work properly without /etc/mtab.
    ln -sf /proc/mounts /etc/mtab
}

# /init must move before switch_root
MoveProcSysDev()
{
    rootdev=$1

    mount -n -o move /run $rootdev/run
    mount -n -o move /dev $rootdev/dev
    mount -n -o move /proc $rootdev/proc
    mount -n -o move /sys $rootdev/sys
}

# Enable tracing if kernel arg "quiet" is not specified
# Must be called after MountProcSysDev
EnableTracing()
{
    # Suppress all kernel messages except KERN_WARNING and above
    echo "4" > /proc/sys/kernel/printk

    # Remove "quiet" kernel arg to show kernel and hex boot process (set -v)
    if cat /proc/cmdline | grep -wq quiet ; then
        :
    else
        TRACE_INIT=1
    fi

    # Add "trace_init" kernel arg to trace init and rc.sysinit
    if cat /proc/cmdline | grep -wq trace_init ; then
        TRACE_INIT=1
    fi

    # Add "trace_modprobe" kernel arg to trace modprobe invocations to help debug hung tasks
    if cat /proc/cmdline | grep -wq trace_modprobe ; then
        TRACE_MODPROBE=1
    fi

    if [ $TRACE_INIT -eq 1 ]; then
        echo "*** Executing $0 ***"
        set -x
    fi

    if [ $TRACE_MODPROBE -eq 1 ]; then
        rm -f /bin/modprobe
        cat <<EOF >/bin/modprobe
#!/bin/sh
echo modprobe "\$@"
BEFORE=\$(date +%s)
busybox-i686 modprobe "\$@"
AFTER=\$(date +%s)
ELAPSED=\$(expr \$AFTER - \$BEFORE)
echo "modprobe time: \${ELAPSED} secs"
EOF
        chmod 755 /bin/modprobe
    fi
}

# /init must umount before switch_root
UmountProcSysDev()
{
    sync
    umount /dev/pts
    # On slow systems (qemu, vmware) /dev sometimes fails to unmount
    for N in 1 2 3 4 5 ; do
        # Cannot redirect to /dev/null so use dummy file in /
        umount /dev >/dummy 2>&1 && break
        sleep 1
    done
    rm -f /dummy
    umount /sys
    umount /proc
    umount /run
}

WaitForUuid()
{
    local i
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 ; do
        blkid | grep -q "${1:-BADUUID}" && break
        sleep 1
    done
}

WaitForDev()
{
    local i
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 ; do
        [ -e $1 ] && break
        sleep 1
    done
}

WaitForCdrom()
{
    local srdev

    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
        for blockdev in /dev/sr* ; do
            [ -e $blockdev ] || continue
            srdev+="$blockdev "
        done

        if [ -z "$srdev" ]; then
            sleep 1
        else
            sleep 3 && break
        fi
    done

    for i in $srdev; do echo $i; done
}

# Wait for up to 15 seconds for udev to fire and add usb drive
WaitForUsb()
{
    local usbdev

    lsmod | grep -q "usb_storage" || Error "CLI tuning cubesys.probeusb is not enabled"
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
        for blockdev in /sys/block/sd* ; do
            [ -e $blockdev ] || continue
            if readlink $blockdev | grep -q usb
            then
                local DEVNAME=$(basename $blockdev)
                usbdev+="$(lsblk -o NAME,FSTYPE /dev/$DEVNAME | grep $DEVNAME | awk '$2 != "" {print $1}' | grep -o "${DEVNAME}.*" | head -1 | xargs) "
            fi
        done
        if [ -z "$usbdev" ]; then
            sleep 1
        else
            sleep 3 && break
        fi
    done

    for i in $usbdev; do echo "/dev/$i"; done
}

Source()
{
    if [ -f "$1" ] ; then
        (
            [ $TRACE_INIT -eq 0 ] || set -x
            source "$1"
        )
    fi
}

SourceAndLog()
{
    if [ -f "$1" ] ; then
        (
            set -x
            source "$1"
        ) >> "$2" 2>&1
    fi
}

WhichPart()
{
    case $1 in
        *5) echo "1" ;;
        *6) echo "2" ;;
        *) echo "Unknown" ;;
    esac
}

# Usage: $PROG appliance_shutdown [<delay_secs>]
appliance_shutdown()
{
    local delay=${1:-0}

    Debug "Shutting down appliance"

    echo "System is shutting down in $delay minutes"
    if (( "$delay" > 0 )); then
        /sbin/shutdown -P +$delay
    else
        /sbin/poweroff
    fi
}

# Usage: $PROG appliance_reboot [<delay_secs>]
appliance_reboot()
{
    local delay=${1:-0}

    Debug "Rebooting appliance"

    echo "System is rebooting in $delay minutes"
    if (( "$delay" > 0 )); then
        /sbin/shutdown -r +$delay
    else
        /sbin/reboot
    fi
}

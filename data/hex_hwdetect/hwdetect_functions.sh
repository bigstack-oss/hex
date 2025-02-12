# HEX SDK

ETHTOOL=/sbin/ethtool

WaitForEthDev()
{
    local i
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 ; do
        $ETHTOOL eth$1 >/dev/null 2>&1 && break
        sleep 1
    done
}

# Return the Linux driver (e.g. e1000) for the specified interface index
# Usage: IfDrv <index>
# index     Interface index starting at 0, e.g. eth<index>
IfDrv()
{
    local DRIVER=$($ETHTOOL -i eth$1 2>/dev/null | grep driver | cut -d ' ' -f 2)
    echo $DRIVER
}

# Return hardware address for the specified interface index
# Usage: IfMac <index>
# index     Interface index starting at 0, e.g. eth<index>
IfMac()
{
    if ip link show eth$1 >/dev/null 2>&1 ; then
        ip link show eth$1 | grep "link/ether " | awk '{print tolower($2)}'
    fi
}

# Return 0 if the specified interface supports auto negotiation.
# Usage: IfAutoNeg <index>
# index     Interface index starting at 0, e.g. eth<index>
IfAutoNeg()
{
    # autoneg speed is meaningless with optical transceivers
    $ETHTOOL eth$1 2>/dev/null | grep -qi "Twisted Pair"
    [ $? -ne 0 ] && return 1   # 1 = no autoneg!

    $ETHTOOL eth$1 2>/dev/null | grep 'Supports auto-negotiation' | cut -d: -f2 | grep -qi Yes
    return $?
}

# Return list of supported link modes for the specified interface index
# Link modes are returned as a comma separated list, e.g. 10H,10F,100H,100F,1000F
# Usage: IfLinkModes <index>
# index     Interface index starting at 0, e.g. eth<index>
SupportedLinkModes()
{
    $ETHTOOL eth$1 2>/dev/null |
        sed -n '/Supported link modes/,$p' |
        sed -e '/Supports auto-negotiation:/,$d' -e '/Supported pause frame use:/,$d' |
        sed -e 's/Supported link modes://' -e 's/base.*\///g' -e 's/Half/H,/g' -e 's/Full/F,/g' |
        while read M ; do
            # Ignore non-standard link modes
            case "$M" in
                1000H) ;;
                "Not reported") ;;
                *) echo -n "$M" ;;
            esac
        done |
        sed -e 's/,$//' -e 's/ //g'
}
AdvertisedLinkModes()
{
    $ETHTOOL eth$1 2>/dev/null |
        sed -n '/Advertised link modes/,$p' |
        sed '/Advertised pause frame use:/,$d' |
        sed -e 's/Advertised link modes://' -e 's/base.*\///g' -e 's/Half/H,/g' -e 's/Full/F,/g' |
        while read M ; do
            # Ignore non-standard link modes
            case "$M" in
                1000H) ;;
                "Not reported") ;;
                *) echo -n "$M" ;;
            esac
        done |
        sed -e 's/,$//' -e 's/ //g'
}
IfLinkModes()
{
    # Use advertised link modes first unless it is empty
    local LINK_MODES="$(AdvertisedLinkModes $1)"
    [ -n "$LINK_MODES" ] || LINK_MODES="$(SupportedLinkModes $1)"
    echo "$LINK_MODES"
}

# Return PCI bus ID (e.g. 0000:14:08.0) for the specified interface index
# Usage: IfBusId <index>
# index     Interface index starting at 0, e.g. eth<index>
IfBusId()
{
    local BUSID
    case $(IfDrv $1) in
        virtio*)
            # ethtool does not properly report bus ID for virtio interfaces
            BUSID=$(find /sys/devices/pci* | grep virtio | grep net | grep "eth$1\$" | awk -F/ '{print $5}')
            ;;
        *)
            BUSID=$($ETHTOOL -i eth$1 2>/dev/null | grep bus-info | cut -d ' ' -f 2)
            ;;
    esac
    echo $BUSID
}

# Return 4-digit vendor ID (e.g. 8086) for the specified interface index
# Usage: IfVendor <index>
# index     Interface index starting at 0, e.g. eth<index>
IfVendor()
{
    # 0000:02:00.0 -> 02:00.0
    local X=$(echo $(IfBusId $1) | sed 's/0000:\(.\)/\1/')
    echo $(lspci -n | grep "$X" | cut -d\  -f3 | cut -d ':' -f 1)
}

# Return 4-digit device ID (e.g. 1480) for the specified interface index
# Usage: IfDev <index>
# index     Interface index starting at 0, e.g. eth<index>
IfDev()
{
    # 0000:02:00.0 -> 02:00.0
    local X=$(echo $(IfBusId $1) | sed 's/0000:\(.\)/\1/')
    echo $(lspci -n | grep "$X" | cut -d\  -f3 | cut -d ':' -f 2)
}

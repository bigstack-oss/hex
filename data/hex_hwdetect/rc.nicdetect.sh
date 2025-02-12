#!/bin/sh

. /etc/hwdetect.d/hwdetect_functions

FIXED_NIC_RULE=/etc/udev/rules.d/70-persistent-net.rules
DEFAULT_LINK_RULE=/etc/systemd/network/99-default.link
NICDETECT_DONE=/run/nicdetect_done

GetLinkModes()
{
    local LINK_MODES
    if IfAutoNeg $1 ; then
        LINK_MODES="Auto,$(IfLinkModes $1)"
    else
        # No autoneg means just one mode
        LINK_MODES=$(echo $(IfLinkModes $1) | sed -e 's/.*,//g')
    fi
    # On some Virtual platforms like XenServer, there is no
    # mode information available for interfaces. In such cases
    # safest to assume Auto.
    if [ -z "$LINK_MODES" ]; then
        LINK_MODES="Auto"
    fi
    echo $LINK_MODES
}

WaitForIfn()
{
    local i=0
    local prefix=$1
    local idx=$2
    local pciid=$3
    local timeout=${4:-9999}
    while [ $i -lt $timeout ]; do
        if $ETHTOOL $prefix$idx >/dev/null 2>&1; then
            if [ "$prefix" == "ifn" ]; then
                ip link set ifn$idx name eth$idx
            fi
            local drv=$(IfDrv $idx)
            local link=$(GetLinkModes $idx)
            local mac=$(IfMac $idx)
            local bus=$(IfBusId $idx)
            local vdr=$(IfVendor $idx)
            local dev=$(IfDev $idx)
            if [ -n "$drv" -a -n "$link" -a ${#mac} -eq 17 -a ${#vdr} -eq 4 -a ${#dev} -eq 4 -a ${#bus} -eq 12 ]; then
                if [ "$prefix" == "ifn" ]; then
                    if [ "$pciid" == "$bus" ]; then
                        break;
                    fi
                else
                    break
                fi
            fi
        fi
        sleep 1
        i=$(expr $i + 1)
    done
    [ $i -lt $timeout ]
}

UpdateNetworkSettings()
{
    local ID=$1

    echo "###### eth$ID ######" >> /etc/settings.sys
    echo "sys.net.if.label.eth$ID = IF.$(( $ID + 1 ))" >> /etc/settings.sys
    echo "sys.net.if.description.eth$ID = Interface $(( $ID + 1 ))" >> /etc/settings.sys
    echo "sys.net.if.driver.eth$ID = $(IfDrv $ID)" >> /etc/settings.sys
    echo "sys.net.if.link_modes.eth$ID = $(GetLinkModes $ID)" >> /etc/settings.sys
    echo "sys.net.if.mac.eth$ID = $(IfMac $ID)" >> /etc/settings.sys
    echo "sys.net.if.businfo.eth$ID = $(IfBusId $ID)" >> /etc/settings.sys
    echo "sys.net.if.vendor.eth$ID = $(IfVendor $ID)" >> /etc/settings.sys
    echo "sys.net.if.device.eth$ID = $(IfDev $ID)" >> /etc/settings.sys
}

if cat /proc/cmdline | grep -wq nicdetect_disable ; then
    exit 0
fi

PREFIX="ifn"
if [ ! -f $FIXED_NIC_RULE -o -f $NICDETECT_DONE ]; then
    PREFIX="eth"
fi

echo "[Link]" > $DEFAULT_LINK_RULE
echo "MACAddressPolicy=none" >> $DEFAULT_LINK_RULE

# remove all existing interface settings
TMPSYS=$(mktemp -u /tmp/settings.XXX)
sed -e "/# Network interfaces/d" -e "/###### eth.* ######/d" -e "/sys.net.if/d" /etc/settings.sys >$TMPSYS
# sanitize potentially mal-formed file before replacing
if [[ "$(diff $TMPSYS /etc/settings.sys 2>&1)" =~ Binary ]]; then
    BADLINE=$(cat $TMPSYS | wc -l)
    sed -i "${BADLINE}d" $TMPSYS
fi
cp -f $TMPSYS /etc/settings.sys

# To generate a list of settings for each interface. e.g.
#  sys.net.if.label.eth0 = IF.1
#  sys.net.if.description.eth0 = Interface 1
#  sys.net.if.driver.eth0 = 8139cp
#  sys.net.if.link_modes.eth0 = Auto,10F,100F,1000F
#  sys.net.if.mac.eth0 = 52:54:00:b5:55:e9
#  sys.net.if.businfo.eth0 = 0000:00:03.0
#  sys.net.if.vendor.eth0 = 10ec
#  sys.net.if.device.eth0 = 8139

echo "# Network interfaces" >> /etc/settings.sys
IDX=0
for PCIID in $(for D in $(ls /sys/class/pci_bus/*\:*/device/*\:*/class); do echo "$D:$(cat $D)"; done | grep 0x020000 | awk -F'/' '{print $7}' | sort)
do
    if WaitForIfn $PREFIX $IDX $PCIID 60 ; then
        UpdateNetworkSettings $IDX
    else
        echo "$PREFIX$IDX ($PCIID) detection timeout"
    fi
    IDX=$(( $IDX + 1 ))
done

touch $NICDETECT_DONE

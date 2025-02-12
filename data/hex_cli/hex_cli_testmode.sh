#!/bin/sh

PROG=$(basename $0)
HEX_SCRIPTSDIR=/usr/lib/hex_sdk
if [ -f $HEX_SCRIPTSDIR/functions ]; then
    . $HEX_SCRIPTSDIR/functions
else
    echo "$PROG: functions not found" >&2
    exit 1
fi

Usage()
{
    echo "Usage: $PROG on | off" >&2
    exit 1
}

SetTestMode()
{
    SYS=/etc/settings.sys
    NEW=/etc/settings.sys.new
    sed -e '/^sys.cli.testmode/d' $SYS > $NEW
    echo "sys.cli.testmode = $1" >> $NEW
    mv $NEW $SYS
}

if [ $# -ne 1 ]; then
    Usage
elif [ "$1" = "on" ]; then
    SetTestMode 1
elif [ "$1" = "off" ]; then
    SetTestMode 0
else
    Usage
fi

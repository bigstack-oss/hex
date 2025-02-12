#!/bin/sh

PROG=$(basename $0)
HEX_SCRIPTSDIR=/usr/lib/hex_sdk
if [ -f $HEX_SCRIPTSDIR/functions ]; then
    . $HEX_SCRIPTSDIR/functions
else
    echo "$PROG: functions not found" >&2
    exit 1
fi

Usage() {
    echo "Usage: $PROG [ -v ] <policy-path> ..." >&2
    echo "-v    Enable debug messages" >&2
    exit 1
}

DEBUG_FLAG=

while getopts "v" OPT ; do
    case $OPT in
        v) DEBUG_FLAG=-v ;;
        *) Usage ;;
    esac
done
shift $(($OPTIND - 1))

[ $# -gt 0 ] || Usage

# Validate arguments
for I in $* ; do
    P="$(echo $I | sed 's!/etc/policies/!!')"
    [ "$I" = "/etc/policies/$P" ] || Error "Invalid policy file: $I"
    [ -f "/etc/policies/$P" ] || Error "Invalid policy file: $I"
done

# Copy policies to be edited to temp directory
# Build up vi command
TEMPDIR=$(MakeTempDir)
COMMAND="vi"
for I in $* ; do
    P="$(echo $I | sed 's!/etc/policies/!!')"
    D=$(dirname $P)
    mkdir -p $TEMPDIR/$D
    cp /etc/policies/$P $TEMPDIR/$P
    COMMAND="$COMMAND $TEMPDIR/$P"
done

# Edit policies
eval $COMMAND

# Check if anything has been changed
CHANGES=0
for I in $* ; do
    P="$(echo $I | sed 's!/etc/policies/!!')"
    cmp -s /etc/policies/$P $TEMPDIR/$P || CHANGES=1
done

if [ $CHANGES -eq 0 ]; then
    echo "No changes detected"
    exit 0
else
    echo "Committing changes..."
    # Suppress debug messages from console (they will still be in syslog)
    hex_config -e $DEBUG_FLAG commit $TEMPDIR 2>&1 | grep -v 'Debug: '
    exit $?
fi

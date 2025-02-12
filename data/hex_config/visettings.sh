#!/bin/sh

PROG=$(basename $0)
HEX_SCRIPTSDIR=/usr/lib/hex_sdk
if [ -f $HEX_SCRIPTSDIR/functions ]; then
    . $HEX_SCRIPTSDIR/functions
else
    echo "$PROG: functions not found" >&2
    exit 1
fi

ViUsage() {
    echo "Usage: $PROG [ -v ] [ -d ]" >&2
    echo "-v    Enable hex_config debug messages" >&2
    echo "-d    Restore from defaults before editing" >&2
    exit 1
}

Usage() {
    echo "Usage: $PROG [ -v ]" >&2
    echo "-v    Enable hex_config debug messages" >&2
    exit 1
}

DEBUG_FLAG=
ORIG_SETTINGS=/etc/settings.txt
DEFAULT_SETTINGS=/etc/settings.def
MNT_DIR=/mnt/install
SAVED_SETTINGS=/mnt/install/settings.txt

case "$PROG" in
    visettings)
        while getopts "vd" OPT ; do
            case $OPT in
                v) DEBUG_FLAG=-v ;;
                d) ORIG_SETTINGS=$DEFAULT_SETTINGS ;;
                *) ViUsage ;;
            esac
        done
        shift $(($OPTIND - 1))
        [ $# -eq 0 ] || ViUsage
        ;;
    *)
        while getopts "v" OPT ; do
            case $OPT in
                v) DEBUG_FLAG=-v ;;
                *) Usage ;;
            esac
        done
        shift $(($OPTIND - 1))
        [ $# -eq 0 ] || Usage
        ;;
esac

NEW_SETTINGS=$(MakeTemp)
[ -f $ORIG_SETTINGS ] || touch $ORIG_SETTINGS

CommitChanges()
{
    if cmp -s $ORIG_SETTINGS $NEW_SETTINGS ; then
        echo "No changes detected"
        exit 0
    else
        echo "NOTE: Changes will not be in sync with LMI. Please reboot before using LMI again."
        echo "Committing changes..."
        # Suppress debug messages from console (they will still be in syslog)
        hex_config -e $DEBUG_FLAG commit $NEW_SETTINGS 2>&1 | grep -v 'Debug: '
        exit $?
    fi
}

case "$PROG" in
    visettings)
        cp $ORIG_SETTINGS $NEW_SETTINGS
        vi $NEW_SETTINGS
        CommitChanges
        ;;
    savesettings)
        mount -o remount,rw $MNT_DIR
        cp $ORIG_SETTINGS $SAVED_SETTINGS
        mount -o remount,ro $MNT_DIR
        echo "Settings saved"
        exit 0
        ;;
    restoresettings)
        if [ -f $SAVED_SETTINGS ]; then
            cp $SAVED_SETTINGS $NEW_SETTINGS
            CommitChanges
        else
            echo "No saved settings"
            exit 1
        fi
        ;;
esac

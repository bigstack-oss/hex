#!/bin/sh

PROG=$(basename $0)
HEX_SCRIPTSDIR=/usr/lib/hex_sdk
if [ -f $HEX_SCRIPTSDIR/functions ]; then
    . $HEX_SCRIPTSDIR/functions
else
    echo "$PROG: functions not found" >&2
    exit 1
fi

ROLLBACK_DIR='/var/fixpack_rollback'

ExitError()
{
    logger -t $PROG "$1"
    exec 1>&3
    exec 2>&4
    echo "$1"
    exit $2
}

Usage()
{
    echo "Usage: $PROG [args]"
    echo "   -i <fixpack_path>    Install a fixpack"
    echo "   -u                   Uninstall most recently installed fixpack"
    exit 1
}

Rollback()
{
    if [ ! -d ${ROLLBACK_DIR}/fixpack-0 ]; then
        echo "There are no rollback points."
        exit 0
    fi

    . ${ROLLBACK_DIR}/fixpack-0/fixpack.info

    (
        set -e
        # execute pre rollback script
        [ -e ${ROLLBACK_DIR}/fixpack-0/pre_rollback.sh ] && . ${ROLLBACK_DIR}/fixpack-0/pre_rollback.sh ${ROLLBACK_DIR}/fixpack-0 || true
    ) && STATUS=0 || STATUS=$?

    if [ $STATUS -ne 0 ]; then
        logger -t $PROG "$FIXPACK_NAME pre_rollback failed"
        echo "$FIXPACK_NAME pre_rollback failed"
        exit 1
    fi

    (
        set -e
        tar -zx -f ${ROLLBACK_DIR}/fixpack-0/backup.tgz -C /
    ) && STATUS=0 || STATUS=$?

    if [ $STATUS -ne 0 ]; then
        logger -t $PROG "$FIXPACK_NAME uninstall failed"
        echo "$FIXPACK_NAME uninstall failed"
        exit 1
    fi

    (
        set -e
        # execute post rollback script
        [ -e ${ROLLBACK_DIR}/fixpack-0/post_rollback.sh ] && . ${ROLLBACK_DIR}/fixpack-0/post_rollback.sh ${ROLLBACK_DIR}/fixpack-0 || true
    ) && STATUS=0 || STATUS=$?

    if [ $STATUS -ne 0 ]; then
        logger -t $PROG "$FIXPACK_NAME post_rollback failed"
        echo "$FIXPACK_NAME post_rollback failed"
        exit 1
    fi

    rm -rf ${ROLLBACK_DIR}/fixpack-0
    for N in 1 2 3 4 5 6 7 8 9; do
        [ -d ${ROLLBACK_DIR}/fixpack-$N ] && mv ${ROLLBACK_DIR}/fixpack-$N ${ROLLBACK_DIR}/fixpack-`expr $N - 1`
    done

    logger -t $PROG "$FIXPACK_NAME uninstall successful"
    echo "$FIXPACK_NAME uninstall successful"
    /usr/sbin/hex_config fixpack_add_history "$FIXPACK_ID" "$FIXPACK_NAME" "NO" "$FIXPACK_DESCRIPTION" "Uninstalled"
    exit 0
}

Cleanup()
{
    STATUS=$?

    # Umount fixpack
    sync
    umount $TEMPLOOP
    UnmountRemoveLoop $LOOPDEV

    # Log success/failure to syslog and insert into fix pack history
    if [ $STATUS -eq 0 ]; then
        logger -t $PROG "$FIXPACK install successful"
        echo "$FIXPACK install successful"
        exec 1>&3
        echo "$FIXPACK install successful"
        /usr/sbin/hex_config fixpack_add_history "$FIXPACK_ID" "$FIXPACK_NAME" "$ROLLBACK" "$FIXPACK_DESCRIPTION" "Installed"
    else
        logger -t $PROG "$FIXPACK install failed"
        echo "$FIXPACK install failed"
        exec 1>&3
        echo "$FIXPACK install failed"
    fi
}

while getopts "i:u" OPT ; do
    case $OPT in
        i) FIXPACKFILE="$OPTARG" ;;
        u) Rollback ;;
        *) Usage ;;
    esac
done
shift $(($OPTIND - 1))

if [ $# -lt 0 ]; then
    Usage
fi

FIXPACK=$(basename $FIXPACKFILE .fixpack)

FIXPACK_DIR=/var/support/fixpack
[ -d $FIXPACK_DIR ] || mkdir -p $FIXPACK_DIR

# Save output to /var/support/fixpack/<fixpack>.log
exec 3>&1    #save stdout
exec 4>&2    #save stderr
exec 1>>/var/support/fixpack/$FIXPACK.log    #redirect stdout to file
exec 2>>/var/support/fixpack/$FIXPACK.log    #redirect stderr to file

# Verify fixpack file exists
[ -e "$FIXPACKFILE" ] || ExitError "$FIXPACK: file not found" 2

# Verify fixpack signature
/bin/true || ExitError "$FIXPACK: Failed to verify signature" 3
echo "$FIXPACK: Signature verified" >&3

# Mount fixpack
LOOPDEV=$(SetupLoop $FIXPACKFILE)
trap Cleanup INT TERM EXIT
TEMPLOOP=$(MakeTempDir)

mount -o ro -t ext4 $LOOPDEV $TEMPLOOP

# Get the info file for NAME, DESCRIPTION, ID, SUPPORTED_FIRMWARES.
[ -e "$TEMPLOOP/fixpack.info" ] || ExitError "$FIXPACK: Fix pack is not in the correct format" 4
. $TEMPLOOP/fixpack.info

if [ -n "$SUPPORTED_FIRMWARES" ]; then
    SUPPORTED=0
    VERSION=`/bin/grep 'sys.product.version' /etc/settings.sys | /usr/bin/awk '{print $NF}'`
    for entry in $SUPPORTED_FIRMWARES
    do
        if [ "$VERSION" = "$entry" ]; then
            SUPPORTED=1
            break
        fi
    done
    if [ "$SUPPORTED" -eq "0" ]; then
        ExitError "$FIXPACK: fixpack is not valid for this firmware version" 9
    fi
fi

# Check no rollback flag
if [ -e "$TEMPLOOP/norollback" ]; then
    ROLLBACK="No"
else
    ROLLBACK="Yes"
fi

# Run install.sh
[ -e "$TEMPLOOP/install.sh" ] || ExitError "$FIXPACK: Failed to locate install script" 5
sh $TEMPLOOP/install.sh $TEMPLOOP $ROLLBACK_DIR >>/var/support/fixpack/$FIXPACK.log 2>&1


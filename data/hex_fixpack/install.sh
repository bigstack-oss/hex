#!/bin/sh

PROG=$(basename $0)

HEX_SCRIPTSDIR=/usr/lib/hex_sdk
if [ -f $HEX_SCRIPTSDIR/functions ]; then
    . $HEX_SCRIPTSDIR/functions
else
    echo "$PROG: functions not found"
    exit 1
fi

set -x

Usage() {
    echo "Usage: $PROG <mount directory> <rollback directory>"
    echo "  <mount directory>         Path to where the fixpack is currently mounted."
    echo "  <rollback directory>      Location of rollback directory into which to store restore files."
    exit 1
}

Restore() {
    if [ -f $tmpdir/backup.tgz ]; then
        tar -zx -f $tmpdir/backup.tgz -C / --exclude dev/null
    fi
    rm -rf $tmpdir
}

[ $# -eq 2 ] || Usage
FIXPACK_MNT=$1
. $FIXPACK_MNT/fixpack.info

ROLLBACK_DIR=$2
mkdir -p $ROLLBACK_DIR
tmpdir=$(mktemp -d $ROLLBACK_DIR/fixpack.XXXXXX)
STATUS=1

if hex_sdk | grep -q "license_check"; then
    # Check license in v2.x
    LC=$(hex_sdk -v license_check || true)
    if echo $LC | grep -q -i "expired\|compromised\|Invalid license\|not installed"; then
        echo "License check failed $LC, aborting installation"
        exit $STATUS
    fi
else
    # Verify if HW SN is registered
    SN=$(dmidecode --type system | grep "System Information" -A 8 | grep "Serial Number" | awk '{print $3}')
    if ! grep -q "${SN:-.*}" $FIXPACK_MNT/sn.lst; then
        echo "Unable to verify SN: $SN, aborting installation"
        exit $STATUS
    fi
fi

# Pre-install
(
    set -e
    # execute pre installscript
    [ -e $FIXPACK_MNT/pre_install.sh ] && . $FIXPACK_MNT/pre_install.sh $FIXPACK_MNT || true
) && STATUS=0 || STATUS=$?

if [ $STATUS -ne 0 ]; then
    echo "Unable to execute pre_install script, aborting installation"
    exit $STATUS
fi

# Backup files
if [ -f $FIXPACK_MNT/backup.lst ] && [ -s $FIXPACK_MNT/backup.lst ]; then
    (
        set -e
        cd / && tar --ignore-failed-read -cz -T $FIXPACK_MNT/backup.lst -f $tmpdir/backup.tgz 2>/dev/null
    ) && STATUS=0 || STATUS=$?
else
    (
        set -e
        cd / && tar -cz -f $tmpdir/backup.tgz /dev/null
    ) && STATUS=0 || STATUS=$?
fi

if [ $STATUS -ne 0 ]; then
    echo "Unable to backup files, aborting installation"
    exit $STATUS
fi

# Remove files no longer needed
if [ -e $FIXPACK_MNT/remove.lst ] ; then
    while read filename
    do
        rm -f /${filename}
    done <$FIXPACK_MNT/remove.lst
fi

# Copy manifest and info to tmpdir
[ -e $FIXPACK_MNT/manifest ] && cp $FIXPACK_MNT/manifest $tmpdir || true
[ -e $FIXPACK_MNT/norollback ] && cp $FIXPACK_MNT/norollback $tmpdir || true
[ -e $FIXPACK_MNT/pre_rollback.sh ] && cp $FIXPACK_MNT/pre_rollback.sh $tmpdir || true
[ -e $FIXPACK_MNT/post_rollback.sh ] && cp $FIXPACK_MNT/post_rollback.sh $tmpdir || true
cp $FIXPACK_MNT/fixpack.info $tmpdir

# Extract hotfix files
(
    set -e
    [ -e $FIXPACK_MNT/rootfs.cgz ] && ExtractCgz $FIXPACK_MNT/rootfs.cgz / || true
) && STATUS=0 || STATUS=$?

if [ $STATUS -ne 0 ]; then
    Restore
    echo "Unable to extract files, restoring files and aborting installation"
    exit $STATUS
fi

# Post-install
(
    set -e
    [ -e $FIXPACK_MNT/post_install.sh ] && . $FIXPACK_MNT/post_install.sh $FIXPACK_MNT || true
) && STATUS=0 || STATUS=$?

if [ $STATUS -ne 0 ]; then
    Restore
    echo "Unable to execute post_install script, restoring files and aborting installation"
    exit $STATUS
fi

# Save rollback dir
rm -rf $ROLLBACK_DIR/fixpack-9
for N in 8 7 6 5 4 3 2 1 0; do
    [ -d $ROLLBACK_DIR/fixpack-$N ] && mv $ROLLBACK_DIR/fixpack-$N $ROLLBACK_DIR/fixpack-`expr $N + 1`
done
mv $tmpdir $ROLLBACK_DIR/fixpack-0

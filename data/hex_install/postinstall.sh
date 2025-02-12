#!/bin/sh
# hex/postinstall.sh is used in the first boot after hex_install and should never fail

HEX_SDK=/usr/sbin/hex_sdk

# Adapt policies to hardware
/usr/sbin/hex_translate adapt

# Create factory default policy
rm -rf /etc/default_policies
cp -r /etc/policies /etc/default_policies

# Create factory default policy snapshot
COMMENTFILE=/tmp/comment.$$
echo "Factory defaults" > $COMMENTFILE
/usr/sbin/hex_config -- snapshot_create -h unconfigured $COMMENTFILE >/dev/null 2>&1
rm -f $COMMENTFILE

# add update history entry if this is refresh install
# otherwise, postpone the task after data migration in postupdate.sh
if [ $UPDATE -eq 0 ]; then
    $HEX_SDK AddUpdateHistory /etc/version firmware
fi


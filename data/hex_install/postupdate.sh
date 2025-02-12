#!/bin/sh
# hex/postupdate.sh is used in the first boot after hex_install and should never fail

HEX_SDK=/usr/sbin/hex_sdk
HEX_TUNING=/usr/sbin/hex_tuning
HEX_TRANSLATE=/usr/sbin/hex_translate
HEX_CONFIG=/usr/sbin/hex_config

umount_target()
{
    sync
    umount /mnt/target
}

trap 'umount_target' INT TERM EXIT

if [ "$TRACE" = 1 ]; then
    HEX_FLAGS="-ve"
fi

# mount another partition in /mnt/target
$HEX_SDK MountOtherPartition

# source current new product version "T_sys_product_version"
source $HEX_TUNING /etc/settings.sys sys.product.version

# source the old product version as well, "X_sys_product_version"
source $HEX_TUNING "-p X_" /mnt/target/etc/settings.sys sys.product.version

# migrating policies from old partition
$HEX_TRANSLATE $HEX_FLAGS migrate "$T_sys_product_version" /mnt/target

# do not migrate postinstall.log over one created by this script
rm -f /mnt/target/var/log/postinstall.log

# migrating system artificates (db, certs, etc) from old partition
$HEX_CONFIG $HEX_FLAGS migrate "$X_sys_product_version" /mnt/target

# update install history
$HEX_SDK AddUpdateHistory /etc/version firmware

umount /mnt/target

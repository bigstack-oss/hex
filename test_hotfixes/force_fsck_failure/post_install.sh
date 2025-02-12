#!/bin/sh

GRUBCFG="/boot/grub/menu.lst"

sed -i -e 's/bzImage/bzImage force_fsck_failure/' $GRUBCFG
exit 0

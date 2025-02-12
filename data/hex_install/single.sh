#!/bin/sh

# 1: Partition 1
# 2: Partition 2
# 3: Create backup of partition 1
# 4: Create backup of partition 2
# 5: Partition 1 single user
# 6: Partition 2 single user
default=
case $(grub-get-default) in
    1) default=5 ;;
    2) default=6 ;;
    *) ;;
esac
if [ -n "$default" ]; then
    grub-set-default $default
    echo "Rebooting to single user mode"
    reboot
else
    echo "Already in single user mode"
fi

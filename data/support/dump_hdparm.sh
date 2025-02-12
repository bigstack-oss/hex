#!/bin/bash
touch /etc/settings.sys
source hex_tuning /etc/settings.sys sys.install.hdd
[ -z "$T_sys_install_hdd" ] || hdparm -I $T_sys_install_hdd

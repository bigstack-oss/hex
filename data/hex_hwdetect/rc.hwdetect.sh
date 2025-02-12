#!/bin/sh

source /etc/init_functions

TRACE_FLAG=
[ $TRACE_INIT -eq 0 ] || TRACE_FLAG=-x

# Output hardware detection results during live/installer boot on all consoles
/usr/sbin/hex_hwdetect $TRACE_FLAG | tee -a $(GetKernelConsoleDevices) > /dev/null

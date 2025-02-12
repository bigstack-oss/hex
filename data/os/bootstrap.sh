#!/bin/sh
# Default hostname to "unconfigured" instead of empty (a.k.a., "(none)")
# Will be changed by hex_config bootstrap
systemctl restart systemd-hostnamed
/usr/bin/hostnamectl set-hostname unconfigured

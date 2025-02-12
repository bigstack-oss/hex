#!/bin/sh

/usr/sbin/hex_sdk banner_login | /usr/sbin/hex_banner
/usr/bin/reset
exec /bin/login

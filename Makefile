# HEX SDK

include ../build.mk

# Additonal fakeroot build tools required by HEX
SUBDIRS += fakeroot

# HEX base root filesystem
SUBDIRS += rootfs

# HEX component sources
SUBDIRS += src

include $(HEX_MAKEDIR)/hex_sdk.mk


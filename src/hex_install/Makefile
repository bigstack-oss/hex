# HEX SDK

include ../../../build.mk

# Installer ramdisk used by projects
ALL = $(HEX_INSTALL_RD)

# Delete _debug and .size files
PKGCLEAN += $(patsubst %.cgz,%*,$(HEX_INSTALL_RD))

# projrootfs.mk => $(PROJ_ROOTFS): $(PROJ_BASE_ROOTFS) $(PROJ_INITTABS) $(PROGRAMS)

PROGRAMS = hex_config hex_cli
DISABLE_SYMLINK_HEX_BIN = 1

hex_cli_MODULES = cli_appliance.o cli_install.o
hex_config_MODULES = config_appliance.o

PROJ_NAME = hex_install

PROJ_BUILD_ROOTFS = 1

# Use base rootfs
PROJ_BASE_ROOTFS = $(HEX_BASE_ROOTFS)

# Install directly to final ramdisk image
PROJ_ROOTFS = $(HEX_INSTALL_RD)

include $(HEX_MAKEDIR)/hex_sdk.mk

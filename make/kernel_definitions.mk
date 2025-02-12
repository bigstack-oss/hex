# HEX SDK

# Kernel version to be used by fakeroot tools, rootfs, depmod, etc.

KER_INFO := $(shell [ -f /kernel-info ] || dnf info kernel > /kernel-info)

KER_VER := $(shell cat /kernel-info | grep Version | tail -1 | awk '{print $$3}')
KER_REL := $(shell cat /kernel-info | grep Release | tail -1 | awk '{print $$3}')
KERNEL_VERS ?= $(KER_VER)-$(KER_REL).$(HEX_ARCH)

# Official kernel packages
KERNEL_PKG := kernel.$(HEX_ARCH)
KERNEL_EXTRA_PKG := kernel-modules.$(HEX_ARCH) kernel-modules-extra.$(HEX_ARCH)
KERNEL_FIRMWARE_PKG := linux-firmware

# Kernel image inside kernel rpm
KERNEL_IMAGE := $(KERNEL_VERS)/linux
KERNEL_INITRD := $(KERNEL_VERS)/initrd

# Kernel modules directory in rootfs
KERNEL_MODULE_DIR := lib/modules/$(KERNEL_VERS)

# Kernel firmware directory in rootfs
KERNEL_FIRMWARE_DIR := lib/firmware

KERNEL_PKGS = $(KERNEL_PKG) $(KERNEL_EXTRA_PKG) $(KERNEL_FIRMWARE_PKG)

HEX_EXPORT_VARS += KERNEL_VERS KERNEL_PKG KERNEL_EXTRA_PKG KERNEL_FIRMWARE_PKG KERNEL_IMAGE KERNEL_MODULE_DIR KERNEL_FIRMWARE_DIR

# HEX SDK

include ../../build.mk

# Avoid conflicts with project's fakeroot packages
FAKEROOT_DIR := /hex-fakeroot-tools

MACHINE_ID := $(shell cat /etc/machine-id)

$(call FAKEROOT_INSTALL,kernel_dir,\
	$(HEX_BOOTDIR)/$(KERNEL_VERS),\
	[ -e $(HEX_BOOTDIR)/$(KERNEL_VERS) ] || mkdir -p {})

$(call FAKEROOT_INSTALL,linux,\
	/boot/vmlinuz-$(KERNEL_VERS) $(HEX_BOOTDIR)/$(KERNEL_IMAGE),\
	[ -e $(HEX_BOOTDIR)/$(KERNEL_IMAGE) ] || mv -f {})

$(call FAKEROOT_INSTALL,initrd,\
	/boot/initramfs-$(KERNEL_VERS).* $(HEX_BOOTDIR)/$(KERNEL_INITRD),\
	[ -e $(HEX_BOOTDIR)/$(KERNEL_INITRD) ] || mv -f {})

include $(HEX_MAKEDIR)/hex_sdk.mk


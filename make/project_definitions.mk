# HEX SDK

# Project-specific packages (can be overwritten in project.mk)
PROJ_PKGDIR        ?= $(TOP_SRCDIR)/core/pkg
PROJ_OSSDIR        ?= $(PROJ_PKGDIR)/$(HEX_ARCH)/oss
PROJ_THIRDPARTYDIR ?= $(PROJ_PKGDIR)/$(HEX_ARCH)/thirdparty
HEX_EXPORT_VARS += PROJ_PKGDIR PROJ_OSSDIR PROJ_THIRDPARTYDIR

# Project-specific libraries, images (can be overwritten in project.mk)
PROJ_LIBDIR ?= $(TOP_BLDDIR)/lib
PROJ_IMGDIR ?= $(TOP_BLDDIR)/img
PROJ_SHMODDIR ?= $(TOP_SRCDIR)/core/sdk_sh
PROJ_PROFILEDIR ?= $(TOP_SRCDIR)/core/profile
HEX_EXPORT_VARS += PROJ_LIBDIR PROJ_IMGDIR PROJ_PROFILEDIR

# Project definitions (can be overridden in current makefile)
# PROJ_VENDOR_XXX should be customizable
PROJ_VENDOR_COMPANY  := Bigstack
PROJ_VENDOR_NAME     := Hex
PROJ_VENDOR_LONGNAME := Hex Appliance
PROJ_VENDOR_VERSION  := 1.0
PROJ_NAME         := Hex
PROJ_LONGNAME     := Hex Appliance
PROJ_VERSION      := 1.0
PROJ_HARDWARE     := qemu
PROJ_KERNEL       := $(HEX_KERNEL)
PROJ_INITRD       := initramfs.cgz
PROJ_FIRMWARE	  := $(HEX_FIRMWARE)
PROJ_RAMDISK_PADDING := 10M
PROJ_BASE_ROOTFS  = $(HEX_FULL_ROOTFS)
PROJ_ROOTFS       := rootfs.cgz
PROJ_ROOTFS_DEBUG := rootfs_debug.cgz
PROJ_INITRD_DEBUG := initramfs_debug.cgz

PROJ_BUILD_ROOTFS   := 0
PROJ_BUILD_PPU      := 0
PROJ_BUILD_USB      := 0
PROJ_BUILD_LIVE_USB := 0
PROJ_BUILD_ISO      := 0
PROJ_BUILD_LIVE_ISO := 0
PROJ_BUILD_OVA      := 0
PROJ_BUILD_VAGRANT  := 0
PROJ_BUILD_PXE      := 0
PROJ_BUILD_PXESERVER     := 0
PROJ_BUILD_PXESERVER_ISO := 0
PROJ_BUILD_PXEDEPLOY     := 0

# Location of project deliverables
PROJ_SHIPDIR := $(BLDDIR)/ship

# Default settings
PROJ_SETTINGS := $(wildcard $(SRCDIR)/settings.txt)

# System settings
PROJ_SYS_SETTINGS := $(wildcard $(SRCDIR)/settings.sys)

PROJ_PPU := proj.pkg
PROJ_PPU_PADDING := 16M
PROJ_ROOTFS_MD5 := proj_rootfs.md5
PROJ_ROOTFS_COMMIT := proj_rootfs.commit

PROJ_PPUISO := proj.pkgiso
PROJ_PPUISO_PADDING := 1M

PROJ_ISO := proj.iso
PROJ_ISO_RD := iso_initramfs.cgz
PROJ_ISO_PADDING := 1M

PROJ_LIVE_ISO := live_proj.iso
PROJ_LIVE_ISO_RD := live_iso_initramfs.cgz
PROJ_LIVE_ISO_PADDING := 1M

# Use .pxe.tgz instead of just .pxe so that its easier to unpack
PROJ_PXE := proj.pxe.tgz
PROJ_PXE_RD := pxe_initramfs.cgz

PROJ_PXESERVER := proj_pxeserver.img
PROJ_PXESERVER_RD := pxeserver_initramfs.cgz
PROJ_PXESERVER_PADDING := 1M

PROJ_PXESERVER_ISO := proj_pxeserver.iso
PROJ_PXESERVER_ISO_RD := pxeserver_iso_initramfs.cgz
PROJ_PXESERVER_ISO_PADDING := 1M

# Use .img instead of .usb since its compatible with Linux imagewriter
PROJ_USB := proj.img
PROJ_USB_RD := usb_initramfs.cgz
PROJ_USB_PADDING := 1M

# Use .img instead of .usb since its compatible with Linux imagewriter
PROJ_LIVE_USB := live_proj.img
PROJ_LIVE_USB_RD := live_usb_initramfs.cgz
PROJ_LIVE_USB_PADDING := 1M

ifneq ($(PROJ_README_TEMPLATE),)
PROJ_README := $(PROJ_SHIPDIR)/README.txt
endif

# Product-specific boot time installation script
PROJ_INIT_SCRIPT := $(SRCDIR)/proj_init.conf

PROJ_OVA := proj.ova
PROJ_OVA_RAW := ova.raw
PROJ_OVA_ISO := ova.iso
PROJ_OVA_RD  := ova_initramfs.cgz

# Hardware profile to use
PROJ_OVA_HARDWARE := vmware

# Size of the virtual disk
PROJ_OVA_SIZE := 8G

# Amount of extra space to leave on ISO boot image
PROJ_OVA_PADDING := 1M

PROJ_VAGRANT     := proj.box
PROJ_VAGRANT_ISO := vagrant.iso
PROJ_VAGRANT_RD  := vagrant_initramfs.cgz
PROJ_VAGRANT_SIZE := 8G
PROJ_VAGRANT_PADDING := 1M
PROJ_VAGRANT_HARDWARE := vbox

PROJ_ENABLE_ROOT_SHELL := 1

# Always use UTC timezone for consistency
# Build scripts should set PROJ_BUILD_LABEL
ifeq ($(PROJ_BUILD_LABEL),)
PROJ_BUILD_LABEL := $(shell env TZ=UTC date +"%Y%m%d-%H%M")
endif

# Assume using git for source code management
# Get the last commit
ifeq ($(PROJ_BUILD_COMMIT),)
PROJ_BUILD_COMMIT := $(shell git -C $(TOP_SRCDIR) log -1 --oneline | awk '{printf $$1}')
endif

# Assume that all builds are initially developer signed (until resigned for beta or release)
# Mark non-production builds
ifeq ($(PRODUCTION),1)
PROJ_BUILD_DESC := $(PROJ_BUILD_COMMIT)
else
PROJ_BUILD_DESC := $(PROJ_BUILD_COMMIT)
endif

# Assume that all builds are initially developer signed (until resigned for beta or release)
# Mark non-production builds
ifeq ($(TRIAL),1)
PROJ_BUILD_DESC := $(PROJ_BUILD_COMMIT)_trial
else
PROJ_BUILD_DESC := $(PROJ_BUILD_COMMIT)
endif

# Install function helper
# Usage: $(call PROJ_INSTALL_HELPER,install-script,flags,src,dest)
define PROJ_INSTALL_HELPER
ifneq ($(1),INSTALL_DNF)
ifneq ($(1),INSTALL_APT)
ifneq ($(1),INSTALL_PIP)
# Register dependency
$(PROJ_ROOTFS): $(3)
endif
endif
endif

# ARC only
# Add dependency on .exclude file if it exists
ifeq ($(1),INSTALL_ARC)
$(3)_exclude := $(patsubst %.tgz,%.exclude,$(patsubst %.tar.gz,%.exclude,$(3)))
ifneq ($$(wildcard $$($(3)_exclude)),)
$(PROJ_ROOTFS): $$($(3)_exclude)
endif
endif

# Install action
rootfs_install::
	$$(Q)$($(1)) $(2) $$(ROOTDIR) $(3) $(4)
endef

# Install program
# Usage: $(call PROJ_INSTALL_PROGRAM,flags,src,dest)
define PROJ_INSTALL_PROGRAM
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_PROGRAM,$(1),$(2),$(3)))
endef

# Install shared library
# Usage: $(call PROJ_INSTALL_SO,flags,src,dest)
define PROJ_INSTALL_SO
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_SO,$(1),$(2),$(3)))
endef

# Install single/custom Linux kernel module
# Usage: $(call PROJ_INSTALL_LKM,flags,src,dest)
define PROJ_INSTALL_LKM
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_LKM,$(1),$(2),$(3)))
endef

# Install script
# Usage: $(call PROJ_INSTALL_SCRIPT,flags,src,dest)
define PROJ_INSTALL_SCRIPT
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_SCRIPT,$(1),$(2),$(3)))
endef

# Install data
# Usage: $(call PROJ_INSTALL_DATA,flags,src,dest)
define PROJ_INSTALL_DATA
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_DATA,$(1),$(2),$(3)))
endef

# Install packages with APT
# Usage: $(call PROJ_INSTALL_APT,flags,src ...)
define PROJ_INSTALL_APT
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_APT,$(1),$(2)))
endef

# Install packages with DNF
# Usage: $(call PROJ_INSTALL_DNF,flags,src ...)
define PROJ_INSTALL_DNF
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_DNF,$(1),$(addsuffix .x86_64,$(2))))
endef

# Usage: $(call PROJ_INSTALL_DNF_NOARCH,flags,src ...)
define PROJ_INSTALL_DNF_NOARCH
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_DNF,$(1),$(2)))
endef

# Install DEB file(s)
# Usage: $(call PROJ_INSTALL_DEB,flags,src ...)
define PROJ_INSTALL_DEB
$(foreach deb,$(2),$(eval $(call PROJ_INSTALL_HELPER,INSTALL_DEB,$(1),$(deb),)))
endef

# Install tar.gz/tgz/... file(s)
# Usage: $(call PROJ_INSTALL_ARC,flags,src ...)
define PROJ_INSTALL_ARC
$(foreach tgz,$(2),$(eval $(call PROJ_INSTALL_HELPER,INSTALL_ARC,$(1),$(tgz),)))
endef

# Install kernel module(s) from a list of cpio regular expressions
# Usage: $(call PROJ_INSTALL_MODULES,flags,modules-file)
define PROJ_INSTALL_MODULES
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_MODULES,$(1),$(2),))
endef

# Install packages with PIP
# Usage: $(call PROJ_INSTALL_PIP,pkg ...)
define PROJ_INSTALL_PIP
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_PIP,$(1) -c $(PROJ_PIP_CONSTRAINT),$(2),))
endef

# Install packages with PIP without constraint file
# Usage: $(call PROJ_INSTALL_PIP_NC,pkg ...)
define PROJ_INSTALL_PIP_NC
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_PIP,$(1),$(2),))
endef

# Usage: $(call PROJ_NO_REINSTALL_PIP,pkg ...)
define PROJ_NO_REINSTALL_PIP
$(eval $(call PROJ_INSTALL_HELPER,INSTALL_PIP,$(1) -n -c $(PROJ_PIP_CONSTRAINT),$(2),))
endef

# Forward declarations
.PHONY: rootfs_install
rootfs_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: build_label_install
build_label_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: ppuiso_install
ppuiso_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: ppu_install
ppu_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: proj_initrd_install
proj_initrd_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: iso_boot_install
iso_boot_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: iso_ramdisk_install
iso_ramdisk_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: pxe_bundle_install
pxe_bundle_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: pxe_ramdisk_install
pxe_ramdisk_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: usb_boot_install
usb_boot_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: usb_ramdisk_install
usb_ramdisk_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: live_iso_boot_install
live_iso_boot_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: live_iso_ramdisk_install
live_iso_ramdisk_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: pxeserver_boot_install
pxeserver_boot_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: pxeserver_ramdisk_install
pxeserver_ramdisk_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: pxeserver_iso_boot_install
pxeserver_iso_boot_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: pxeserver_iso_ramdisk_install
pxeserver_iso_ramdisk_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: live_usb_boot_install
live_usb_boot_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: live_usb_ramdisk_install
live_usb_ramdisk_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: ova_boot_install
ova_boot_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: ova_ramdisk_install
ova_ramdisk_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: vagrant_boot_install
vagrant_boot_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi

.PHONY: vagrant_ramdisk_install
vagrant_ramdisk_install::
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi


# HEX SDK

ifndef TOP_SRCDIR
$(error Variable TOP_SRCDIR not set)
endif

ifndef TOP_BLDDIR
$(error Variable TOP_BLDDIR not set)
endif

ifndef HEX_MAKEDIR
$(error Variable HEX_MAKEDIR not set)
endif

ifeq ($(wildcard $(HEX_MAKEDIR)/hex_sdk.mk),)
$(error Variable HEX_MAKEDIR not set correctly)
endif

# Full (non-relative) paths to current build and source directories
BLDDIR := $(shell pwd)
ifneq ($(shell echo "$(BLDDIR)"|grep -e "/hex/"),)
SRCDIR := $(patsubst $(TOP_BLDDIR)%,$(TOP_DIR)%,$(BLDDIR))
else
SRCDIR := $(patsubst $(TOP_BLDDIR)%,$(TOP_SRCDIR)%,$(BLDDIR))
endif

# List of important variables to export to the shell
HEX_EXPORT_VARS := TOP_BLDDIR TOP_SRCDIR BLDDIR SRCDIR
HEX_EXPORT_VARS += HEX_ARCH HEX_VER

# HEX directories
# HEX_MAKEDIR is defined in build.mk by configure
HEX_TOPDIR      := $(TOP_DIR)/hex
HEX_SCRIPTSDIR  := $(HEX_TOPDIR)/scripts
HEX_DATADIR     := $(HEX_TOPDIR)/data
HEX_INCLUDEDIR  := $(HEX_TOPDIR)/include
HEX_EXPORT_VARS += HEX_TOPDIR HEX_MAKEDIR HEX_SCRIPTSDIR HEX_DATADIR HEX_INCLUDEDIR

HEX_SRCDIR     := $(TOP_SRCDIR)/hex
HEX_BLDDIR     := $(TOP_BLDDIR)/hex/src
HEX_LIBDIR     := $(TOP_BLDDIR)/hex/lib
HEX_BINDIR     := $(TOP_BLDDIR)/hex/bin
HEX_IMGDIR     := $(TOP_BLDDIR)/hex/img
HEX_MODDIR     := $(TOP_BLDDIR)/hex/src/modules
HEX_SHMODDIR   := $(TOP_SRCDIR)/hex/src/sdk_sh
HEX_DOCDIR     := $(TOP_BLDDIR)/hex/doc
HEX_EXPORT_VARS += HEX_SRCDIR HEX_BLDDIR HEX_LIBDIR HEX_BINDIR HEX_IMGDIR HEX_MODDIR HEX_SHMODDIR HEX_DOCDIR

# HEX packages
HEX_BOOTDIR       := /boot/hex
HEX_PKGDIR        := $(HEX_TOPDIR)/pkg
HEX_DISTDIR       := $(HEX_PKGDIR)/$(HEX_ARCH)/$(HEX_VER)
HEX_REPODIR       := $(HEX_DISTDIR)/$(HEX_DIST)
HEX_OSSDIR        := $(HEX_PKGDIR)/$(HEX_ARCH)/oss
HEX_THIRDPARTYDIR := $(HEX_PKGDIR)/$(HEX_ARCH)/thirdparty
HEX_MIRROR        := $(shell ip r | grep default | cut -d" " -f3)
HEX_PROXY_URL     := http://$(HEX_MIRROR):3128
HEX_COMPANY_DN    := www.bigstack.co
PXESERVER_IP      := 192.168.1.150
HEX_EXPORT_VARS += HEX_PKGDIR HEX_DISTDIR HEX_REPODIR HEX_OSSDIR HEX_THIRDPARTYDIR HEX_MIRROR HEX_PROXY_URL

# Link dynamically against SDK if compiled for release
# otherwise link statically to simplify testing
HEX_SDK_LIB_ARCHIVE := $(HEX_LIBDIR)/libhex_sdk.a
HEX_SDK_LIB_SO      := $(HEX_LIBDIR)/libhex_sdk.so
HEX_SDK_LDLIBS      := -lrt -lyaml -lglib-2.0 -lreadline -lcrypt -lcrypto -lssl -lpthread
ifeq ($(PRODUCTION),1)
HEX_SDK_LIB := $(HEX_SDK_LIB_SO)
else
HEX_SDK_LIB := $(HEX_SDK_LIB_ARCHIVE)
endif

# HEX component libraries
HEX_CONFIG_LIB     := $(HEX_LIBDIR)/libhex_config.a
HEX_TRANSLATE_LIB  := $(HEX_LIBDIR)/libhex_translate.a
HEX_CLI_LIB        := $(HEX_LIBDIR)/libhex_cli.a
HEX_FIRSTTIME_LIB  := $(HEX_LIBDIR)/libhex_firsttime.a

# HEX kernel
HEX_KERNEL                    := $(HEX_IMGDIR)/bzImage
HEX_EXPORT_VARS += HEX_KERNEL

# HEX firmware (UEFI x64)
HEX_FIRMWARE    := $(HEX_IMGDIR)/grubx64.efi

# HEX SDK Base OS
HEX_ROOTFS_CORE := $(HEX_BOOTDIR)/ubi9.tar.gz
HEX_MINI_INITRD := $(HEX_IMGDIR)/hex_mini_initramfs.cgz
HEX_BASE_ROOTFS := $(HEX_IMGDIR)/hex_base_rootfs.cgz
HEX_FULL_ROOTFS := $(HEX_IMGDIR)/hex_full_rootfs.cgz

# HEX installer ramdisk
HEX_INSTALL_RD := $(HEX_IMGDIR)/hex_install_initramfs.cgz

# HEX PXE server/installer ramdisk
HEX_PXE_RD := $(HEX_IMGDIR)/hex_pxe_initramfs.cgz
HEX_PXE_SERVER_RD := $(HEX_IMGDIR)/hex_pxe_server_initramfs.cgz
HEX_PXE_SERVER_ISO_RD := $(HEX_IMGDIR)/hex_pxe_server_iso_initramfs.cgz

TRIANGLE_TOPDIR      := $(TOP_DIR)/triangle
TRIANGLE_SCRIPTSDIR  := $(TRIANGLE_TOPDIR)/scripts
HEX_EXPORT_VARS += TRIANGLE_TOPDIR TRIANGLE_SCRIPTSDIR

# Forward declaration
.PHONY: help
help::
	@echo "Usage: make <target> ..."
	@echo "where <target> is one of:"

# NOTE: Help content should not exceed 80 columns. Please use the following lines as a guide.
#	@echo "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#	@echo "target       Description"


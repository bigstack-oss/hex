# HEX SDK

include ../../build.mk

# SDK static/shared library
LIB_SUBDIRS += hex_sdk_library
# This must always be listed last
LIB_SUBDIRS += sdk_so
SUBDIRS += $(LIB_SUBDIRS)

# SDK components (utilities/daemons)
SUBDIRS += hex_tuning
SUBDIRS += hex_translate
SUBDIRS += hex_config
SUBDIRS += hex_cli
SUBDIRS += hex_firsttime
SUBDIRS += hex_crashd
SUBDIRS += hex_banner

# Reusable modules for SDK components
SUBDIRS += modules

# USB/ISO/PXE Installer project
SUBDIRS += hex_install
SUBDIRS += hex_pxe_rd
SUBDIRS += hex_pxe_server_rd
SUBDIRS += hex_pxe_server_iso_rd

# Target to only build static/shared libraries
.PHONY: lib
lib:
	$(Q)$(MAKE) SUBDIRS="$(LIB_SUBDIRS)" all

include $(HEX_MAKEDIR)/hex_sdk.mk


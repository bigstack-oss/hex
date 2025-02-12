# HEX SDK

# Check if user accidentally tried to change a special variable after build.mk was included
define HEX_CHECK_SPECIAL_VAR
ifneq ($$($(1)),$$(_SAVED_$(1)))
$$(error $(1) cannot be changed after including 'build.mk')
endif
endef
$(foreach var,$(HEX_SPECIAL_VARS),$(eval $(call HEX_CHECK_SPECIAL_VAR,$(var))))

# Make sure we check that we're running as root inside the fakeroot before we build
.PHONY: _fakerootcheck
_fakerootcheck:
	@if [ $$EUID != 0 ]; then echo "Error: Must be run as root" >&2; exit 1; fi
	@if [ x$$(printenv DEVOPS_ENV) != 'x__JAIL__' ]; then echo "Error: Must be run in build jail" >&2 ; exit 1 ; fi

# Rerun configure if it or any of the makefiles have changed
.PHONY: _reconfigure
_reconfigure: _fakerootcheck $(TOP_BLDDIR)/reconfigure
	@true

$(TOP_BLDDIR)/reconfigure: $(HEX_TOPDIR)/configure $(shell find $(TOP_SRCDIR) -name Makefile -o -name \*.mk)
	$(Q)cd $(TOP_BLDDIR) && ./reconfigure

# Make sure these directories get created before we try to build
BUILD := $(HEX_LIBDIR) $(HEX_BINDIR) $(HEX_IMGDIR) $(HEX_CATALOGDIR) $(BUILD)
LD_LIBRARY_PATH := $(HEX_LIBDIR):$(LD_LIBRARY_PATH)
DISTCLEAN += $(HEX_LIBDIR) $(HEX_BINDIR) $(HEX_IMGDIR)
$(HEX_LIBDIR) $(HEX_BINDIR) $(HEX_IMGDIR) $(HEX_CATALOGDIR):
	$(call RUN_CMD,mkdir -p $@,"  MKDIR   $@")

# Do not pass SUBDIRS specified on the command line to sub-makes
#XSUBDIRS := $(subst $(SPACE),\ ,$(SUBDIRS))
#MAKEOVERRIDES := $(subst SUBDIRS=$(XSUBDIRS),,$(MAKEOVERRIDES))
unexport SUBDIRS

# Execute current target across all subdirectories
ifneq ($(SUBDIRS),)
.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(Q)$(MAKE) -C $@ $(TARGET)
endif

# Helper for implementing recursion
# $(1): target
ifneq ($(SUBDIRS),)
define RECURSE
$(Q)$(MAKE) TARGET=$(1) $(SUBDIRS)
endef
else
define RECURSE
@true
endef
endif

ifeq ($(USE_MEMCHECK),0)
DISABLE_MEMCHECK := 1
endif

ifeq ($(DISABLE_MEMCHECK),1)
ifeq ($(TESTRUNNER),$(MEMCHECK))
TESTRUNNER :=
endif
MEMCHECK :=
endif

# These must be listed in this order
include $(HEX_MAKEDIR)/hex_crashd.mk
include $(HEX_MAKEDIR)/hex_translate.mk
include $(HEX_MAKEDIR)/hex_cli.mk
include $(HEX_MAKEDIR)/hex_firsttime.mk
ifneq ($(PROJ_HEAVYFS_INSTALL),1)
include $(HEX_MAKEDIR)/hex_support.mk
include $(HEX_MAKEDIR)/hex_hwdetect.mk
include $(HEX_MAKEDIR)/hex_shell.mk
include $(HEX_MAKEDIR)/hex_banner.mk
endif
include $(HEX_MAKEDIR)/hex_config.mk


ifeq ($(PROJ_BUILD_USB),1)
PROJ_BUILD_ROOTFS := 1
PROJ_BUILD_PPU := 1
endif

ifeq ($(PROJ_BUILD_LIVE_USB),1)
PROJ_BUILD_ROOTFS := 1
endif

ifeq ($(PROJ_BUILD_ISO),1)
PROJ_BUILD_ROOTFS := 1
PROJ_BUILD_PPU := 1
endif

ifeq ($(PROJ_BUILD_LIVE_ISO),1)
PROJ_BUILD_ROOTFS := 1
endif

ifeq ($(PROJ_BUILD_OVA),1)
PROJ_BUILD_ROOTFS := 1
PROJ_BUILD_PPU := 1
endif

ifeq ($(PROJ_BUILD_PXESERVER),1)
PROJ_BUILD_ROOTFS := 1
PROJ_BUILD_PXE := 1
PROJ_BUILD_PPU := 1
endif

ifeq ($(PROJ_BUILD_PXESERVER_ISO),1)
PROJ_BUILD_ROOTFS := 1
PROJ_BUILD_PXE := 1
PROJ_BUILD_PPU := 1
endif

ifeq ($(PROJ_BUILD_PXEDEPLOY),1)
PROJ_BUILD_ROOTFS := 1
PROJ_BUILD_PXE := 1
PROJ_BUILD_PPU := 1
endif

ifeq ($(PROJ_BUILD_PXE),1)
PROJ_BUILD_ROOTFS := 1
PROJ_BUILD_PPU := 1
endif

ifeq ($(PROJ_BUILD_DIAG_USB),1)
PROJ_BUILD_DIAG_RD := 1
endif

ifeq ($(PROJ_BUILD_DIAG_PXE),1)
PROJ_BUILD_DIAG_RD := 1
endif

ifeq ($(PROJ_BUILD_ROOTFS),1)
include $(HEX_MAKEDIR)/projrootfs.mk
include $(HEX_MAKEDIR)/projinitramfs.mk
endif

ifeq ($(PROJ_BUILD_PPU),1)
include $(HEX_MAKEDIR)/projppu.mk
ALL += $(PROJ_PPU) $(PROJ_PPUISO)
endif

ifeq ($(PROJ_BUILD_TEST_HOTFIXES),1)
include $(HEX_MAKEDIR)/projhotfix.mk
FULL += $(PROJ_TEST_HOTFIXES)
endif

ifeq ($(PROJ_BUILD_USB),1)
include $(HEX_MAKEDIR)/projusb.mk
FULL += $(PROJ_USB)
endif

#ifeq ($(PROJ_BUILD_LIVE_USB),1)
#include $(HEX_MAKEDIR)/projliveusb.mk
#FULL += $(PROJ_LIVE_USB)
#endif

ifeq ($(PROJ_BUILD_ISO),1)
include $(HEX_MAKEDIR)/projiso.mk
FULL += $(PROJ_ISO)
endif

#ifeq ($(PROJ_BUILD_LIVE_ISO),1)
#include $(HEX_MAKEDIR)/projliveiso.mk
#FULL += $(PROJ_LIVE_ISO)
#endif
#
#ifeq ($(PROJ_BUILD_OVA),1)
#include $(HEX_MAKEDIR)/projova.mk
#FULL += $(PROJ_OVA)
#endif
#
#ifeq ($(PROJ_BUILD_VAGRANT),1)
#include $(HEX_MAKEDIR)/projvagrant.mk
#FULL += $(PROJ_VAGRANT)
#endif

ifeq ($(PROJ_BUILD_PXE),1)
include $(HEX_MAKEDIR)/projpxe.mk
FULL += $(PROJ_PXE)
endif

ifeq ($(PROJ_BUILD_PXEDEPLOY),1)
include $(HEX_MAKEDIR)/projpxedeploy.mk
endif

ifeq ($(PROJ_BUILD_PXESERVER),1)
include $(HEX_MAKEDIR)/projpxeserver.mk
FULL += $(PROJ_PXESERVER)
endif

ifeq ($(PROJ_BUILD_PXESERVER_ISO),1)
include $(HEX_MAKEDIR)/projpxeserveriso.mk
FULL += $(PROJ_PXESERVER_ISO)
endif

include $(HEX_MAKEDIR)/variable_targets.mk
include $(HEX_MAKEDIR)/fakeroot_targets.mk
include $(HEX_MAKEDIR)/compile_targets.mk
include $(HEX_MAKEDIR)/library_targets.mk
include $(HEX_MAKEDIR)/devtools_targets.mk
include $(HEX_MAKEDIR)/program_targets.mk
include $(HEX_MAKEDIR)/project_targets.mk
include $(HEX_MAKEDIR)/build_targets.mk
include $(HEX_MAKEDIR)/checkrootfs_targets.mk
include $(HEX_MAKEDIR)/test_targets.mk
include $(HEX_MAKEDIR)/test_extra_programs_targets.mk
include $(HEX_MAKEDIR)/clean_targets.mk
include $(HEX_MAKEDIR)/misc_targets.mk

# Dependencies
ifneq ($(DEP_FILES),)
-include $(DEP_FILES)
endif


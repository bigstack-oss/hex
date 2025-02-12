# HEX SDK

#
# Project Test Hotfixes Creation
#

help::
	@echo "testhotfixes Create project test hotfixes"

# $(1):	fixpack basename
define GEN_TEST_FIXPACK_TARGET
PROJ_TEST_HOTFIXES += $(PROJ_SHIPDIR)/$(1)
PKGCLEAN += $(PROJ_SHIPDIR)/$(1)

$(PROJ_SHIPDIR)/$(1): $(HEX_IMGDIR)/test_hotfixes/$(1)
	$$(Q)[ -d $$(PROJ_SHIPDIR) ] || mkdir -p $$(PROJ_SHIPDIR)
	$$(call RUN_CMD_TIMED,cp $$< $$@, "  CP      $(1)")
ifeq ($(wildcard $(HEX_HOTFIXES)/test_hotfixes/$(1).do_not_resign),)
	$$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/signfixpack $$@,"  SIGN    $(1)")
endif

endef # GEN_TEST_FIXPACK_TARGET

HEX_HOTFIXES := $(shell cd $(HEX_IMGDIR)/test_hotfixes >/dev/null 2>&1 && ls *.fixpack 2>/dev/null)
$(foreach fixpack,$(HEX_HOTFIXES),$(eval $(call GEN_TEST_FIXPACK_TARGET,$(fixpack))))
ifeq ($(DEBUG_MAKE),1)
$(foreach fixpack,$(HEX_HOTFIXES),$(info $(call GEN_TEST_FIXPACK_TARGET,$(fixpack))))
endif

.PHONY: testhotfixes
testhotfixes: $(PROJ_TEST_HOTFIXES)
	@true
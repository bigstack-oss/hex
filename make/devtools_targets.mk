# HEX SDK

#
# Development tools
# (non-production builds)
#

.PHONY: devtools_install
devtools_install:
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/hex_install/single.sh ./usr/sbin/single
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/hex_install/wipembr.sh ./usr/sbin/wipembr

ifeq ($(PRODUCTION),0)
LICENSE_NAME := development_trial
else
LICENSE_NAME := production_trial
endif

.PHONY: license_install
license_install:
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi
	@if [ ! -f $(PRIVATE_PEM) ]; then echo "Error: $(PRIVATE_PEM) is not found" >&2; exit 1; fi
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makelicense -t $(PRIVATE_PEM) $(PASSPHRASE) $(ROOTDIR)/var/support/$(LICENSE_NAME),"  GEN     $(LICENSE_NAME)")

.PHONY: license_generate
license_generate:
	@if [ ! -f $(PRIVATE_PEM) ]; then echo "Error: $(PRIVATE_PEM) is not found" >&2; exit 1; fi
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makelicense -t $(PRIVATE_PEM) $(PASSPHRASE) $(CORE_SHIPDIR)/$(LICENSE_NAME),"  GEN     $(LICENSE_NAME)")

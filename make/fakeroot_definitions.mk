# HEX SDK

FAKEROOT_DIR := /fakeroot-tools
FAKEROOT_TOOLS :=

# Install fakeroot tool(s) (deb, tgz, tar.gz, or gem)
# Usage: $(call FAKEROOT_INSTALL,marker,file ...,cmd {})
# Each occurrence of the string '{}' in the command is replaced with 'file ...'.
define FAKEROOT_INSTALL_HELPER
FAKEROOT_TOOLS += $$(FAKEROOT_DIR)/$(1)
$$(FAKEROOT_DIR)/$(1):
	$$(call RUN_CMD_SILENT,$(3),"  INSTALL $$@")
	$$(Q)touch $$@
endef

define FAKEROOT_INSTALL
$(eval $(call FAKEROOT_INSTALL_HELPER,$(1),$(2),$(subst {},$(2),$(3))))
endef

# Run fakeroot command
# Usage: $(call FAKEROOT_COMMAND,marker,cmd)
# File 'marker' is created dependent on source Makefile.
# If Makefile is newer than marker, then the command is re-run.
define FAKEROOT_COMMAND_HELPER
FAKEROOT_TOOLS += $$(FAKEROOT_DIR)/$(1)
$$(FAKEROOT_DIR)/$(1): $$(SRCDIR)/Makefile
	$$(call RUN_CMD_SILENT,$(2),"  INSTALL $$@")
	$$(Q)touch $$@
endef

define FAKEROOT_COMMAND
$(eval $(call FAKEROOT_COMMAND_HELPER,$(1),$(2)))
endef

define FAKEROOT_COMMAND_ALWAYS_HELPER
FAKEROOT_TOOLS += $$(FAKEROOT_DIR)/$(1)
$$(FAKEROOT_DIR)/$(1): $$(SRCDIR)/Makefile 
	$$(call RUN_CMD_SILENT,$(2),"  RUN ALWAYS $$@")
endef

define FAKEROOT_COMMAND_ALWAYS
$(eval $(call FAKEROOT_COMMAND_ALWAYS_HELPER,$(1),$(2)))
endef

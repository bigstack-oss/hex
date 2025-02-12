# HEX SDK
include ../../build.mk

FIXPACK_DIR := $(HEX_IMGDIR)/test_hotfixes

ROOT_SHELL_FIXPACK       := $(FIXPACK_DIR)/Diagnostic_Tools_Bigstack_Internal_Use_Only_dev.fixpack
NO_SIG_FIXPACK           := $(FIXPACK_DIR)/no_signature_dev.fixpack
SUCCESS_FIXPACK          := $(FIXPACK_DIR)/success_dev.fixpack
NOROLLBACK_FIXPACK       := $(FIXPACK_DIR)/no_rollback_dev.fixpack
FAILURE_FIXPACK          := $(FIXPACK_DIR)/failure_dev.fixpack
FSCK_FAILURE_FIXPACK     := $(FIXPACK_DIR)/force_fsck_failure_dev.fixpack
NEED_REBOOT_FIXPACK      := $(FIXPACK_DIR)/need_reboot_dev.fixpack
MISMATCH_VERSION_FIXPACK := $(FIXPACK_DIR)/mismatch_version_dev.fixpack

TEST_FIXPACKS := \
	$(FIXPACK_DIR) \
	$(ROOT_SHELL_FIXPACK) \
	$(NO_SIG_FIXPACK) \
	$(SUCCESS_FIXPACK) \
	$(NOROLLBACK_FIXPACK) \
	$(FAILURE_FIXPACK) \
	$(FSCK_FAILURE_FIXPACK) \
	$(NEED_REBOOT_FIXPACK) \
	$(MISMATCH_VERSION_FIXPACK)

FULL += $(TEST_FIXPACKS)
PKGCLEAN += $(TEST_FIXPACKS)

$(FIXPACK_DIR):
	$(Q)mkdir -p $@

$(ROOT_SHELL_FIXPACK): $(wildcard $(SRCDIR)/diagnostic_tools/*)
	$(call RUN_CMD,$(SHELL) $(HEX_SCRIPTSDIR)/makehotfix $(SRCDIR)/diagnostic_tools $@,"  GEN     $@")

$(NOROLLBACK_FIXPACK): $(wildcard $(SRCDIR)/no_rollback/*)
	$(call RUN_CMD,$(SHELL) $(HEX_SCRIPTSDIR)/makehotfix $(SRCDIR)/no_rollback $@,"  GEN     $@")

$(NO_SIG_FIXPACK): $(wildcard $(SRCDIR)/no_signature/*)
	$(call RUN_CMD,$(SHELL) $(HEX_SCRIPTSDIR)/makehotfix -n $(SRCDIR)/no_signature $@,"  GEN     $@")
	$(Q)touch $@.do_not_resign

$(SUCCESS_FIXPACK): $(wildcard $(SRCDIR)/success/*)
	$(call RUN_CMD,$(SHELL) $(HEX_SCRIPTSDIR)/makehotfix $(SRCDIR)/success $@,"  GEN     $@")

$(FAILURE_FIXPACK): $(wildcard $(SRCDIR)/failure/*)
	$(call RUN_CMD,$(SHELL) $(HEX_SCRIPTSDIR)/makehotfix $(SRCDIR)/failure $@,"  GEN     $@")

$(FSCK_FAILURE_FIXPACK): $(wildcard $(SRCDIR)/force_fsck_failure/*)
	$(call RUN_CMD,$(SHELL) $(HEX_SCRIPTSDIR)/makehotfix $(SRCDIR)/force_fsck_failure $@,"  GEN     $@")

$(NEED_REBOOT_FIXPACK): $(wildcard $(SRCDIR)/need_reboot/*)
	$(call RUN_CMD,$(SHELL) $(HEX_SCRIPTSDIR)/makehotfix $(SRCDIR)/need_reboot $@,"  GEN     $@")

$(MISMATCH_VERSION_FIXPACK): $(wildcard $(SRCDIR)/mismatch_version/*)
	$(call RUN_CMD,$(SHELL) $(HEX_SCRIPTSDIR)/makehotfix $(SRCDIR)/mismatch_version $@,"  GEN     $@")

include $(HEX_MAKEDIR)/hex_sdk.mk


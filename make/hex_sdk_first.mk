# HEX SDK

# All variable definitions must go in this file (or into a file included by this file), including
# multi-line variable/function definitions created with 'define'. This file can also contain some
# rules as long as they don't reference variables that a local Makefile might override.
#
# All other make rules (with and without recipes (dependency rules)) that reference these variables
# must go into one of the files included by 'hex_sdk.mk', including $(eval $(call ...)) constructs
# that autogenerate other variables or rules.

# Default make target
.PHONY: default
default: all
	@true

# Include project-specific definitions
ifneq ($(wildcard $(TOP_SRCDIR)/project.mk),)
include $(TOP_SRCDIR)/project.mk
endif

include $(HEX_MAKEDIR)/hex_sdk_definitions.mk
include $(HEX_MAKEDIR)/make_flag_definitions.mk
include $(HEX_MAKEDIR)/run_cmd_definitions.mk
include $(HEX_MAKEDIR)/variable_definitions.mk
include $(HEX_MAKEDIR)/kernel_definitions.mk
include $(HEX_MAKEDIR)/fakeroot_definitions.mk
include $(HEX_MAKEDIR)/library_definitions.mk
include $(HEX_MAKEDIR)/program_definitions.mk
include $(HEX_MAKEDIR)/build_definitions.mk
include $(HEX_MAKEDIR)/install_definitions.mk
include $(HEX_MAKEDIR)/project_definitions.mk
include $(HEX_MAKEDIR)/checkrootfs_definitions.mk
include $(HEX_MAKEDIR)/devtools_definitions.mk
include $(HEX_MAKEDIR)/test_definitions.mk
include $(HEX_MAKEDIR)/clean_definitions.mk

# The following variables are set by configure in build.mk.
# They can only be overridden by one of the following methods:
# 1) rerun configure with different flags
# 2) edit build.mk
# 3) set the variable in local Makefile before the inclusion of build.mk
# Options for debugging
HEX_SPECIAL_VARS += VERBOSE INTERACTIVE PRINT_DIRS DUMP_DEB_INFO
# Options of build types
HEX_SPECIAL_VARS += DEBUG PRODUCTION TRIAL QUIETBOOT PROFILE
# Options of system configs
HEX_SPECIAL_VARS += KERNEL_ARGS CONSOLE_SPEED
# Options of environment configs
HEX_SPECIAL_VARS += HEX_VER HEX_ARCH

# Save the values of these special variables so we can check later to see if they've been changed
define HEX_SAVE_SPECIAL_VAR
_SAVED_$(1) := $$($(1))
endef
$(foreach var,$(HEX_SPECIAL_VARS),$(eval $(call HEX_SAVE_SPECIAL_VAR,$(var))))


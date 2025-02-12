# HEX SDK

#
# Provconfig targets
#

ifneq ($(or $(findstring hex_config,$(PROGRAMS)),$(findstring hex_config,$(TESTS_EXTRA_PROGRAMS))),)

hex_config_LIBS += $(HEX_CONFIG_LIB) $(HEX_SDK_LIB)
hex_config_LDLIBS += $(HEX_SDK_LDLIBS) -lcrypto

hex_config_check: hex_config
	$(call RUN_CMD_SILENT,./hex_config --test,"  CHK     hex_config")
	$(Q)touch $@

BUILDCLEAN += hex_config_check

help::
	@echo "dump_tuning  Dump hex_config tuning parameters (non-recursive)"

.PHONY: dump_tuning
dump_tuning:
	$(Q)echo "hex_config tuning parameters:"
	$(Q)./hex_config --dump_tuning

help::
	@echo "dump_hex_config"
	@echo "             Dump hex_config commit and snapshot command order (non-recursive)"

.PHONY: dump_hex_config
dump_hex_config:
	$(Q)echo "hex_config commit order:"
	$(Q)./hex_config --dump
	$(Q)echo ; echo "hex_config snapshot command order:"
	$(Q)./hex_config --dump_snapshot

ifneq ($(findstring hex_config,$(PROGRAMS)),)

# Run hex_config in test mode to catch errors during static initialization
BUILD += hex_config_check

$(call PROJ_INSTALL_PROGRAM,-S,hex_config,./usr/sbin)

# utility to set testmode on/off
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_config/visettings.sh,./usr/sbin/visettings)

PROJ_BOOTSTRAP += $(HEX_DATADIR)/hex_config/bootstrap_hex_config

HEX_CONFIG_POST_DIR := /etc/hex_config/post.d

else # TESTS_EXTRA_PROGRAMS

TESTBUILD += hex_config_check

endif
endif # hex_config


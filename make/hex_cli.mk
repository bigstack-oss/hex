# HEX SDK

ifneq ($(or $(findstring hex_cli,$(PROGRAMS)),$(findstring hex_cli,$(TESTS_EXTRA_PROGRAMS))),)

hex_cli_LIBS += $(HEX_CLI_LIB) $(HEX_SDK_LIB)
hex_cli_LDLIBS += $(HEX_SDK_LDLIBS)

hex_cli_check: hex_cli
	$(call RUN_CMD_SILENT,./hex_cli --test,"  CHK     hex_cli")
	$(Q)touch $@

BUILDCLEAN += hex_cli_check

ifneq ($(findstring hex_cli,$(PROGRAMS)),)

# Run hex_cli in test mode to catch errors during static initialization
BUILD += hex_cli_check

help::
	@echo "dump_hex_cli"
	@echo "             Dump hex_cli available commands and usages"

.PHONY: dump_hex_cli
dump_hex_cli:
	$(Q)echo "hex_cli command list:"
	$(Q)./hex_cli --dump

$(call PROJ_INSTALL_PROGRAM,-S,hex_cli,./usr/sbin)

$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_cli/hex_cli_testmode.sh,./usr/sbin/hex_cli_testmode)

# Initialize /etc/motd used by hex_cli
rootfs_install::
	$(Q)if [ -n "$(PROJ_LONGNAME)" ]; then echo "Welcome to $(PROJ_LONGNAME)" > $(ROOTDIR)/etc/motd ; fi

else # TESTS_EXTRA_PROGRAMS

TESTBUILD += hex_cli_check

endif
endif # hex_cli


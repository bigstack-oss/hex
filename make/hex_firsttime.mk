# HEX SDK

#
# CLI-based first time setup wizard targets
#

ifneq ($(or $(findstring hex_firsttime,$(PROGRAMS)),$(findstring hex_firsttime,$(TESTS_EXTRA_PROGRAMS))),)

hex_firsttime_LIBS += $(HEX_FIRSTTIME_LIB) $(HEX_SDK_LIB)
hex_firsttime_LDLIBS += $(HEX_SDK_LDLIBS)

hex_firsttime_check: hex_firsttime
	$(call RUN_CMD_SILENT,./hex_firsttime --test,"  CHK     hex_firsttime")
	$(Q)touch $@

BUILDCLEAN += hex_firsttime_check

ifneq ($(findstring hex_firsttime,$(PROGRAMS)),)

# Run hex_firsttime in test mode to catch errors during static initialization
BUILD += hex_firsttime_check

$(call PROJ_INSTALL_PROGRAM,-S,hex_firsttime,./usr/sbin)

else # TESTS_EXTRA_PROGRAMS

TESTBUILD += hex_firsttime_check

endif
endif # hex_firsttime
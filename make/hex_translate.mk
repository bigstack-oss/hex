# HEX SDK

#
# Provtranslate targets
#

ifneq ($(or $(findstring hex_translate,$(PROGRAMS)),$(findstring hex_translate,$(TESTS_EXTRA_PROGRAMS))),)

hex_translate_LIBS += $(HEX_TRANSLATE_LIB) $(HEX_SDK_LIB)
hex_translate_LDLIBS += $(HEX_SDK_LDLIBS)

hex_translate_check: hex_translate
	$(call RUN_CMD_SILENT,./hex_translate --test,"  CHK     hex_translate")
	$(Q)touch $@

BUILDCLEAN += hex_translate_check

help::
	@echo "dump_hex_translate"
	@echo "             Dump hex_translate translate order (non-recursive)"

.PHONY: dump_hex_translate
dump_hex_translate:
	$(Q)echo "hex_translate translate order:"
	$(Q)./hex_translate --dump

ifneq ($(findstring hex_translate,$(PROGRAMS)),)

# Run hex_translate in test mode to catch errors during static initialization

BUILD += hex_translate_check

$(call PROJ_INSTALL_PROGRAM,,hex_translate,./usr/sbin)
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_translate/vipolicy.sh,./usr/sbin/vipolicy)

HEX_TRANSLATE_POST_DIR := /etc/hex_translate/post.d

else # TESTS_EXTRA_PROGRAMS

TESTBUILD += hex_translate_check

endif
endif # hex_translate


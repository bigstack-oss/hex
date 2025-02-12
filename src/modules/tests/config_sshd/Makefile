# HEX SDK

include ../../../../../build.mk

TESTS_EXTRA_PROGRAMS = hex_config

hex_config_MODULES = config_dummies.o config_sshd.o

CPPFLAGS += -I$(TOP_SRCDIR)/hex/src/modules/include

include $(HEX_MAKEDIR)/hex_sdk.mk

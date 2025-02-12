# HEX SDK

include ../../../build.mk

# space delimited list of sources to exclude
EXCLUDE_SRCS := cli_fixpack.cpp

LIB_SRCS := $(filter-out $(EXCLUDE_SRCS), $(shell cd $(SRCDIR) && ls *.c *.cpp 2>/dev/null))

SUBDIRS := tests

include $(HEX_MAKEDIR)/hex_sdk.mk


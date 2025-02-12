# HEX SDK

include ../../../../build.mk

TESTS_LIBS = $(HEX_CLI_LIB) $(HEX_SDK_LIB)
TESTS_LDLIBS = -lreadline 

test_conflict_01_EXTRA_SRCS = conflict_01_extra.cpp
test_conflict_03_EXTRA_SRCS = conflict_03_extra.cpp

include $(HEX_MAKEDIR)/hex_sdk.mk

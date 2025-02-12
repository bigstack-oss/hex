# HEX SDK

include ../../../../build.mk

TESTS_LIBS = $(HEX_CONFIG_LIB) $(HEX_SDK_LIB)
TESTS_LDLIBS = -lcrypto

TESTS_EXTRA_PROGRAMS = testopenfd
testopenfd_SRCS = testopenfd.c

test_command_02_EXTRA_SRCS = extra_command_02.cpp
test_module_03_EXTRA_SRCS = extra_module_03.cpp
test_trigger_03_EXTRA_SRCS = extra_trigger_03.cpp

include $(HEX_MAKEDIR)/hex_sdk.mk


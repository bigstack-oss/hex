# HEX SDK

include ../../../../../build.mk

TESTS_LIBS = $(HEX_SDK_LIB_ARCHIVE)
TESTS_LDLIBS = -ldl

TESTS_EXTRA_PROGRAMS = daemontest

daemontest_SRCS = daemontest.c
daemontest_LIBS = $(HEX_SDK_LIB_ARCHIVE)

CLEAN += test.out output

include $(HEX_MAKEDIR)/hex_sdk.mk
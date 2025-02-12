# HEX SDK

include ../../../../../build.mk

TESTS_LIBS = $(HEX_SDK_LIB_ARCHIVE)

TESTS_EXTRA_PROGRAMS := authtest
authtest_SRCS := authtest.c
authtest_LIBS = $(HEX_SDK_LIB)
authtest_LDLIBS = -lcrypt


include $(HEX_MAKEDIR)/hex_sdk.mk

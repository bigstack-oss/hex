# HEX SDK

include ../../../../../build.mk

TESTS_LIBS = $(HEX_SDK_LIB_ARCHIVE)
TESTS_EXTRA_PROGRAMS = testproc

testproc_SRCS = testproc.c
testproc_LIBS = $(HEX_SDK_LIB_ARCHIVE)

DISABLE_MEMCHECK = 1

include $(HEX_MAKEDIR)/hex_sdk.mk
# HEX SDK

include ../../../../build.mk

TESTS_EXTRA_PROGRAMS = dummy

dummy_SRCS = dummy.c
dummy_LIBS = $(HEX_SDK_LIB)
dummy_LDLIBS = -lrt

TESTS_LIBS = $(HEX_SDK_LIB)
TESTS_LDLIBS = -ldl -lrt

include $(HEX_MAKEDIR)/hex_sdk.mk


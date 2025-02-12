# HEX SDK

include ../../../build.mk

PROGRAMS = hex_tuning_helper

hex_tuning_helper_SRCS = hex_tuning_helper.c
hex_tuning_helper_LIBS = $(HEX_SDK_LIB)
hex_tuning_helper_LDLIBS = $(HEX_SDK_LDLIBS)

SUBDIRS := tests

include $(HEX_MAKEDIR)/hex_sdk.mk


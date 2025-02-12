# HEX SDK

include ../../../build.mk

SUBDIRS = tests

PROGRAMS = hex_crashd hex_crashinfo

hex_crashd_SRCS = crash_main.c
hex_crashd_LIBS = $(HEX_SDK_LIB)
hex_crashd_LDLIBS = $(HEX_SDK_LDLIBS)

hex_crashinfo_SRCS = ../hex_sdk_library/crash/tests/crashinfo.c
hex_crashinfo_LIBS = $(HEX_SDK_LIB)
hex_crashinfo_LDLIBS = $(HEX_SDK_LDLIBS)

include $(HEX_MAKEDIR)/hex_sdk.mk

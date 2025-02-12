# HEX SDK

include ../../../../../build.mk

TESTS_LIBS = $(HEX_SDK_LIB_ARCHIVE)

TESTS_EXTRA_PROGRAMS = send_cmd

send_cmd_SRCS = send_cmd.c
send_cmd_LIBS = $(HEX_SDK_LIB_ARCHIVE)
send_cmd_LDLIBS = -lrt

include $(HEX_MAKEDIR)/hex_sdk.mk

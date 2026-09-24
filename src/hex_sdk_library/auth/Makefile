# HEX SDK

include ../../../../build.mk

SUBDIRS = tests

LIB = $(HEX_SDK_LIB_ARCHIVE)

LIB_SRCS := \
	local_auth.c \
	auth.c

COMPILE_FOR_SHARED_LIB = 1

# checkauth and pamauth used to be built here; they are test drivers, and a program cannot
# be built in a directory that feeds the archive it links against. See tests/Makefile.

include $(HEX_MAKEDIR)/hex_sdk.mk

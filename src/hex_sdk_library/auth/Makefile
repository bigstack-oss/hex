# HEX SDK

include ../../../../build.mk

SUBDIRS = tests

LIB = $(HEX_SDK_LIB_ARCHIVE)

LIB_SRCS := \
	local_auth.c \
	auth.c

COMPILE_FOR_SHARED_LIB = 1

PROGRAMS := checkauth
checkauth_SRCS := checkauth.c
checkauth_LDLIBS = -lcrypt

PROGRAMS += pamauth
pamauth_SRCS := pamauth.c
pamauth_LDLIBS = -lpam

include $(HEX_MAKEDIR)/hex_sdk.mk

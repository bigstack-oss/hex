# HEX SDK

include ../../../build.mk

SUBDIRS += log
SUBDIRS += pidfile
SUBDIRS += process
SUBDIRS += crash
SUBDIRS += tuning
SUBDIRS += lock
SUBDIRS += parse
SUBDIRS += auth
SUBDIRS += cli_util
SUBDIRS += yml_util
SUBDIRS += strict
SUBDIRS += watchdog
SUBDIRS += filesystem
SUBDIRS += loop
SUBDIRS += queue
SUBDIRS += cmd
SUBDIRS += topten
SUBDIRS += table
SUBDIRS += logrotate
SUBDIRS += event_util
SUBDIRS += dryrun
SUBDIRS += crypto
SUBDIRS += license

# Unit tests for sources built directly into the SDK library (e.g. exec.cpp).
SUBDIRS += tests

LIB = $(HEX_SDK_LIB_ARCHIVE)

LIB_SRCS = exec.cpp

COMPILE_FOR_SHARED_LIB = 1

include $(HEX_MAKEDIR)/hex_sdk.mk

# HEX SDK

include ../../../../../build.mk

TESTS_EXTRA_PROGRAMS = testproc testthreads testdaemon daemontest

testproc_SRCS = testproc.c
testproc_LIBS = $(HEX_SDK_LIB)
testproc_LDLIBS = -lrt

testthreads_SRCS = testthreads.c
testthreads_LIBS = $(HEX_SDK_LIB)
testthreads_LDLIBS = -lrt -lpthread

# daemon for testing daemon library
testdaemon_SRCS = testdaemon.c
testdaemon_LIBS = $(HEX_SDK_LIB)

# daemon for testing daemon_control library
daemontest_SRCS = daemontest.c
daemontest_LIBS = $(HEX_SDK_LIB)

TESTS_LIBS = $(HEX_SDK_LIB)

CLEAN += test*.*

include $(HEX_MAKEDIR)/hex_sdk.mk



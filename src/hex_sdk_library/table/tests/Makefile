# HEX SDK

include ../../../../../build.mk

TESTS_LIBS = $(HEX_SDK_LIB_ARCHIVE)

TESTS_LDLIBS = -ldl -lrt -lpthread

test_init_failures_01_LDLIBS = $(TESTS_LDLIBS) -Wl,-wrap,shm_open 

include $(HEX_MAKEDIR)/hex_sdk.mk



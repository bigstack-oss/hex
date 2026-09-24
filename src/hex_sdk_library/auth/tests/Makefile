# HEX SDK

include ../../../../../build.mk

TESTS_LIBS = $(HEX_SDK_LIB_ARCHIVE)

# checkauth and pamauth are drivers for the HexAuth and PAM APIs -- never installed, only
# ever run by the scripts below. They are built here rather than in ../ because a program
# built in a directory that feeds libhex_sdk.a is never up to date: ../ is the 8th of 26
# subdirectories appending to the archive, so the 15 that follow always leave it newer than
# anything ../ linked, and every later make re-links them. Built with the test suite they
# also link a complete archive instead of whichever members happened to exist by then.
TESTS_EXTRA_PROGRAMS := authtest
authtest_SRCS := authtest.c
authtest_LIBS = $(HEX_SDK_LIB)
authtest_LDLIBS = -lcrypt

TESTS_EXTRA_PROGRAMS += checkauth
checkauth_SRCS := checkauth.c
checkauth_LIBS = $(HEX_SDK_LIB)
checkauth_LDLIBS = -lcrypt

TESTS_EXTRA_PROGRAMS += pamauth
pamauth_SRCS := pamauth.c
pamauth_LIBS = $(HEX_SDK_LIB)
pamauth_LDLIBS = -lpam


include $(HEX_MAKEDIR)/hex_sdk.mk

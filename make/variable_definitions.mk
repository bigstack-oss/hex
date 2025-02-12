# HEX SDK

BUILD      :=
TESTBUILD  :=
ALL        :=
TESTALL    :=
FULL       :=

LIB :=
LIB_SRCS :=

PROGRAMS :=

# If set to 1 memcheck is not used in this subdirectory
DISABLE_MEMCHECK := 0

# Valgind's memcheck
ifeq ($(GEN_MEMCHECK_SUPPRESSIONS),1)
MEMCHECK := $(SHELL) $(HEX_SCRIPTSDIR)/memcheck -s
else
MEMCHECK := $(SHELL) $(HEX_SCRIPTSDIR)/memcheck
endif

# Tool to run test scripts with, if any.  Default to valgrind's memcheck tool.
TESTRUNNER := $(MEMCHECK)

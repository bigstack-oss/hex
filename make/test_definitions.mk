# HEX SDK
#     Hex unit test makefile definitions

TEST_SRCS := $(shell cd $(SRCDIR) 2>/dev/null && ls test_*.cpp test_*.c bug_*.cpp bug_*.c 2>/dev/null)
TEST_OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(TEST_SRCS)))
DEP_FILES += $(patsubst %.cpp,%.d,$(patsubst %.c,%.d,$(TEST_SRCS) $(TEST_EXTRA_SRCS)))
TEST_SCRIPTS := $(patsubst %.xp,%,$(patsubst %.sh,%,$(shell cd $(SRCDIR) 2>/dev/null && ls test_*.sh bug_*.sh test_*.xp 2>/dev/null)))
TEST_PROGS := $(patsubst %.cpp,%,$(patsubst %.c,%,$(TEST_SRCS)))

ifeq ($(DEBUG_MAKE_TEST),1)
$(info TEST_SRCS=$(TEST_SRCS))
$(info TEST_SCRIPTS=$(TEST_SCRIPTS))
$(info TEST_PROGS=$(TEST_PROGS))
$(info TEST_CLASS=$(TEST_CLASS))
endif

# $(1): test program name
# $(2): make variable to hold list of test outputs (e.g. TESTS)
# $(3): file extension to use for test output file (e.g. test)
define GEN_TESTPROG_TARGETS
TESTBUILD += $(1)
BUILDCLEAN += $(1)
$(1).$(3): $(1) $(ALL) $(TESTS_EXTRA_PROGRAMS)
$(2) += $(1).$(3)

# Add dependency on shell script if it exists
ifneq ($$(wildcard $(SRCDIR)/$(1).sh),)
$(1).$(3): $(SRCDIR)/$(1).sh
endif

# Add dependency on expect script if it exists
ifneq ($$(wildcard $(SRCDIR)/$(1).xp),)
$(1).$(3): $(SRCDIR)/$(1).xp
endif

# Force separate compilation/linking to workaround dependency bug
$(1): $(1).o

$(1)_SRCS = $(notdir $(wildcard $(SRCDIR)/$(1).c) $(wildcard $(SRCDIR)/$(1).cpp))

# add extra test source
ifneq ($$($(1)_EXTRA_SRCS),)
$(1): $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$($(1)_EXTRA_SRCS)))
$(1)_SRCS += $($(1)_EXTRA_SRCS)
DEP_FILES += $(patsubst %.cpp,%.d,$(patsubst %.c,%.d,$($(1)_EXTRA_SRCS)))
else
ifneq ($(TESTS_EXTRA_SRCS),)
$(1): $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(TESTS_EXTRA_SRCS)))
$(1)_SRCS += $(TESTS_EXTRA_SRCS)
DEP_FILES += $(patsubst %.cpp,%.d,$(patsubst %.c,%.d,$(TESTS_EXTRA_SRCS)))
endif
endif

ifneq ($$(shell echo "$($(1)_LIBS)"|grep -e " -l"),)
$$(error $(1)_LIBS contains -l flags, move these to $(1)_LDLIBS)
endif
ifneq ($$(shell echo "$($(1)_LIBS)"|grep -e " -l"),)
$$(error TESTS_LIBS contains -l flags, move these to TESTS_LDLIBS)
endif

ifneq ($$($(1)_LIBS),)
$(1): $$($(1)_LIBS)
else
ifneq ($(TESTS_LIBS),)
$(1): $(TESTS_LIBS)
$(1)_LIBS += $(TESTS_LIBS)
else
ifneq ($(LIB),)
$(1): $(LIB)
endif
endif
endif

ifneq ($$($(1)_LDLIBS),)
$(1): LDLIBS = $$($(1)_LDLIBS)
else
ifneq ($(TESTS_LDLIBS),)
$(1): LDLIBS = $(TESTS_LDLIBS)
endif
endif

ifneq ($$($(1)_CXXFLAGS),)
$(1): CXXFLAGS += $$($(1)_CXXFLAGS)
endif

ifneq ($$($(1)_LDCMD),)
$(1): LDCMD = $$($(1)_LDCMD)
else
ifneq ($$(TESTS_LDCMD),)
$(1): LDCMD = $(TESTS_LDCMD)
else
ifneq ($$(filter %.cpp,$$($(1)_SRCS)),)
$(1): LDCMD = $(CXX)
endif
endif

# If linking against sdk lib, need CXX
ifneq ($$(filter $$(HEX_SDK_LIB),$$($(1)_LIBS)),)
$(1): LDCMD = $(CXX)
endif

endif

endef # GEN_TESTPROG_TARGETS

# $(1): test script name
# $(2): make variable to hold list of test outputs (e.g. TESTS, VTESTS, NTESTS)
# $(3): file extension to use for test output file (e.g. tst, vtst, ntst)
define GEN_TESTSCRIPT_TARGETS

# Add scripts to list if no source file have the same basename
# (those are covered by GEN_TESTPROG_TARGETS)
ifeq ($(strip $(notdir $$(wildcard $(SRCDIR)/$(1).c $(SRCDIR)/$(1).cpp))),)
TESTBUILD += $(1)
BUILDCLEAN += $(1)
$(1).$(3): $(1) $(ALL) $(PROGRAMS) $(TESTS_EXTRA_PROGRAMS) $(wildcard $(SRCDIR)/testprep.sh) $(wildcard $(SRCDIR)/testpost.sh)
$(2) += $(1).$(3)
endif

HEX_EXPORT_VARS += TEST_HDD

endef # GEN_TESTSCRIPT_TARGETS

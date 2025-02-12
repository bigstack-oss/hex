# HEX SDK
#     unit test targets

.SUFFIXES: .test

# Do not use RUN_CMD_TIMED
# runtests computes/displays elapsed time so that it can be used inside vtest without needing make
%.test: %
	$(Q)export $(PROJ_TEST_EXPORTS); export TESTRUNNER="$(TESTRUNNER)"; $(SHELL) $(HEX_SCRIPTSDIR)/runtests $(VERBOSE_FLAG) $(INTERACTIVE_FLAG) $@

$(foreach testscript,$(TEST_SCRIPTS),$(eval $(call GEN_TESTSCRIPT_TARGETS,$(testscript),TESTS,test)))
ifeq ($(DEBUG_MAKE_TEST),1)
$(foreach testscript,$(TEST_SCRIPTS),$(info $(call GEN_TESTSCRIPT_TARGETS,$(testscript),TESTS,test)))
endif

$(foreach testprog,$(TEST_PROGS),$(eval $(call GEN_TESTPROG_TARGETS,$(testprog),TESTS,test)))
ifeq ($(DEBUG_MAKE_TEST),1)
$(foreach testprog,$(TEST_PROGS),$(info $(call GEN_TESTPROG_TARGETS,$(testprog),TESTS,test)))
endif

$(foreach testclass,$(TEST_CLASS),$(eval $(call GEN_TESTCLASS_TARGETS,$(testclass),TESTS,test)))
ifeq ($(DEBUG_MAKE_TEST),1)
$(foreach testclass,$(TEST_CLASS),$(info $(call GEN_TESTCLASS_TARGETS,$(testclass),TESTS,test)))
endif

# Sort tests in name order
TESTS := $(sort $(TESTS))

# Allow disabling tests
TESTS := $(filter-out $(EXCLUDE_TESTS), $(TESTS))

ifeq ($(DEBUG_MAKE_TEST),1)
$(info TESTS=$(TESTS))
endif

# All tests should be dependent on possible project output files in the same directory
ifneq ($(TESTS),)
$(TESTS): $(TEST_DEPS)
endif

#
# Test targets
#

help::
	@echo "testall      Run all tests and check that code coverage meets target"
	@echo "             Same as \"covclean testbuild test vtest ntest itest rtest covcheck\""
	@echo "             NOTE: Must run \"make all\" first from top directory"
	@echo "             NOTE: Only reruns tests that require it (e.g. failed or"
	@echo "                   dependency changes)"
	@echo "                   Use \"make testclean testall\" to force rerunning"
	@echo "                   of all tests"

.PHONY: testall
testall:: _reconfigure _testall_r
	@true

# Recursive-helper to avoid running reconfigure/covclean more than once
# Execute all test stages and then recurse into subdirectories 
# Coverage check not cummulative and only includes tests for each individual subdirectory
_testall_r:: _testbuild _test
	$(call RECURSE,_testall_r)

help::
	@echo "test         Run all unit tests (implies all/testbuild)"

.PHONY: test
test:: _test_r
ifeq ($(THDD_PTH),)
	@true
else
	@mount | grep tmpfs | grep -o "$(THDD_PTH:/=)" | xargs -i umount {} || true
endif

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _test_r
_test_r:: _test
	$(call RECURSE,_test_r)

# Non-recursive
.PHONY: _test
_test:: _all _testbuild $(TESTALL) $(TESTS)
	@true

# Non-recursive
.PHONY: testil
testil: $(shell echo $(TESTS) | grep -P -o ".*test_$(NUM).*? ")
	@true

# showtests
help::
	@echo "showtests    Print out all unit tests"

.PHONY: showtests
showtests:: _reconfigure _showtests_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _showtests_r
_showtests_r:: _showtests
	$(call RECURSE,_showtests_r)

# Non-recursive
.PHONY: _showtests
_showtests::
	@echo $(BLDDIR): $(TESTS)

#
# Test configuration
#

help::
	@echo "testvars"
	@echo "               INTERACTIVE (I)              stop after first test failure"
	@echo "               GEN_MEMCHECK_SUPPRESSIONS    generate valgrind suppressed file (.supp)"
	@echo "               DISABLE_MEMCHECK             disable memcheck"
	@echo "               [TEST].disable_testrunner    touch this file to disable testrunner for [TEST]"

# HEX SDK
#     Unit test extra program targets

# Programs to assist with testing (coverage not enforced)
# NOTE: These are not the same as TEST_PROGS, which is an auto generated list
$(foreach prog,$(TESTS_EXTRA_PROGRAMS),$(eval $(call GEN_PROGRAM_TARGETS,$(prog),0)))
ifeq ($(DEBUG_MAKE_TEST),1)
$(foreach prog,$(TESTS_EXTRA_PROGRAMS),$(info $(call GEN_PROGRAM_TARGETS,$(prog),0)))
endif

TESTBUILD += $(TESTS_EXTRA_PROGRAMS)
BUILDCLEAN += $(TESTBUILD)


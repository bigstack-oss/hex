# HEX SDK

#
# Program targets
#

BUILD += $(PROGRAMS)
BUILDCLEAN += $(PROGRAMS)

# HEX binaries are symlinked to the HEX bin dir for distribution
# Project binaries are not
ifneq ($(DISABLE_SYMLINK_HEX_BIN),1)
SYMLINK_HEX_BIN = $(Q)[ $2 -eq 0 ] || ln -sf `pwd`/$1 $(HEX_BINDIR)/$1
else
SYMLINK_HEX_BIN := @true
endif

# $(1): program name
# $(2): 1=enable coverage checking, 0=disable coverage checking
define GEN_PROGRAM_TARGETS

ifneq ($$(shell echo "$(1)"|grep -e "^test_\|^bug_"),)
$$(error PROGRAMS/TESTS_EXTRA_PROGRAMS cannot begin with "test_" or "bug_" as these prefixes are reserved for unit tests)
endif

ifneq ($$(shell echo " $($(1)_LIBS)"|grep -e " -l"),)
$$(info $(1)_LIBS should contain a white-space separated list of pathnames to libraries to link against)
$$(info $(1)_LDLIBS should contain a white-space separated list of linker flags (-l) specifying system libraries to link against)
$$(error $(1)_LIBS contains -l flags, move these to $(1)_LDLIBS)
endif

$(1)_OBJS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$($(1)_SRCS)))
DEP_FILES += $(patsubst %.cpp,%.d,$(patsubst %.c,%.d,$($(1)_SRCS)))

ifneq ($$($(1)_LIBS),)
$(1): $$($(1)_OBJS) $$($(1)_MODULES) $$($(1)_LIBS)
	$$(call RUN_CMD,$$(LDCMD) $$(LDFLAGS) $$(TARGET_ARCH) $$^ $$(LIBS) $$(LDLIBS) -o $$@,"  LD      $$@")
	$(call SYMLINK_HEX_BIN,$(1),$(2))
else
$(1): $$($(1)_OBJS) $$($(1)_MODULES) $(LIB)
	$$(call RUN_CMD,$$(LDCMD) $$(LDFLAGS) $$(TARGET_ARCH) $$^ $$(LIBS) $$(LDLIBS) -o $$@,"  LD      $$@")
	$(call SYMLINK_HEX_BIN,$(1),$(2))
endif

ifneq ($$($(1)_LDLIBS),)
$(1): LDLIBS = $$($(1)_LDLIBS)
endif

ifneq ($$($(1)_LDCMD),)
$(1): LDCMD = $$($(1)_LDCMD)
else
ifneq ($$(filter %.cpp,$$($(1)_SRCS)),)
$(1): LDCMD = $(CXX)
endif

# If linking against sdk lib, need CXX
ifneq ($$(filter $$(HEX_SDK_LIB),$$($(1)_LIBS)),)
$(1): LDCMD = $(CXX)
endif
endif

ifeq ($(HEX_INSIDE_SRCDIR),1)
ifneq ($(DISABLE_SYMLINK_HEX_BIN),1)
BUILDCLEAN += $(HEX_BINDIR)/$(1)
endif
endif

endef # GEN_PROGRAM_TARGETS

# Programs to be redistributed (coverage enforced)
$(foreach prog,$(PROGRAMS),$(eval $(call GEN_PROGRAM_TARGETS,$(prog),1)))
ifeq ($(DEBUG_MAKE),1)
$(foreach prog,$(PROGRAMS),$(info $(call GEN_PROGRAM_TARGETS,$(prog),1)))
endif


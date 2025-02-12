# HEX SDK

#
# Library targets
#

# HEX: Don't remove libhex_sdk.a during make clean in each subdirectory
ifeq ($(LIB),$(HEX_SDK_LIB_ARCHIVE))
DISABLE_CLEAN_LIB := 1
endif

# Automatically detect if we're building a shared library
ifneq ($(LIB),)
ifeq ($(suffix $(LIB)),.so)
COMPILE_FOR_SHARED_LIB := 1
MAKE_SHARED_LIB := 1
endif
endif

ifneq ($(LIB_SRCS),)
LIB_OBJS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(LIB_SRCS)))
BUILD += $(LIB_OBJS)

# If COMPILE_FOR_SHARED_LIB is 1 then include compilation flags for building shared object
ifeq ($(COMPILE_FOR_SHARED_LIB),1)
$(LIB_OBJS): OPTFLAGS += -fno-omit-frame-pointer -fPIC
endif

DEP_FILES += $(patsubst %.cpp,%.d,$(patsubst %.c,%.d,$(LIB_SRCS)))

ifneq ($(LIB),)
BUILD += $(LIB)
ifneq ($(DISABLE_CLEAN_LIB),1)
BUILDCLEAN += $(LIB)
endif
ifneq ($(MAKE_SHARED_LIB),1)
# Don't clean lib here since its built across multiple directories
# Create rule to add library objects to archive
# Some versions of make will choke if LIB contains trailing whitespace so we must strip it
$(LIB): $(strip $(LIB))($(LIB_OBJS))
endif
endif # LIB
endif # LIB_SRCS

#
# Shared library targets
#

# If MAKE_SHARED_LIB is 1, then we want to build a shared object from the object files
ifeq ($(MAKE_SHARED_LIB),1)
ifneq ($(LIB),)
SONAME = $(shell basename $(LIB))
BUILDCLEAN += $(LIB)

# Create rule to build shared library from object files
$(LIB): $(LIB_OBJS)
	$(call RUN_CMD,$(LDCMD) $(LDFLAGS) -shared -Wl$(COMMA)-soname$(COMMA)$(SONAME) $^ $(LIBS) $(LDLIBS) -o $@,"  LD      $@")
endif # LIB
endif # MAKE_SHARED_LIB

# If MAKE_SHARED_LIB_FROM_ARCHIVE is 1 then create shared object from archive
# This should always be done in a separate directory since it removes object files
ifeq ($(MAKE_SHARED_LIB_FROM_ARCHIVE),1)
ifneq ($(LIB),)
LIB_SO = $(patsubst %.a,%.so,$(LIB))
SONAME = $(shell basename $(LIB_SO))
BUILD += $(LIB_SO)
BUILDCLEAN += $(LIB_SO) $(LIB)

# Create rule to build shared library from archive
# Use the following to override linker:
# $(LIB_SO): LDCMD = $(CXX)
$(LIB_SO): $(LIB)
	$(call RUN_CMD,OBJS=`$(AR) t $(LIB)`; $(AR) x $(LIB) && $(LDCMD) -shared -Wl$(COMMA)-soname$(COMMA)$(SONAME) $$OBJS $(LIBS) $(LDLIBS) -o $@ && $(RM) $$OBJS,"  LD      $@")
endif # LIB
endif # MAKE_SHARED_LIB_FROM_ARCHIVE


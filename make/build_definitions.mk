# HEX SDK

# Hex SDK header dependencies
HEX_SDK_INCDIR := -I/usr/lib64/glib-2.0/include -I/usr/include/glib-2.0

# GCC Toolset 15, installed in the build jail beside the system gcc 11 (which still builds kernel
# modules, rpms and pip wheels); gnu23 needs gcc 14 or newer. Make predefines CC and CXX, so `?=`
# never fired: replace only make's defaults, and let the environment or command line still win.
GCC_TOOLSET_BINDIR := /opt/rh/gcc-toolset-15/root/usr/bin
ifneq ($(filter default undefined,$(origin CC)),)
CC       := $(GCC_TOOLSET_BINDIR)/gcc
endif
ifneq ($(filter default undefined,$(origin CXX)),)
CXX      := $(GCC_TOOLSET_BINDIR)/g++
endif
#FIXME: deprecated declarations should be replaced
#WARNFLAGS := -Wall -Werror -Wno-unused-result
WARNFLAGS := -Wall -Werror -Wno-unused-result -Wno-error=deprecated-declarations
OPTFLAGS := -g
CFLAGS    = $(WARNFLAGS) $(OPTFLAGS) -std=gnu23
CXXFLAGS  = $(WARNFLAGS) $(OPTFLAGS) -fno-rtti -fexceptions -std=gnu++23
CPPFLAGS := -MMD -D_REENTRANT $(HEX_SDK_INCDIR) -I$(HEX_INCLUDEDIR) -I$(SRCDIR) -I.

LDFLAGS	 :=
ifeq ($(DEBUG),0)
OPTFLAGS += -O2 -DNDEBUG
endif

ifeq ($(TRIAL),1)
OPTFLAGS += -DTRIAL_BUILD
endif

ifeq ($(PROFILE),1)
OPTFLAGS += -pg -finstrument-functions
LDFLAGS  += -pg -L$(TOP_BLDDIR)/tools/yagprof 
# note that we do not link with yagmon automatically, components
# that wish to use yagprof must add -lyagmon to the end of their _LDLIBS
# variable (must be at the end)
endif
AR       := ar
ARFLAGS  := rU
STRIP    := strip

# Include target-specific toolchain info
ifneq ($(wildcard $(HEX_MAKEDIR)/build_defs_$(HEX_ARCH).mk),)
include $(HEX_MAKEDIR)/build_defs_$(HEX_ARCH).mk
endif

ifneq ($(CCACHE),)
CC  := $(CCACHE) $(CC)
CXX := $(CCACHE) $(CXX)
endif

# Override gmake's default link cmd so we can override the compiler used for linking only
LDCMD  := $(CC)
LINK.o = $(LDCMD) $(LDFLAGS)

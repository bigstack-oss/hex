# HEX SDK

# Hex SDK header dependencies
HEX_SDK_INCDIR := -I/usr/lib64/glib-2.0/include -I/usr/include/glib-2.0

CC       ?= gcc
CXX      ?= g++
#FIXME: deprecated declarations should be replaced
#WARNFLAGS := -Wall -Werror -Wno-unused-result
WARNFLAGS := -Wall -Werror -Wno-unused-result -Wno-error=deprecated-declarations
OPTFLAGS := -g
CFLAGS    = $(WARNFLAGS) $(OPTFLAGS) -std=gnu99
CXXFLAGS  = $(WARNFLAGS) $(OPTFLAGS) -fno-rtti -fexceptions -std=gnu++2a
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

# HEX SDK

# Convenient abbreviations
ifdef V
VERBOSE := $(V)
endif

ifdef I
INTERACTIVE := $(I)
endif

ifdef D
PRINT_DIRS := $(D)
endif

ifdef P
PRODUCTION := $(P)
endif

ifdef T
TRIAL := $(T)
endif

ifdef QB
QUIETBOOT := $(QB)
endif

# INTERACTIVE implies VERBOSE
ifneq ($(INTERACTIVE),0)
VERBOSE := 1
endif

# Verbose implies print directories
ifneq ($(VERBOSE),0)
PRINT_DIRS := 1
endif

# Production implies quietboot
ifneq ($(PRODUCTION),0)
DEBUG := 0
QUIETBOOT := 1
endif


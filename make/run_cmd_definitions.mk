# HEX SDK

# Some targets/scripts require Bash instead of Bourne shell to work correctly
SHELL := /bin/bash

# Useful definitions
COMMA := ,
NULLSTRING :=
SPACE := $(NULLSTRING) #end of line

# V	Controls level of verbosity from make builds.  1=verbose, 0=non-verbose (brief).
#
# Q	Used to hide commands in non-verbose mode that do not normally need to be shown,
#	but might be needed in verbose mode to debug makefiles.
#
#	Example:
#
#	foo:
#		${Q}touch foo
#
# RUN_CMD
#	Used to hide commands in non-verbose mode, but show brief description.
#	Stdout and stderr from command still leak through even if commands succeeds (e.g. gcc warnings, etc.).
#
#	Example:
#
#	%.o: %.c
#		$(call RUN_CMD,$(COMPILE.c) $(OUTPUT_OPTION) $<,"  CC      $@")
#
# RUN_CMD_SILENT
#	Used to hide commands in non-verbose mode, but show brief description.
#	Stdout and stderr from command are hidden unless command exits non-zero.
#	RUN_CMD is preferred (simpler/faster), but this can be used to wrap scripts that
#	cannot be made quiet enough.
#
#	Example:
#
#	initramfs.cgz:
#		$(call RUN_CMD_SILENT,$(SHELL) $(HEX_SCRIPTSDIR)/makeinitramfs $(TOP_SRCDIR) $(HEX_IMGDIR),"  GEN     $@")
#
# RUN_CMD_SHOULD_FAIL
#	Same as RUN_CMD_SILENT except that command is expected to fail (return non-zero).
#	Used for exercising the test framework.
#
ifeq ($(VERBOSE),0)
Q := @
QEND := >/dev/null 2>&1
RUN_CMD = @[ -z $2 ] || echo $2; $1
RUN_CMD_SILENT =      @[ -z $2 ] || echo $2; TMPFILE=`mktemp /tmp/tmp.XXXXXX`; STATUS=0; if ( set -e ; $1 ) >$$TMPFILE 2>&1; then true; else cat $$TMPFILE; STATUS=1; fi; rm -f $$TMPFILE; exit $$STATUS
RUN_CMD_SHOULD_FAIL = @[ -z $2 ] || echo $2; TMPFILE=`mktemp /tmp/tmp.XXXXXX`; STATUS=0; if ( set -e ; $1 ) >$$TMPFILE 2>&1; then cat $$TMPFILE; STATUS=1; else true; fi; rm -f $$TMPFILE; exit $$STATUS
RUN_CMD_TIMED =       @[ -z $2 ] || echo -n $2; TMPFILE=`mktemp /tmp/tmp.XXXXXX`; BEFORE=`date +%s`; STATUS=0; if ( set -e ; $1 ) >$$TMPFILE 2>&1; then AFTER=`date +%s`; echo " (`expr $$AFTER - $$BEFORE`s)"; else echo; cat $$TMPFILE; STATUS=1; fi; rm -f $$TMPFILE; exit $$STATUS
else
Q :=
QEND :=
RUN_CMD = $1
RUN_CMD_SILENT = $1
RUN_CMD_SHOULD_FAIL = ( set -e ; $1 ) && exit 1 || exit 0
RUN_CMD_TIMED = $1
endif

# Verbose flag to pass to HEX SDK scripts
ifeq ($(VERBOSE),3)
# Enable script debugging and save temp files
HEX_DEBUG="trace nocleanup"
export HEX_DEBUG
VERBOSE_FLAG := -v
else
ifeq ($(VERBOSE),2)
# Enable script tracing
HEX_DEBUG=trace
export HEX_DEBUG
VERBOSE_FLAG := -v
else
ifeq ($(VERBOSE),1)
VERBOSE_FLAG := -v
else
VERBOSE_FLAG :=
endif
endif
endif

# If non-zero, export so that install deb will dump deb info
ifeq ($(DUMP_DEB_INFO),1)
export DUMP_DEB_INFO
endif

# Don't display messages about directory changes in non-verbose mode
# Set D=1 on command line to show directories without enabling verbose mode
ifeq ($(PRINT_DIRS),0)
MAKEFLAGS += --no-print-directory
endif

# Make command to use in scripts
# Don't ever print directories when running submake in same directory
MAKECMD = $(MAKE) VERBOSE=$(VERBOSE) --no-print-directory

# Quiet flag for makebootimg
# QUIET_FLAG is used by scripts and QUIET_KERNEL_ARG is used by 'sed' for *.in files 
ifeq ($(QUIETBOOT),0)
QUIET_FLAG :=
QUIET_KERNEL_ARG :=
else
QUIET_FLAG := -q
QUIET_KERNEL_ARG := quiet
endif

# Interactive flag to pass to HEX SDK scripts
ifeq ($(INTERACTIVE),0)
INTERACTIVE_FLAG :=
else
INTERACTIVE_FLAG := -i
endif


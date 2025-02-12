# HEX SDK

# Custom install program that generates build report
# Always strip since we're using gdbserver, only local copy needs symbols
INSTALL_PROGRAM := STRIP=$(STRIP) $(SHELL) $(HEX_SCRIPTSDIR)/install -s -t prog
INSTALL_SO      := STRIP=$(STRIP) $(SHELL) $(HEX_SCRIPTSDIR)/install -s -t so
INSTALL_LKM     := STRIP=$(STRIP) $(SHELL) $(HEX_SCRIPTSDIR)/install -s -t lkm
INSTALL_SCRIPT  := $(SHELL) $(HEX_SCRIPTSDIR)/install -t script
INSTALL_DATA    := $(SHELL) $(HEX_SCRIPTSDIR)/install
INSTALL_RPM     := $(SHELL) $(HEX_SCRIPTSDIR)/installrpm
INSTALL_ARC     := $(SHELL) $(HEX_SCRIPTSDIR)/installarc
INSTALL_MODULES := $(SHELL) $(HEX_SCRIPTSDIR)/installmodules
INSTALL_FIRMWARE := $(SHELL) $(HEX_SCRIPTSDIR)/installfirmware
INSTALL_PIP     := $(SHELL) $(HEX_SCRIPTSDIR)/installpip

ifeq ($(WEAK_DEP),0)
INSTALL_DNF     := $(SHELL) $(HEX_SCRIPTSDIR)/installdnf
else
INSTALL_DNF     := $(SHELL) $(HEX_SCRIPTSDIR)/installdnf -w
endif

LOCKED_RPMS     := $(ROOTDIR)/locked_rpms.txt
BLKLST_RPMS     := $(ROOTDIR)/blklst_rpms.txt

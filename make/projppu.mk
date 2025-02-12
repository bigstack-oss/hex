# HEX SDK

#
# Project Package Update (a.k.a. PPU or ".pkg" file) creation
#

# HEX and Product-specific installation script to be run during hex_install
INSTALL_SCRIPTS ?= $(wildcard $(HEX_DATADIR)/hex_install/install.sh $(SRCDIR)/install.sh)
POSTINSTALL_SCRIPTS ?= $(wildcard $(HEX_DATADIR)/hex_install/postinstall.sh $(SRCDIR)/postinstall.sh)

UPDATE_SCRIPT ?= $(wildcard $(HEX_DATADIR)/hex_install/update.sh $(SRCDIR)/update.sh)
POSTUPDATE_SCRIPTS ?= $(wildcard $(HEX_DATADIR)/hex_install/postupdate.sh $(SRCDIR)/postupdate.sh)

PROJ_VERSION_UNDERSCORE = $(shell echo $(PROJ_VERSION) | sed 's/\./_/g')
PROJ_PPU_DATE = $(shell echo $(PROJ_BUILD_LABEL) | sed 's/-.*//g' | sed 's%\(....\)\(..\)\(..\)%\2/\3/\1%')
ifeq ($(DEBUG_MAKE),1)
$(info PROJ_VERSION_UNDERSCORE=$(PROJ_VERSION_UNDERSCORE))
$(info PROJ_PPU_DATE=$(PROJ_PPU_DATE))
endif

help::
	@echo "ppu          Create project ppu (.pkg) (non-recursive)"

PKGCLEAN += $(PROJ_PPU) $(PROJ_ROOTFS_MD5) $(PROJ_ROOTFS_COMMIT) $(PROJ_PPUISO)

.PHONY: ppu
ppu: $(PROJ_PPU)
	@true

$(PROJ_PPU): $(PROJ_KERNEL) $(PROJ_INITRD) $(PROJ_ROOTFS)
	$(Q)$(MAKECMD) PROJ_PPU_LONGNAME=$$(readlink $(PROJ_RELEASE)).pkg ppu_build

.PHONY: ppu_build
ppu_build::
	$(Q)[ -d $(PROJ_SHIPDIR) ] || mkdir -p $(PROJ_SHIPDIR)
	$(Q)$(RM) $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*.pkg $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*.pkg.md5 $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*_rootfs.md5
	$(Q)$(RM) $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*_debug.cgz $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*.commit
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makeppu -p $(PROJ_PPU_PADDING) -c '$(MAKECMD) ROOTDIR=@ROOTDIR@ ppu_install' $(PROJ_KERNEL) $(PROJ_INITRD) $(PROJ_ROOTFS) $(PROJ_FIRMWARE) $(PROJ_SHIPDIR)/$(PROJ_PPU_LONGNAME),"  GEN     $(PROJ_PPU_LONGNAME)")
	$(Q)[ ! -e $(PROJ_ROOTFS_DEBUG) ] || cp $(PROJ_ROOTFS_DEBUG) $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_debug.cgz
	$(Q)ln -sf $(PROJ_SHIPDIR)/$(PROJ_PPU_LONGNAME) $(PROJ_PPU)
	$(Q)ln -sf $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_rootfs.md5 $(PROJ_ROOTFS_MD5)
	$(Q)md5sum < $(PROJ_PPU) > $(PROJ_SHIPDIR)/$(PROJ_PPU_LONGNAME).md5
	$(Q)chmod 0644 $(PROJ_PPU) $(PROJ_SHIPDIR)/$(PROJ_PPU_LONGNAME).md5
	$(Q)for PKG in $(PROJ_SHIPDIR)/$$(basename $(PROJ_PPU_LONGNAME) .pkg)_*.pkg ; do md5sum < $$PKG > $$PKG.md5 ; chmod 0644 $$PKG $$PKG.md5; done
	$(Q)ln -sf $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE)).commit $(PROJ_ROOTFS_COMMIT)
	$(Q)echo $(PROJ_BUILD_COMMIT) > $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE)).commit

# Install additional files into PPU
ppu_install::
	$(Q)readlink $(PROJ_RELEASE) > $(ROOTDIR)/release
	$(Q)echo $(PROJ_BUILD_COMMIT) > $(ROOTDIR)/commit

# Copy install scripts to PPU package
ifneq ($(INSTALL_SCRIPTS),)
ppu_install::
	$(Q)cat $(INSTALL_SCRIPTS) >> $(ROOTDIR)/install.sh

$(PROJ_PPU): $(INSTALL_SCRIPTS)
endif

# Copy post install scripts to PPU package
ifneq ($(POSTINSTALL_SCRIPTS),)
ppu_install::
	$(Q)cat $(POSTINSTALL_SCRIPTS) >> $(ROOTDIR)/postinstall.sh

$(PROJ_PPU): $(POSTINSTALL_SCRIPTS)
endif

# Copy update scripts to PPU package
ifneq ($(UPDATE_SCRIPT),)
ppu_install::
	$(Q)cat $(UPDATE_SCRIPT) >> $(ROOTDIR)/update.sh

$(PROJ_PPU): $(UPDATE_SCRIPT)
endif

# Copy post update scripts to PPU package
ifneq ($(POSTUPDATE_SCRIPTS),)
ppu_install::
	$(Q)cat $(POSTUPDATE_SCRIPTS) >> $(ROOTDIR)/postupdate.sh

$(PROJ_PPU): $(POSTUPDATE_SCRIPTS)
endif

.PHONY: ppuiso
ppuiso: $(PROJ_PPUISO)
	@true

$(PROJ_PPUISO): $(PROJ_PPU)
	$(Q)$(MAKECMD) PROJ_PPUISO_LONGNAME=$$(readlink $(PROJ_RELEASE))_pkg.iso ppuiso_build

.PHONY: ppuiso_build
ppuiso_build::
	$(Q)[ -d $(PROJ_SHIPDIR) ] || mkdir -p $(PROJ_SHIPDIR)
	$(Q)$(RM) $(PROJ_SHIPDIR)/$(PROJ_NAME)*$(PROJ_BUILD_DESC)_pkg.iso*
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makedataimg -p $(PROJ_PPUISO_PADDING) -b $(PROJ_PPU) -c '$(MAKECMD) ROOTDIR=@ROOTDIR@ ppuiso_install' iso $(PROJ_SHIPDIR)/$(PROJ_PPUISO_LONGNAME),"  GEN     $(PROJ_PPUISO_LONGNAME)")
	$(Q)ln -sf $(PROJ_SHIPDIR)/$(PROJ_PPUISO_LONGNAME) $(PROJ_PPUISO)
	$(Q)md5sum < $(PROJ_PPUISO) > $(PROJ_SHIPDIR)/$(PROJ_PPUISO_LONGNAME).md5

ppuiso_install::
	$(Q)cp $(PROJ_SHIPDIR)/$(PROJ_NAME)*$(PROJ_BUILD_DESC)_*.pkg.md5 $(ROOTDIR)/
	$(Q)sync

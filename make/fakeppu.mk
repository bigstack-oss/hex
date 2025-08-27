# HEX SDK

#
# Fake Package Update (a.k.a. ".pkg" file) creation
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
	$(Q)echo "fake.pkg     Create fake ppu (.pkg) (non-recursive)"

PKGCLEAN += $(FAKE_PPU) $(FAKE_PPU).md5 $(FAKE_PPU).sha256

$(FAKE_PPU): $(PROJ_KERNEL) $(PROJ_INITRD) $(PROJ_ROOTFS)
	$(Q)$(MAKECMD) FAKE_PPU_LONGNAME=$$(echo $$(readlink $(PROJ_RELEASE)) | sed 's/CUBE/FAKE/').pkg fake_ppu_build

.PHONY: fake_ppu_build
fake_ppu_build::
	$(Q)[ -d $(PROJ_SHIPDIR) ] || mkdir -p $(PROJ_SHIPDIR)
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makeppu -p $(PROJ_PPU_PADDING) -c '$(MAKECMD) ROOTDIR=@ROOTDIR@ fake_ppu_install' $(PROJ_KERNEL) $(PROJ_INITRD) $(HEX_IMGDIR)/hex_base_rootfs.cgz $(PROJ_FIRMWARE) $(PROJ_SHIPDIR)/$(FAKE_PPU_LONGNAME),"  GEN     $(FAKE_PPU_LONGNAME)")
	$(Q)ln -sf $(PROJ_SHIPDIR)/$(FAKE_PPU_LONGNAME) $(FAKE_PPU)
	$(Q)chmod 0644 $(FAKE_PPU)
	$(Q)nohup bash -c "md5sum < $(FAKE_PPU) > $(PROJ_SHIPDIR)/$(FAKE_PPU_LONGNAME).md5 && chmod 0644 $(PROJ_SHIPDIR)/$(FAKE_PPU_LONGNAME).md5" >/dev/null 2>&1 &
	$(Q)nohup bash -c "sha256sum < $(FAKE_PPU) > $(PROJ_SHIPDIR)/$(FAKE_PPU_LONGNAME).sha256 && chmod 0644 $(PROJ_SHIPDIR)/$(FAKE_PPU_LONGNAME).sha256" >/dev/null 2>&1 &
	$(Q)for PKG in $(PROJ_SHIPDIR)/$$(basename $(FAKE_PPU_LONGNAME) .pkg)_*.pkg ; do chmod 0644 $$PKG ; done
	$(Q)nohup bash -c "for PKG in $(PROJ_SHIPDIR)/$$(basename $(FAKE_PPU_LONGNAME) .pkg)_*.pkg ; do md5sum < $$PKG > $$PKG.md5 && chmod 0644 $$PKG.md5 ; sha256sum < $$PKG > $$PKG.sha256 && chmod 0644 $$PKG.sha256 ; done" >/dev/null 2>&1 &

# Install additional files into PPU
fake_ppu_install::
	$(Q)readlink $(PROJ_RELEASE) > $(ROOTDIR)/release
	$(Q)echo $(PROJ_BUILD_COMMIT) > $(ROOTDIR)/commit

# Copy install scripts to PPU package
ifneq ($(INSTALL_SCRIPTS),)
fake_ppu_install::
	$(Q)cat $(INSTALL_SCRIPTS) >> $(ROOTDIR)/install.sh

$(PROJ_PPU): $(INSTALL_SCRIPTS)
endif

# Copy post install scripts to PPU package
ifneq ($(POSTINSTALL_SCRIPTS),)
fake_ppu_install::
	$(Q)cat $(POSTINSTALL_SCRIPTS) >> $(ROOTDIR)/postinstall.sh

$(PROJ_PPU): $(POSTINSTALL_SCRIPTS)
endif

# Copy update scripts to PPU package
ifneq ($(UPDATE_SCRIPT),)
fake_ppu_install::
	$(Q)cat $(UPDATE_SCRIPT) >> $(ROOTDIR)/update.sh

$(PROJ_PPU): $(UPDATE_SCRIPT)
endif

# Copy post update scripts to PPU package
ifneq ($(POSTUPDATE_SCRIPTS),)
fake_ppu_install::
	$(Q)cat $(POSTUPDATE_SCRIPTS) >> $(ROOTDIR)/postupdate.sh

$(PROJ_PPU): $(POSTUPDATE_SCRIPTS)
endif

# HEX SDK

#
# Project installable PXE image creation
#

help::
	@echo "pxe          Create project pxe installer bundle (non-recursive)"

PROJ_RELEASE_LONGNAME := $(shell readlink $(PROJ_RELEASE))
PKGCLEAN += $(PROJ_PXE)
TEST_DEPS += $(PROJ_PXE)

.PHONY: pxe
pxe: $(PROJ_PXE)
	@true

# Create PXE bundle
# Derive PPU and PXE filenames from helper symlink
$(PROJ_PXE): $(PROJ_KERNEL) $(PROJ_PPU) $(PROJ_PXE_RD)
	$(Q)$(MAKECMD) PROJ_PPU_LONGNAME=$$(readlink $(PROJ_RELEASE)).pkg PROJ_PXE_LONGNAME=$$(readlink $(PROJ_RELEASE)).pxe.tgz pxe_build

.PHONY: pxe_build
pxe_build:
	$(Q)[ -d $(PROJ_SHIPDIR) ] || mkdir -p $(PROJ_SHIPDIR)
	$(Q)$(RM) $(PROJ_SHIPDIR)/$(PROJ_NAME)*$(PROJ_BUILD_DESC).pxe.tgz*
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makepxebundle $(QUIET_FLAG) -S $(CONSOLE_SPEED) -c '$(MAKECMD) ROOTDIR=@ROOTDIR@ pxe_bundle_install' $(PROJ_NAME) $(PROJ_KERNEL) $(PROJ_PXE_RD) $(PROJ_SHIPDIR)/$(PROJ_PPU_LONGNAME) $(PROJ_SHIPDIR)/$(PROJ_PXE_LONGNAME),"  GEN     $(PROJ_PXE_LONGNAME)")
	$(Q)ln -sf $(PROJ_SHIPDIR)/$(PROJ_PXE_LONGNAME) $(PROJ_PXE)
	$(Q)nohup md5sum < $(PROJ_PXE) > $(PROJ_SHIPDIR)/$(PROJ_PXE_LONGNAME).md5 2>&1 &
	$(Q)nohup sha256sum < $(PROJ_PXE) > $(PROJ_SHIPDIR)/$(PROJ_PXE_LONGNAME).sha256 2>&1 &

$(PROJ_PXE_RD): $(HEX_PXE_RD) $(HEX_HWDETECT_FILES) $(PROJ_PPU) $(HEX_DATADIR)/hex_install/hex_pxe_install.sh.in $(HEX_DATADIR)/hex_install/hex_autoinstall.sh.in $(HEX_DATADIR)/hex_install/hex_pxe_fetch.sh.in
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/mountinitramfs '$(MAKECMD) PPU=$$(readlink $(PROJ_RELEASE)).pkg ROOTDIR=@ROOTDIR@ pxe_ramdisk_install' $< $@,"  GEN     $@")

pxe_ramdisk_install::
	$(Q)echo "sys.install.mode = pxe" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "if [ -d /sys/firmware/efi ]; then /usr/bin/hostname uefi-installer; else /usr/bin/hostname bios-installer; fi" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)sed -e 's/@IMAGE_NAME@/$(PROJ_RELEASE_LONGNAME)\*.pkg/' $(HEX_DATADIR)/hex_install/hex_pxe_install.sh.in > $(ROOTDIR)/usr/sbin/hex_pxe_install
	$(Q)chmod 755 $(ROOTDIR)/usr/sbin/hex_pxe_install
	$(Q)sed -e 's|@HEX_AGENT_ENV_DIR@|$(HEX_AGENT_ENV_DIR)|g' -e 's|@HEX_INSTALL_DATA_LABEL_PREFIX@|$(HEX_INSTALL_DATA_LABEL_PREFIX)|g' -e 's|@HEX_INSTALL_SKIP_TRANSPORTS@|$(HEX_INSTALL_SKIP_TRANSPORTS)|g' $(HEX_DATADIR)/hex_install/hex_autoinstall.sh.in > $(ROOTDIR)/usr/sbin/hex_autoinstall
	$(Q)chmod 755 $(ROOTDIR)/usr/sbin/hex_autoinstall
	@# Ship the preflight agent into the installer so hex_autoinstall can run
	@# --preflight before restore (agent binary provided by the build).
	$(Q)if [ -f $(TOP_BLDDIR)/core/phone-home-agent/phone-home-agent ]; then \
		cp -f $(TOP_BLDDIR)/core/phone-home-agent/phone-home-agent $(ROOTDIR)/usr/sbin/phone-home-agent && \
		chmod 755 $(ROOTDIR)/usr/sbin/phone-home-agent ; \
	else echo "  WARN    phone-home-agent not built; installer preflight disabled" ; fi
	$(Q)chroot $(ROOTDIR) bash -c "rm -f /etc/systemd/system/NetworkManager.service"
	$(Q)chroot $(ROOTDIR) bash -c "systemctl enable NetworkManager"
	@# Ship the image-fetch + autoinstall logic as a standalone script; rc.local
	@# only invokes it. @TOKEN@ placeholders are filled at build time.
	$(Q)sed -e 's|@HEX_COMPANY_DN@|$(HEX_COMPANY_DN)|g' -e 's|@PXESERVER_IP@|$(PXESERVER_IP)|g' -e 's|@PROJ_RELEASE_LONGNAME@|$(PROJ_RELEASE_LONGNAME)|g' $(HEX_DATADIR)/hex_install/hex_pxe_fetch.sh.in > $(ROOTDIR)/usr/sbin/hex_pxe_fetch
	$(Q)chmod 755 $(ROOTDIR)/usr/sbin/hex_pxe_fetch
	$(Q)echo "/usr/sbin/hex_pxe_fetch" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)chroot $(ROOTDIR) bash -c "chmod 755 /etc/rc.d/rc.local"

# Install project build label into installer image
pxe_ramdisk_install:: build_label_install

# Install hardware detection and kernel modules into pxe image
pxe_ramdisk_install:: hex_hwdetect_install

# Install welcome messages into installer image
pxe_ramdisk_install::
	$(Q)if [ -n "$(PROJ_LONGNAME)" ]; then echo "Welcome to $(PROJ_LONGNAME) Installer" > $(ROOTDIR)/etc/motd ; fi


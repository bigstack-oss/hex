# HEX SDK

#
# Project installable ISO image creation
#

help::
	@echo "iso          Create project iso installer image (non-recursive)"

PKGCLEAN += $(PROJ_ISO)

# Non-recursive
.PHONY: iso
iso: $(PROJ_ISO)
	@true

# Create bootable ISO image
# Derive PPU and ISO filenames from helper symlink
$(PROJ_ISO): $(PROJ_KERNEL) $(PROJ_ISO_RD) $(PROJ_PPU)
	$(Q)$(MAKECMD) PROJ_PPU=$$(readlink $(PROJ_PPU)) PROJ_ISO_LONGNAME=$$(readlink $(PROJ_RELEASE)).iso iso_build

.PHONY: iso_build
iso_build:
	$(Q)[ -d $(PROJ_SHIPDIR) ] || mkdir -p $(PROJ_SHIPDIR)
	$(Q)$(RM) $(PROJ_SHIPDIR)/$(PROJ_NAME)*.iso $(PROJ_SHIPDIR)/$(PROJ_NAME)*.iso.md5
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makebootimg -S $(CONSOLE_SPEED) -k "$(KERNEL_ARGS) $(ISO_KERNEL_ARGS) nicdetect_disable" $(QUIET_FLAG) -p $(PROJ_ISO_PADDING) -b $(PROJ_PPU) -c '$(MAKECMD) ROOTDIR=@ROOTDIR@ iso_boot_install' iso $(PROJ_KERNEL) $(PROJ_ISO_RD) $(PROJ_SHIPDIR)/$(PROJ_ISO_LONGNAME),"  GEN     $(PROJ_ISO_LONGNAME)")
	$(Q)ln -sf $(PROJ_SHIPDIR)/$(PROJ_ISO_LONGNAME) $(PROJ_ISO)
	$(Q)md5sum < $(PROJ_ISO) > $(PROJ_SHIPDIR)/$(PROJ_ISO_LONGNAME).md5

$(PROJ_ISO_RD): $(HEX_INSTALL_RD) $(HEX_HWDETECT_FILES) $(HEX_DATADIR)/os/rc.mount.sh
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/mountinitramfs '$(MAKECMD) ROOTDIR=@ROOTDIR@ iso_ramdisk_install' $< $@,"  GEN     $@")

iso_ramdisk_install::
	$(Q)echo "if [ -d /sys/firmware/efi ]; then /usr/bin/hostname uefi-installer; else /usr/bin/hostname bios-installer; fi" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo "sys.live.mode = iso" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.install.mode = iso" >> $(ROOTDIR)/etc/settings.sys
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/rc.mount.sh ./etc/rc.mount

# Install project build label into installer image
iso_ramdisk_install:: build_label_install

# Install hardware detection and kernel modules into installer image
iso_ramdisk_install:: hex_hwdetect_install

# Install welcome messages into installer image
iso_ramdisk_install:: 
	$(Q)if [ -n "$(PROJ_LONGNAME)" ]; then echo "Welcome to $(PROJ_LONGNAME) Installer" > $(ROOTDIR)/etc/motd ; fi


# HEX SDK

#
# Project installable USB image creation
#

help::
	@echo "usb          Create project usb installer image (non-recursive)"

PKGCLEAN += $(PROJ_USB)

.PHONY: usb
usb: $(PROJ_USB)
	@true

# Create bootable USB image
# Derive PPU and USB filenames from helper symlink
$(PROJ_USB): $(PROJ_KERNEL) $(PROJ_PPU) $(PROJ_USB_RD)
	$(Q)$(MAKECMD) PROJ_PPU=$$(readlink $(PROJ_PPU)) PROJ_USB_LONGNAME=$$(readlink $(PROJ_RELEASE)).img usb_build

.PHONY: usb_build
usb_build:
	$(Q)[ -d $(PROJ_SHIPDIR) ] || mkdir -p $(PROJ_SHIPDIR)
	$(Q)$(RM) $(PROJ_SHIPDIR)/$(PROJ_NAME)*$(PROJ_BUILD_DESC).img*
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makebootimg -S $(CONSOLE_SPEED) -k "$(KERNEL_ARGS) nicdetect_disable" $(QUIET_FLAG) -p $(PROJ_USB_PADDING) -b $(PROJ_PPU) -c '$(MAKECMD) ROOTDIR=@ROOTDIR@ usb_boot_install' usb $(PROJ_KERNEL) $(PROJ_USB_RD) $(PROJ_SHIPDIR)/$(PROJ_USB_LONGNAME),"  GEN     $(PROJ_USB_LONGNAME)")
	$(Q)ln -sf $(PROJ_SHIPDIR)/$(PROJ_USB_LONGNAME) $(PROJ_USB)
	$(Q)md5sum < $(PROJ_USB) > $(PROJ_SHIPDIR)/$(PROJ_USB_LONGNAME).md5

$(PROJ_USB_RD): $(HEX_INSTALL_RD) $(HEX_HWDETECT_FILES) $(HEX_DATADIR)/os/rc.mount.sh
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/mountinitramfs '$(MAKECMD) ROOTDIR=@ROOTDIR@ usb_ramdisk_install' $< $@,"  GEN     $@")

usb_ramdisk_install::
	$(Q)echo "if [ -d /sys/firmware/efi ]; then /usr/bin/hostname uefi-installer; else /usr/bin/hostname bios-installer; fi" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo "sys.live.mode = usb" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.install.mode = usb" >> $(ROOTDIR)/etc/settings.sys
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/rc.mount.sh ./etc/rc.mount

# Install project build label into installer image
usb_ramdisk_install:: build_label_install

# Install hardware detection and kernel modules into installer image
usb_ramdisk_install:: hex_hwdetect_install

# Install welcome messages into installer image
usb_ramdisk_install::
	$(Q)if [ -n "$(PROJ_LONGNAME)" ]; then echo "Welcome to $(PROJ_LONGNAME) Installer" > $(ROOTDIR)/etc/motd ; fi


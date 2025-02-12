# HEX SDK

#
# Project PXE Server USB image creation
#

help::
	@echo "pxeserver    Create project pxe server usb image (non-recursive)"

PKGCLEAN += $(PROJ_PXESERVER)
TEST_DEPS += $(PROJ_PXESERVER)
FTPBOOT_PATH := var/ftpboot

.PHONY: pxeserver
pxeserver: $(PROJ_PXESERVER)
	@true

# Create bootable USB image
# Derive USB filename from helper symlink
$(PROJ_PXESERVER): $(PROJ_KERNEL) $(PROJ_PPU) $(PROJ_PXESERVER_RD)
	$(Q)$(MAKECMD) PROJ_PPU=$$(readlink $(PROJ_PPU)) PROJ_PXESERVER_LONGNAME=$$(readlink $(PROJ_RELEASE))_pxeserver.img pxeserver_build

.PHONY: pxeserver_build
pxeserver_build:
	$(Q)[ -d $(PROJ_SHIPDIR) ] || mkdir -p $(PROJ_SHIPDIR)
	$(Q)$(RM) $(PROJ_SHIPDIR)/$(PROJ_NAME)*$(PROJ_BUILD_DESC)_pxeserver.img*
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makebootimg -S $(CONSOLE_SPEED) -k "$(PROJ_KERNEL_ARGS) $(KERNEL_ARGS)" $(QUIET_FLAG) -p $(PROJ_PXESERVER_PADDING) -b $(PROJ_PPU) -c '$(MAKECMD) ROOTDIR=@ROOTDIR@ pxeserver_boot_install' usb $(PROJ_KERNEL) $(PROJ_PXESERVER_RD) $(PROJ_SHIPDIR)/$(PROJ_PXESERVER_LONGNAME),"  GEN     $(PROJ_PXESERVER_LONGNAME)")
	$(Q)ln -sf $(PROJ_SHIPDIR)/$(PROJ_PXESERVER_LONGNAME) $(PROJ_PXESERVER)
	$(Q)md5sum < $(PROJ_PXESERVER) > $(PROJ_SHIPDIR)/$(PROJ_PXESERVER_LONGNAME).md5

$(PROJ_PXESERVER_RD): $(HEX_PXE_SERVER_RD) $(PROJ_PXE)
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/mountinitramfs '$(MAKECMD) ROOTDIR=@ROOTDIR@ pxeserver_ramdisk_install' $< $@,"  GEN     $@")

pxeserver_ramdisk_install::
	$(Q)echo "if [ -d /sys/firmware/efi ]; then /usr/bin/hostname uefi-pxeserver; else /usr/bin/hostname bios-pxeserver; fi" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo "sys.live.mode = usb" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.install.mode = pxe" >> $(ROOTDIR)/etc/settings.sys
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/rc.mount.sh ./etc/rc.mount
	$(Q)mkdir -p $(ROOTDIR)/$(FTPBOOT_PATH)/pxelinux.cfg
	$(Q)tar -I pigz -xf $(PROJ_PXE) -C $(ROOTDIR)/$(FTPBOOT_PATH)/ --exclude=*.pkg
	$(Q)mv $(ROOTDIR)/$(FTPBOOT_PATH)/*_default $(ROOTDIR)/$(FTPBOOT_PATH)/pxelinux.cfg/default
	@# Enable keep (auto) baud, but since default comes from PROJ_PXE, just sed it
	$(Q)sed -i -e '/^serial.*/Id' $(ROOTDIR)/$(FTPBOOT_PATH)/pxelinux.cfg/default
	$(Q)sed -i -e '1 i serial 0 0 0x003' $(ROOTDIR)/$(FTPBOOT_PATH)/pxelinux.cfg/default
	@#the ftp-boot path need to sync with dnsmasq config
	$(Q)cp $(HEX_FIRMWARE) $(ROOTDIR)/$(FTPBOOT_PATH)/
	$(Q)chmod 755 $(ROOTDIR)/$(FTPBOOT_PATH)/grubx64.efi

# Copy postinstall script to /etc/rc.postinstall
# PROJ_POSTINSTALL_SCRIPTS is defined in projxpu.mk
ifneq ($(PROJ_POSTINSTALL_SCRIPTS),)
pxeserver_ramdisk_install::
	$(Q)cat $(PROJ_POSTINSTALL_SCRIPTS) >> $(ROOTDIR)/etc/rc.postinstall
	$(Q)chmod 755 $(ROOTDIR)/etc/rc.postinstall

$(PROJ_PXESERVER_RD): $(PROJ_POSTINSTALL_SCRIPTS)

endif

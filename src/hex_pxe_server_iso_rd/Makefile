# HEX SDK

include ../../../build.mk

# pxe server ramdisk used by projects
ALL = $(HEX_PXE_SERVER_ISO_RD)

# Delete _debug and .size files
PKGCLEAN += $(patsubst %.cgz,%*,$(HEX_PXE_SERVER_ISO_RD)) RPMS

PROJ_NAME = hex_pxe_server
PROJ_BUILD_ROOTFS = 1

PROJ_HARDWARE = qemu vmware generic

# Use base rootfs
PROJ_BASE_ROOTFS = $(HEX_PXE_SERVER_RD)

# Install directly to final ramdisk image
PROJ_ROOTFS = $(HEX_PXE_SERVER_ISO_RD)

FTPBOOT_PATH := var/ftpboot

rootfs_install::
	$(Q)echo 'mkdir -p /mnt/iso' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '( while true; do' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '    if ls /$(FTPBOOT_PATH)/firmware/*.pkg >/dev/null 2>&1 ; then MD5_VERIFIED=true ; else MD5_VERIFIED=false ; fi' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '    for PKG in `ls /$(FTPBOOT_PATH)/firmware/*.pkg 2>/dev/null` ; do' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo "        EXPECT_MD5=\$$(< \$${PKG}.md5 awk '{print \$$1}')" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo "        ACTUAL_MD5=\$$(md5sum \$$PKG | awk '{print \$$1}')" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '        if [ "$$EXPECT_MD5" = "$$ACTUAL_MD5" ]; then' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '            echo "Successfully verified $$PKG md5 checksum" | tee -a `hex_sdk GetKernelConsoleDevices`' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '            chmod 0644 $$PKG' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '        else' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '            MD5_VERIFIED=false' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '            echo "Failed to verify $$PKG md5 checksum" | tee -a `hex_sdk GetKernelConsoleDevices`' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '        fi' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '    done' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '    [ "$$MD5_VERIFIED" = "false" ] || break' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '    echo "Please mount a BMC VirtualMedia ISO that contains .pkg and .md5" | tee -a `hex_sdk GetKernelConsoleDevices`' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '    for h in `ip a | grep "inet " | grep -v -e "host lo" -e pxebr0 | xargs -i echo {} | cut -d" " -f2 | cut -d"/" -f1`; do echo "or scp PkgAndMd5Files root@$$h:/mnt/install/" | tee -a `hex_sdk GetKernelConsoleDevices` ; done' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '    for s in /dev/sr* ; do' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '        timeout 10 mount $$s /mnt/iso || true' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '        if ls /mnt/iso/*.pkg >/dev/null 2>&1; then echo "Copying packages from ISO media" | tee -a `hex_sdk GetKernelConsoleDevices`; cp /mnt/iso/*.pkg* /mnt/install/ ; chmod 0644 /mnt/install/* ; break ; fi' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '        timeout 10 umount $$s || umount -l $$s' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '    done' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo '    sleep 60' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo 'done ) &' >> $(ROOTDIR)/etc/rc.sysinit

include $(HEX_MAKEDIR)/hex_sdk.mk

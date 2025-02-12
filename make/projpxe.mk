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
	$(Q)md5sum < $(PROJ_PXE) > $(PROJ_SHIPDIR)/$(PROJ_PXE_LONGNAME).md5

$(PROJ_PXE_RD): $(HEX_PXE_RD) $(HEX_HWDETECT_FILES) $(PROJ_PPU) $(HEX_DATADIR)/hex_install/hex_pxe_install.sh.in
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/mountinitramfs '$(MAKECMD) PPU=$$(readlink $(PROJ_RELEASE)).pkg ROOTDIR=@ROOTDIR@ pxe_ramdisk_install' $< $@,"  GEN     $@")

pxe_ramdisk_install::
	$(Q)echo "sys.install.mode = pxe" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "if [ -d /sys/firmware/efi ]; then /usr/bin/hostname uefi-installer; else /usr/bin/hostname bios-installer; fi" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)sed -e 's/@IMAGE_NAME@/$(PROJ_RELEASE_LONGNAME)\*.pkg/' $(HEX_DATADIR)/hex_install/hex_pxe_install.sh.in > $(ROOTDIR)/usr/sbin/hex_pxe_install
	$(Q)chmod 755 $(ROOTDIR)/usr/sbin/hex_pxe_install
	$(Q)chroot $(ROOTDIR) bash -c "rm -f /etc/systemd/system/NetworkManager.service"
	$(Q)chroot $(ROOTDIR) bash -c "systemctl enable NetworkManager"
	$(Q)echo "for i in \$$(cat /proc/cmdline); do if [[ \$$i =~ pxe_via_nfs= ]]; then eval \$$i ; fi ; done" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "if [ -z \$${pxe_via_nfs+x} ]; then" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "    for i in {1..10}; do" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "        if timeout 2 ping -c 1 $(HEX_COMPANY_DN); then" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "            timeout 180 /usr/bin/wget -r -np --cut-dirs=3 -nH -R --show-progress -P /mnt/install -A "$(PROJ_RELEASE_LONGNAME)\*.pkg*" $(HEX_COMPANY_DN)/" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "            break" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "        elif timeout 2 ping -c 1 $(PXESERVER_IP); then" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "            timeout 180 /usr/bin/wget -r -np --cut-dirs=3 -nH -R --show-progress -P /mnt/install -A "$(PROJ_RELEASE_LONGNAME)\*.pkg*" $(PXESERVER_IP)/" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "            break" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "        else" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "            sleep 1" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "        fi" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "    done" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "else" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "    /bin/mkdir -p /mnt/nfs" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "    timeout 10 /bin/mount -t nfs -o nolock \$$pxe_via_nfs /mnt/nfs" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "    timeout 180 /usr/bin/rsync --progress /mnt/nfs/$(PROJ_RELEASE_LONGNAME)*.pkg /mnt/install/" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "    sync" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)echo "    timeout 10 umount /mnt/nfs || umount -l /mnt/nfs" >> $(ROOTDIR)/etc/rc.d/rc.local
	$(Q)chroot $(ROOTDIR) bash -c "chmod 755 /etc/rc.d/rc.local"
	$(Q)echo "fi" >> $(ROOTDIR)/etc/rc.d/rc.local

# Install project build label into installer image
pxe_ramdisk_install:: build_label_install

# Install hardware detection and kernel modules into pxe image
pxe_ramdisk_install:: hex_hwdetect_install

# Install welcome messages into installer image
pxe_ramdisk_install::
	$(Q)if [ -n "$(PROJ_LONGNAME)" ]; then echo "Welcome to $(PROJ_LONGNAME) Installer" > $(ROOTDIR)/etc/motd ; fi


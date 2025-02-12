# HEX SDK

include ../../build.mk

DISTFILES := $(HEX_KERNEL_EXTRA_CGZ)
DISTFILES += $(HEX_KERNEL_FIRMWARE_CGZ)
DISTFILES += $(HEX_KERNEL_FIRMWARE_EXTRA_CGZ)
DISTFILES += $(HEX_KERNEL)
DISTFILES += $(HEX_FIRMWARE)
DISTFILES += $(HEX_MINI_INITRD)
DISTFILES += $(HEX_BASE_ROOTFS)
DISTFILES += $(HEX_FULL_ROOTFS)
ALL += $(DISTFILES)

ifeq ($(USE_CORE),0)
MKINITRD_OPTS :=
MKROOTFS_OPTS :=
else
MKINITRD_OPTS := -D -r $(HEX_BOOTDIR)/$(KERNEL_INITRD) -p
MKROOTFS_OPTS := -D -r $(HEX_ROOTFS_CORE) -u
endif

# Delete _debug and .size files
PKGCLEAN += $(patsubst %.cgz,%*,$(DISTFILES))

# Delete RPM packages for mini, base and full images
DISTCLEAN += $(CURDIR)/RPMS-mini $(CURDIR)/RPMS-base $(CURDIR)/RPMS-full

# MINI_PKGS, BASE_PKGS, and FULL_PKGS are based on HEX_ARCH
include $(HEX_SRCDIR)/rootfs/pkgs_$(HEX_VER)_$(HEX_ARCH).mk
$(shell for r in $(BASE_LOCK_PKGS); do echo $$r >> $(LOCKED_RPMS); done)

$(HEX_KERNEL):
	$(call RUN_CMD, cp $(HEX_BOOTDIR)/$(KERNEL_IMAGE) $@,"  GEN     $@")

# UEFI firmware boot loader x64
$(HEX_FIRMWARE): /boot/efi/EFI/centos/grubx64.efi
	$(call RUN_CMD, cp $< $@,"  GEN     $@")

# HEX Mini Initial Ramdisk: Base filesystem for ramdisk image used by grub right before switchroot to hard disk

MINI_DEPS := \
	$(HEX_DATADIR)/os/init_functions.sh \
	$(HEX_DATADIR)/os/switchroot_init.sh \
	$(HEX_DISTDIR)/busybox-i686

$(HEX_MINI_INITRD): $(MINI_DEPS) $(HEX_SCRIPTSDIR)/makerootfs $(call VARIABLE_TARGET,$(MINI_PKGS))
	$(call RUN_CMD_TIMED, $(SHELL) $(HEX_SCRIPTSDIR)/makerootfs $(MKINITRD_OPTS) -c '$(MAKECMD) ROOTDIR=@ROOTDIR@ mini_rootfs_install' $@, "  GEN     $@")

$(call HEX_CHECKROOTFS,$(HEX_MINI_INITRD))

DEFAULT_PASSWD := admin

RPMS-mini:
	$(Q)mkdir -p $@

.PHONY: mini_rootfs_install
mini_rootfs_install:: RPMS-mini
	$(Q)cd $< && dnf download -y $(addsuffix .x86_64,$(MINI_PKGS))
	$(Q)$(INSTALL_RPM) $(ROOTDIR) $$(ls -d $(BLDDIR)/$</*.rpm)
	$(Q)ldconfig -r $(ROOTDIR)
	@# install busybox
	$(Q)$(INSTALL_PROGRAM) $(ROOTDIR) $(HEX_DISTDIR)/busybox-i686 ./bin
	$(Q)cd $(ROOTDIR)/bin && for i in $$(./busybox-i686 --list); do ln -s busybox-i686 $$i 2>/dev/null; done || true
	@# remove doc or static files
	$(Q)rm -rf $(ROOTDIR)/usr/share
	@# Mini ramdisk will run a special init script before executing switchroot over to the hard drive
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/init_functions.sh ./etc/init_functions
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/switchroot_init.sh ./sbin/switchroot_init
	@# /init point to /sbin/switchroot_init
	$(Q)ln -sf /sbin/switchroot_init $(ROOTDIR)/init
	$(Q)rm -rf $(ROOTDIR)/var/lock
	$(Q)find $(ROOTDIR) -executable -type f | xargs -i bash -c "file -i {} | grep -q 'charset=binary' && strip {} 2>/dev/null" || true

## HEX Base Rootfs: components needed by installer images

BASE_DEPS := \
	$(HEX_DATADIR)/os/base.modules \
	$(HEX_DATADIR)/os/systemd-hexctl-sys.service \
	$(HEX_DATADIR)/os/passwd \
	$(HEX_DATADIR)/os/shadow \
	$(HEX_DATADIR)/os/group \
	$(HEX_DATADIR)/os/hosts \
	$(HEX_DATADIR)/os/bashrc.def \
	$(HEX_DATADIR)/os/init_functions.sh \
	$(HEX_DATADIR)/os/rc.sysinit.sh \
	$(HEX_DATADIR)/hex_install/grub-set-default.sh \
	$(HEX_DATADIR)/os/bootstrap.sh

# Note: modules in $(KERNEL_MODULE_DIR) and $(KERNEL_FIRMWARE_DIR) are required by $(INSTALL_MODULES)
$(HEX_BASE_ROOTFS): $(BASE_DEPS) $(call VARIABLE_TARGET,$(BASE_PKGS) $(BASE_PKGS_NOARCH)) $(HEX_SCRIPTSDIR)/makerootfs
	$(call RUN_CMD_TIMED, $(SHELL) $(HEX_SCRIPTSDIR)/makerootfs $(MKROOTFS_OPTS) -c '$(MAKECMD) ROOTDIR=@ROOTDIR@ base_rootfs_install' $@, "  GEN     $@")

.PHONY: base_rootfs_install
base_rootfs_install:
	$(Q)$(INSTALL_DNF) -w -n  $(ROOTDIR) curl
	$(Q)$(INSTALL_DNF) -t base -d '$(BASE_LOCK_PKGS)' $(ROOTDIR) $(addsuffix .x86_64,$(BASE_PKGS)) $(BASE_PKGS_NOARCH)
	$(Q)ldconfig -r $(ROOTDIR)
	$(Q)rm -f $(ROOTDIR)/etc/grub.d/01_menu_auto_hide
	$(Q)chroot $(ROOTDIR) systemctl disable kdump
	$(Q)chroot $(ROOTDIR) systemctl disable NetworkManager
	$(Q)chroot $(ROOTDIR) systemctl mask NetworkManager
	$(Q)chroot $(ROOTDIR) systemctl mask console-getty
	$(Q)chroot $(ROOTDIR) systemctl unmask systemd-logind
	$(Q)chroot $(ROOTDIR) systemctl enable serial-getty@ttyS0
	$(Q)chroot $(ROOTDIR) systemctl unmask getty.target
	$(Q)$(INSTALL_MODULES) $(ROOTDIR) $(HEX_DATADIR)/os/base.modules
	$(Q)$(INSTALL_DATA) $(ROOTDIR) $(HEX_DATADIR)/os/systemd-hexctl-sys.service ./lib/systemd/system
	$(Q)$(INSTALL_DATA) $(ROOTDIR) $(HEX_DATADIR)/os/passwd ./etc
	$(Q)$(INSTALL_DATA) $(ROOTDIR) $(HEX_DATADIR)/os/shadow ./etc
	$(Q)$(INSTALL_DATA) $(ROOTDIR) $(HEX_DATADIR)/os/group ./etc
	$(Q)$(INSTALL_DATA) $(ROOTDIR) $(HEX_DATADIR)/os/hosts ./etc
	$(Q)$(INSTALL_DATA) -f $(ROOTDIR) $(HEX_DATADIR)/os/bashrc.def ./root/.bashrc
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/init_functions.sh ./etc/init_functions
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/rc.sysinit.sh ./etc/rc.sysinit
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/hex_kdump.sh ./sbin/hex_kdump
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/hex_install/grub-set-default.sh ./usr/sbin/grub-set-default
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/bootstrap.sh ./usr/sbin/bootstrap
	$(Q)(echo "$(DEFAULT_PASSWD)"; sleep 1; echo "$(DEFAULT_PASSWD)") | chroot $(ROOTDIR) /usr/bin/passwd root >/dev/null 2>&1
	$(Q)(echo "$(DEFAULT_PASSWD)"; sleep 1; echo "$(DEFAULT_PASSWD)") | chroot $(ROOTDIR) /usr/bin/passwd admin >/dev/null 2>&1
	$(Q)mkdir -p $(ROOTDIR)/etc/systemd/system/sysinit.target.wants $(ROOTDIR)/etc/systemd/network
	$(Q)ln -sf /lib/systemd/system/systemd-hexctl-sys.service $(ROOTDIR)/etc/systemd/system/sysinit.target.wants/systemd-hexctl-sys.service
	$(Q)grpconv --root $(ROOTDIR)
	# https://forum.proxmox.com/threads/pve-5-0-driver-pcspkr-is-already-registered-aborting-message.35497/
	$(Q)echo "blacklist pcspkr" >> $(ROOTDIR)/etc/modprobe.d/blacklist-pcspkr.conf
	$(Q)echo "blacklist cdc_ether" >> $(ROOTDIR)/etc/modprobe.d/usb_net.conf
	$(Q)echo "blacklist usbnet" >> $(ROOTDIR)/etc/modprobe.d/usb_net.conf
	$(Q)echo "blacklist floppy" > $(ROOTDIR)/etc/modprobe.d/floppy.conf
	$(Q)ln -sf /sbin/init $(ROOTDIR)/init
	# keep only /etc/grub.d/01_users which is needed by grub password
	$(Q)chroot $(ROOTDIR) bash -c "find /etc/grub.d/ -type f | grep -v _users | xargs -i rm -f {}"
	# remove useless folders and files
	$(Q)chroot $(ROOTDIR) bash -c "rm -f /root/original-ks.cfg /root/anaconda-* /.dockerenv /etc/passwd- /etc/shadow- /etc/group- /etc/inittab"
	$(Q)chroot $(ROOTDIR) bash -c "rmdir /home /media /srv /usr/games /usr/local/games"
	$(Q)chroot $(ROOTDIR) bash -c "rm -rf /usr/local/share/* /usr/share/{man,doc,licenses} /usr/src /usr/local/src /var/{log,cache}/* /tmp/* /lib/.build-id /root/buildinfo"
	$(Q)[ ! -e $(ROOTDIR)/etc/selinux/config ] || sed -i "s/^SELINUX=.*/SELINUX=disabled/" $(ROOTDIR)/etc/selinux/config

$(call HEX_CHECKROOTFS,$(HEX_BASE_ROOTFS))

## HEX Full Rootfs: Base filesystem plus hex_sdk required packages

FULL_DEPS := $(HEX_DATADIR)/os/systemd-hexctl-user.service

$(HEX_FULL_ROOTFS): $(HEX_BASE_ROOTFS) $(FULL_DEPS) $(call VARIABLE_TARGET,$(FULL_PKGS) $(FULL_PKGS_NOARCH))
	$(call RUN_CMD_TIMED, $(SHELL) $(HEX_SCRIPTSDIR)/mountrootfs -D '$(MAKECMD) ROOTDIR=@ROOTDIR@ full_rootfs_install' $< $@,"  GEN     $@")

.PHONY: full_rootfs_install
full_rootfs_install::
	$(Q)$(INSTALL_DNF) -t full $(ROOTDIR) $(addsuffix .x86_64,$(FULL_PKGS)) $(FULL_PKGS_NOARCH)
	$(Q)ldconfig -r $(ROOTDIR)
	$(Q)$(INSTALL_DATA) $(ROOTDIR) $(HEX_DATADIR)/os/systemd-hexctl-user.service ./lib/systemd/system
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/os/hex_login.sh ./sbin/hex_login
	$(Q)sed -i 's;/sbin/agetty ;/sbin/agetty -n -l /usr/sbin/hex_login ;' $(ROOTDIR)/usr/lib/systemd/system/serial-getty@.service
	$(Q)sed -i 's;/sbin/agetty ;/sbin/agetty -n -l /usr/sbin/hex_login ;' $(ROOTDIR)/usr/lib/systemd/system/getty@.service
	$(Q)sed -i -e "s/.*PermitRootLogin .*/PermitRootLogin yes/" -e "s/.*PasswordAuthentication .*/PasswordAuthentication yes/" $(ROOTDIR)/etc/ssh/sshd_config
	$(Q)ln -sf /lib/systemd/system/systemd-hexctl-user.service $(ROOTDIR)/etc/systemd/system/multi-user.target.wants/systemd-hexctl-user.service
	$(Q)chroot $(ROOTDIR) systemctl disable crond sshd
	$(Q)chroot $(ROOTDIR) mv /etc/cron.daily/logrotate /etc/cron.hourly/ || true
	$(Q)chroot $(ROOTDIR) bash -c "rm -rf /usr/local/share/* /usr/share/{man,doc,licenses} /usr/src /usr/local/src /var/{log,cache}/* /tmp/* /lib/.build-id"

$(call HEX_CHECKROOTFS,$(HEX_FULL_ROOTFS))

include $(HEX_MAKEDIR)/hex_sdk.mk


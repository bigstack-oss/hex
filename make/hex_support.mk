# HEX SDK

#
# Project support functions: hex_tuning, hex_install, ...
#
# Files listed here will be installed to all rootfs files but not to hex_initramfs (initramfs.cgz)
# Because
# projrootfs.mk          => $(PROJ_ROOTFS): $(PROJ_BASE_ROOTFS) $(PROJ_INITTABS) $(PROGRAMS)
# project_definitions.mk => PROJ_BASE_ROOTFS  := $(HEX_FULL_ROOTFS)
# hex_install/Makefile   => PROJ_BASE_ROOTFS = $(HEX_BASE_ROOTFS)
#

$(call PROJ_INSTALL_DATA,,$(HEX_SCRIPTSDIR)/functions,./usr/lib/hex_sdk)
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_tuning/hex_tuning.sh,./usr/sbin/hex_tuning)
$(call PROJ_INSTALL_PROGRAM,,$(HEX_BINDIR)/hex_tuning_helper,./usr/sbin)
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_install/grub-get-default.sh,./usr/sbin/grub-get-default)
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_install/grub2-detect1,./etc/grub.d/00_header)
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_install/grub2-detect2,./etc/grub.d/05_part)
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_install/bigstack-logo.png,./root/bg.png)
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_fixpack/hex_fixpack_install.sh,./usr/sbin/hex_fixpack_install)

# Add a utility script that does not seem to really fit anywhere else
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_uptime,./usr/sbin/hex_uptime)

# dump_hdparm for use by support info
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/support/dump_hdparm.sh,./usr/bin/dump_hdparm)

# pstree used by support info
$(call PROJ_INSTALL_PROGRAM,-d,/usr/bin/pstree,./usr/bin)

# hexdump used by grub-get-default
$(call PROJ_INSTALL_PROGRAM,-d,/usr/bin/hexdump,./usr/bin)

#FIXME: Move this part to devtools
# Add non-production scripts
ifeq ($(PRODUCTION),0)
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_install/single.sh,./usr/sbin/single)
$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_install/wipembr.sh,./usr/sbin/wipembr)
endif

# Directory for crash dumps, support info files etc.
# Give LMI/CLI access to create/delete files
# Give everyone access so that hex_cli can write its crash dump here
rootfs_install::
	$(Q)mkdir -p $(ROOTDIR)/var/support
	$(Q)chroot $(ROOTDIR) chown root:www-data /var/support
	$(Q)chmod 777 $(ROOTDIR)/var/support
	$(Q)chmod g+s $(ROOTDIR)/var/support

# Directory for appliance specific data etc
rootfs_install::
	$(Q)mkdir -p $(ROOTDIR)/etc/appliance
	$(Q)chroot $(ROOTDIR) chown root:www-data /etc/appliance
	$(Q)chmod 777 $(ROOTDIR)/etc/appliance
	$(Q)chmod g+s $(ROOTDIR)/etc/appliance
	$(Q)mkdir -p $(ROOTDIR)/var/appliance-db
	$(Q)chroot $(ROOTDIR) chown root:www-data /var/appliance-db
	$(Q)chmod 777 $(ROOTDIR)/var/appliance-db
	$(Q)chmod g+s $(ROOTDIR)/var/appliance-db

# Directory for state, sla, etc
rootfs_install::
	$(Q)mkdir -p $(ROOTDIR)/etc/appliance/state
	$(Q)mkdir -p $(ROOTDIR)/etc/appliance/sla

# Directory for fixpack and update, etc
rootfs_install::
	$(Q)mkdir -p $(ROOTDIR)/var/fixpack
	$(Q)mkdir -p $(ROOTDIR)/var/update || true

$(PROJ_ROOTFS): $(HEX_DATADIR)/hex_install/hex_install.sh.in

rootfs_install::
	$(Q)sed -e 's/@QUIET_KERNEL_ARG@/$(QUIET_KERNEL_ARG)/' \
	        -e 's/@KERNEL_ARGS@/$(KERNEL_ARGS)/' \
	        -e 's/@CONSOLE_SPEED@/$(CONSOLE_SPEED)/' \
	        $(HEX_DATADIR)/hex_install/hex_install.sh.in >$(ROOTDIR)/usr/sbin/hex_install
	$(Q)chmod 755 $(ROOTDIR)/usr/sbin/hex_install
	$(Q)touch --reference=$(HEX_DATADIR)/hex_install/hex_install.sh.in $(ROOTDIR)/usr/sbin/hex_install



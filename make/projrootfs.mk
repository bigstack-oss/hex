# HEX SDK

#
# Project root filesystem creation
#

DISTCLEAN += $(PROJ_SHIPDIR)

# Symlink to release string to use for all build output files
PROJ_RELEASE := proj.release

help::
	@echo "rootfs       Create project rootfs (implies build) (non-recursive)"

.PHONY: rootfs
rootfs: $(PROJ_ROOTFS)
	@true

$(call HEX_CHECKROOTFS,$(PROJ_ROOTFS))

$(PROJ_ROOTFS): $(PROJ_BASE_ROOTFS) $(PROJ_BOOTSTRAP) $(PROGRAMS)
	$(Q)$(RM) $(PROJ_RELEASE)
	$(call RUN_CMD_TIMED, $(SHELL) $(HEX_SCRIPTSDIR)/mountrootfs -D '$(MAKECMD) ROOTDIR=@ROOTDIR@ PROJ_BUILD_LABEL="$(PROJ_BUILD_LABEL)" rootfs_install' $(PROJ_BASE_ROOTFS) $@, "  GEN     $@")
	@[ $(VERBOSE) -gt 0 ] && echo "Build label: $(PROJ_BUILD_LABEL)" || echo "  BUILD   $(PROJ_BUILD_LABEL)"
	$(Q)ln -sf $(PROJ_NAME)_$(PROJ_VERSION)_$(PROJ_BUILD_LABEL)_$(PROJ_BUILD_DESC) $(PROJ_RELEASE)

PKGCLEAN += $(PROJ_ROOTFS) $(PROJ_RELEASE)

ifeq ($(PROJ_ENABLE_ROOT_SHELL),1)
# Unlock root account
rootfs_install::
	$(Q)chroot $(ROOTDIR) /usr/bin/passwd -u root >/dev/null 2>&1 || true
else
# Lock root account
rootfs_install::
	$(Q)chroot $(ROOTDIR) /usr/bin/passwd -l root >/dev/null 2>&1 || true
endif

rootfs_install::
	$(Q)[ -z "$(PROJ_SYS_SETTINGS)" ] || $(INSTALL_DATA) -f $(ROOTDIR) $(PROJ_SYS_SETTINGS) ./etc/settings.sys
	$(Q)[ -z "$(PROJ_SETTINGS)" ] || $(INSTALL_DATA) -f $(ROOTDIR) $(PROJ_SETTINGS) ./etc/settings.txt
	$(Q)[ -z "$(PROJ_BOOTSTRAP)" ] || cat $(PROJ_BOOTSTRAP) >>$(ROOTDIR)/usr/sbin/bootstrap
	$(Q)sed -i '/GRUB_DEVICE=/s; /`; /boot`;' $(ROOTDIR)/usr/sbin/grub2-mkconfig

# Add version and build info to settings.sys and rootfs
build_label_install::
	$(Q)touch $(ROOTDIR)/etc/settings.sys
	$(Q)sed -i -e "/^sys\.vendor\./d" -e "/^sys\.product\./d" -e "/^sys\.build\./d" $(ROOTDIR)/etc/settings.sys
	$(Q)echo "# Vendor Information (Customizable) " >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.vendor.company = $(PROJ_VENDOR_COMPANY)" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.vendor.name = $(PROJ_VENDOR_NAME)" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.vendor.description = $(PROJ_VENDOR_LONGNAME)" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.vendor.version = $(PROJ_VENDOR_VERSION)" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "# Product Information " >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.product.name = $(PROJ_NAME)" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.product.description = $(PROJ_LONGNAME)" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.product.version = $(PROJ_VERSION)" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "# Release Information " >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.build.label = $(PROJ_BUILD_LABEL)" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.build.production = $(PRODUCTION)" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo "sys.build.signature = Developer" >> $(ROOTDIR)/etc/settings.sys
	$(Q)echo $(PROJ_NAME)_$(PROJ_VERSION)_$(PROJ_BUILD_LABEL)_$(PROJ_BUILD_DESC) > $(ROOTDIR)/etc/version
	$(Q)echo "Welcome to $(PROJ_NAME)_$(PROJ_VERSION)_$(PROJ_BUILD_LABEL)_$(PROJ_BUILD_DESC)" | toilet -f term --metal > $(ROOTDIR)/etc/motd

rootfs_install:: build_label_install

# Always install SDK shared lib for runtime linkage requirements
$(call PROJ_INSTALL_SO,,$(HEX_SDK_LIB_SO),./usr/lib64)

# Install development tools in non-production builds
ifeq ($(PRODUCTION),0)
rootfs_install:: devtools_install
endif

# Install trial license in non-production builds
ifeq ($(PRODUCTION),0)
rootfs_install:: license_install
else
rootfs_install:: license_generate
endif

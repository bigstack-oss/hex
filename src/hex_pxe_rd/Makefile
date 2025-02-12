# HEX SDK

include ../../../build.mk

# pxe installer ramdisk used by projects
ALL = $(HEX_PXE_RD)

# Delete _debug and .size files
PKGCLEAN += $(patsubst %.cgz,%*,$(HEX_PXE_RD)) RPMS

PROJ_NAME = hex_pxe
PROJ_BUILD_ROOTFS = 1

# Use base rootfs
PROJ_BASE_ROOTFS = $(HEX_INSTALL_RD)

# Install directly to final ramdisk image
PROJ_ROOTFS = $(HEX_PXE_RD)

PXE_RD_PKGS := net-tools iproute wget curl dhcp-client vim-enhanced tftp nfs-utils rsync

$(call PROJ_INSTALL_DNF,,$(PXE_RD_PKGS))

HEX_EXPORT_VARS += PXE_RD_PKGS

$(PROJ_ROOTFS): \
	$(SRCDIR)/dhclient.conf

rootfs_install::
	$(Q)$(INSTALL_DATA) $(ROOTDIR) $(SRCDIR)/dhclient.conf ./etc
	$(Q)chroot $(ROOTDIR) systemctl mask gssproxy.service
	$(Q)chroot $(ROOTDIR) bash -c "rm -rf /usr/local/share/* /usr/share/{man,doc,licenses} /usr/src /usr/local/src /var/{log,cache}/* /tmp/* /lib/.build-id"
	$(Q)find $(ROOTDIR)/usr -executable -type f | xargs -i bash -c "file -i {} | grep -q 'charset=binary' && strip {} 2>/dev/null" || true

include $(HEX_MAKEDIR)/hex_sdk.mk


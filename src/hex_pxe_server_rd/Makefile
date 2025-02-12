# HEX SDK

include ../../../build.mk

# pxe server ramdisk used by projects
ALL = $(HEX_PXE_SERVER_RD)

# Delete _debug and .size files
PKGCLEAN += $(patsubst %.cgz,%*,$(HEX_PXE_SERVER_RD)) RPMS

PROJ_NAME = hex_pxe_server
PROJ_BUILD_ROOTFS = 1

PROJ_HARDWARE = qemu vmware generic

# Use base rootfs
PROJ_BASE_ROOTFS = $(HEX_PXE_RD)

# Install directly to final ramdisk image
PROJ_ROOTFS = $(HEX_PXE_SERVER_RD)

LDLINUX_PATH := usr/share/syslinux/ldlinux.c32
PXELINUX_PATH := usr/share/syslinux/pxelinux.0
FTPBOOT_PATH := var/ftpboot

PXE_SERVER_RD_PKGS := syslinux dnsmasq openssh-server openssh-clients lighttpd dhcp-server tcpdump

$(call PROJ_INSTALL_DNF,,$(PXE_SERVER_RD_PKGS))

HEX_EXPORT_VARS += PXE_SERVER_RD_PKGS

rootfs_install::
	$(Q)cp -f $(HEX_SHMODDIR)/sdk_preset.sh $(ROOTDIR)/usr/lib/hex_sdk/modules/
	$(Q)mkdir -p $(ROOTDIR)/$(FTPBOOT_PATH)/pxelinux.cfg
	$(Q)ln -sf /mnt/install $(ROOTDIR)/$(FTPBOOT_PATH)/firmware
	$(Q)cp -f $(ROOTDIR)/$(LDLINUX_PATH) $(ROOTDIR)/$(FTPBOOT_PATH)/
	$(Q)cp -f $(ROOTDIR)/$(PXELINUX_PATH) $(ROOTDIR)/$(FTPBOOT_PATH)/
	$(Q)sed 's/@PXESERVER_IP@/$(PXESERVER_IP)/' $(SRCDIR)/dhcpd.conf > $(ROOTDIR)/etc/dhcp/dhcpd.conf
	$(Q)$(INSTALL_DATA) $(ROOTDIR) $(SRCDIR)/dnsmasq.conf ./etc
	$(Q)echo -e "\n$(PXESERVER_IP) $(HEX_COMPANY_DN)\n" >> $(ROOTDIR)/etc/hosts
	$(Q)echo "$(PXESERVER_IP) $(HEX_COMPANY_DN)" > $(ROOTDIR)/etc/dnsmasq.d/hosts.conf
	$(Q)echo "ip link add name pxebr0 type bridge vlan_filtering 1 stp_state 1" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo "for i in \$$(ip -o link show | awk -F': ' '{print \$$2}' | grep -v lo); do ip link set \$$i up ; done" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo "for i in \$$(ip -o link show | awk -F': ' '{print \$$2}' | grep -v -e lo -e pxebr0); do timeout 10 dhclient \$$i 2>/dev/null || timeout 10 dhclient \$$i 2>/dev/null || ip link set \$$i master pxebr0 ; done" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo "ip addr add dev pxebr0 $(PXESERVER_IP)/24" >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo 'echo "pxebr0" > /tmp/netboot_interface' >> $(ROOTDIR)/etc/rc.sysinit
	$(Q)echo 'server.dir-listing = "enable"' >> $(ROOTDIR)/etc/lighttpd/lighttpd.conf
	$(Q)sed -i "s/^server.use-ipv6 = .*/#&/" $(ROOTDIR)/etc/lighttpd/lighttpd.conf
	$(Q)chroot $(ROOTDIR) bash -c "systemctl enable sshd"
	$(Q)chroot $(ROOTDIR) bash -c "systemctl enable dnsmasq"
	$(Q)chroot $(ROOTDIR) bash -c "systemctl enable dhcpd"
	$(Q)chroot $(ROOTDIR) bash -c "systemctl enable lighttpd"
	$(Q)chroot $(ROOTDIR) bash -c "cd /var/www/ && rm -rf lighttpd && ln -s /$(FTPBOOT_PATH) lighttpd"
	$(Q)chroot $(ROOTDIR) bash -c "rm -rf /usr/local/share/* /usr/share/{man,doc,licenses} /usr/src /usr/local/src /var/cache/* /var/log/*.log /var/log/rhsm /tmp/* /lib/.build-id"
	$(Q)find $(ROOTDIR)/usr -executable -type f | xargs -i bash -c "file -i {} | grep -q 'charset=binary' && strip {} 2>/dev/null" || true
	$(Q)sed -i -e "s/.*PermitRootLogin .*/PermitRootLogin yes/" -e "s/.*PasswordAuthentication .*/PasswordAuthentication yes/" $(ROOTDIR)/etc/ssh/sshd_config

include $(HEX_MAKEDIR)/hex_sdk.mk

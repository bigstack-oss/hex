# HEX SDK

$(call PROJ_INSTALL_SCRIPT,-f,$(HEX_DATADIR)/hex_sdk,./usr/sbin/hex_sdk)

rootfs_install:: $(hex_shell_MODULES_PRE)
	$(Q)mkdir -p $(ROOTDIR)/usr/lib/hex_sdk/modules.pre
	$(Q)for i in $^; do cp -f $$i $(ROOTDIR)/usr/lib/hex_sdk/modules.pre/ ; done

rootfs_install:: $(hex_shell_MODULES)
	$(Q)mkdir -p $(ROOTDIR)/usr/lib/hex_sdk/modules
	$(Q)for i in $^; do cp -f $$i $(ROOTDIR)/usr/lib/hex_sdk/modules/ ; done

rootfs_install:: $(hex_shell_MODULES_POST)
	$(Q)mkdir -p $(ROOTDIR)/usr/lib/hex_sdk/modules.post
	$(Q)for i in $^; do cp -f $$i $(ROOTDIR)/usr/lib/hex_sdk/modules.post/ ; done

# HEX SDK
        
#
# Project initramfs creation
# Used by grub during HDD boot
#

help::
	@echo "initramfs    Create project initramfs (used by grub for HDD boot) (non-recursive)"

.PHONY: initramfs
initramfs: $(PROJ_INITRD)
	@true

#$(PROJ_INITRD): $(HEX_BINDIR)/hex_fsck

#proj_initrd_install::
#	$(Q)$(INSTALL_PROGRAM) $(ROOTDIR) $(HEX_BINDIR)/hex_fsck ./usr/sbin

# Install SDK shared lib if not linking statically
ifeq ($(DEBUG),0)
$(PROJ_INITRD): $(HEX_SDK_LIB_SO)
proj_initrd_install:: 
	$(Q)$(INSTALL_SO) $(ROOTDIR) $(HEX_SDK_LIB_SO) ./usr/lib64
endif

# Add hardware detection and kernel modules into ramdisk
$(PROJ_INITRD): $(HEX_HWDETECT_FILES)
proj_initrd_install:: hex_hwdetect_install_essential

$(PROJ_INITRD): $(HEX_MINI_INITRD)
	$(call RUN_CMD_TIMED, $(SHELL) $(HEX_SCRIPTSDIR)/mountinitramfs '$(MAKECMD) ROOTDIR=@ROOTDIR@ proj_initrd_install' $< $@, "  GEN     $@")

PKGCLEAN += $(PROJ_INITRD)


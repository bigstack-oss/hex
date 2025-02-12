# HEX SDK

# Register rootfs to be checked during 'checkrootfs' target
# Usage: $(call HEX_CHECKROOTFS,rootfs)

define HEX_CHECKROOTFS_HELPER
CHECKROOTFS += $(1).checkrootfs
PKGCLEAN += $(1).checkrootfs
$(1).checkrootfs: $(1)
	$$(call RUN_CMD_TIMED,$$(SHELL) $$(HEX_SCRIPTSDIR)/mountrootfs -r '$$(MAKECMD) ROOTDIR=@ROOTDIR@ checkrootfs_helper' $$<,"  CHK     $$<")
	$$(Q)touch $$@

endef
define HEX_CHECKROOTFS
$(eval $(call HEX_CHECKROOTFS_HELPER,$(1)))
endef


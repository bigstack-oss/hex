# HEX SDK
        
help::
	@echo "checkrootfs  Check root filesystems for errors"

.PHONY: checkrootfs
checkrootfs:: _reconfigure _checkrootfs_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _checkrootfs_r
_checkrootfs_r:: _checkrootfs
	$(call RECURSE,_checkrootfs_r)

# Non-recursive
.PHONY: _checkrootfs
_checkrootfs:: $(CHECKROOTFS)
	@true

.PHONY: checkrootfs_helper
checkrootfs_helper:
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi
	$(Q)$(INSTALL_SCRIPT) $(ROOTDIR) $(HEX_SCRIPTSDIR)/checkrootfs ./
	$(Q)$(INSTALL_PROGRAM) -d $(ROOTDIR) /usr/bin/readelf ./usr/bin
	$(Q)$(INSTALL_SCRIPT) $(ROOTDIR) /usr/bin/ldd ./usr/bin
	$(Q)chroot $(ROOTDIR) sh ./checkrootfs


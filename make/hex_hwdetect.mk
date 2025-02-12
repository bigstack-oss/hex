# HEX SDK

# Look for project hardware files in project source directory unless told otherwise
PROJ_HARDWARE_DIR ?= $(SRCDIR)

#
# hex_hwdetect (hardware detection) install target
#

HEX_HWDETECT_FILES := \
	$(HEX_SCRIPTSDIR)/functions \
	$(HEX_DATADIR)/hex_hwdetect/hex_hwdetect.sh.in \
	$(HEX_DATADIR)/hex_hwdetect/rc.hwdetect.sh \
	$(HEX_DATADIR)/hex_hwdetect/rc.hwdetect_debug.sh \
	$(HEX_DATADIR)/hex_hwdetect/hwdetect_functions.sh \
	$(wildcard $(HEX_DATADIR)/hex_hwdetect/*.hwdetect) \
	$(wildcard $(PROJ_HARDWARE_DIR)/*.hwdetect) \
	$(wildcard $(HEX_DATADIR)/hex_hwdetect/*_settings.sys) \
	$(wildcard $(PROJ_HARDWARE_DIR)/*_settings.sys) \
	$(wildcard $(HEX_DATADIR)/hex_hwdetect/*_settings.txt) \
	$(wildcard $(PROJ_HARDWARE_DIR)/*_settings.txt) \
	$(wildcard $(HEX_DATADIR)/hex_hwdetect/*.rc) \
	$(wildcard $(PROJ_HARDWARE_DIR)/*.rc) \
	$(wildcard $(HEX_DATADIR)/hex_hwdetect/*.modules) \
	$(wildcard $(PROJ_HARDWARE_DIR)/*.modules) \
	$(wildcard $(HEX_DATADIR)/hex_hwdetect/*.firmware) \
	$(wildcard $(PROJ_HARDWARE_DIR)/*.firmware) \
	$(wildcard $(HEX_DATADIR)/hex_hwdetect/*.kernel.args) \
	$(wildcard $(PROJ_HARDWARE_DIR)/*.kernel.args)

$(PROJ_ROOTFS): $(HEX_HWDETECT_FILES)

# Install hardware detection and kernel modules/firmware into project rootfs
rootfs_install:: hex_hwdetect_install
	@true

RC_HWDETECT := $(HEX_DATADIR)/hex_hwdetect/rc.hwdetect.sh
ifeq ($(PRODUCTION),0)
ifneq ($(DEBUG),0)
RC_HWDETECT := $(HEX_DATADIR)/hex_hwdetect/rc.hwdetect_debug.sh
endif
endif
RC_NICDETECT := $(HEX_DATADIR)/hex_hwdetect/rc.nicdetect.sh

_ALL_MOD=/tmp/_all.modules
ALL_MOD=/tmp/all.modules
_ALL_FRM=/tmp/_all.firmware
ALL_FRM=/tmp/all.firmware

.PHONY: hex_hwdetect_install_essential
hex_hwdetect_install_essential:
	@if [ -z "$(ROOTDIR)" ]; then echo "Error: ROOTDIR not set" >&2; exit 1; fi
	@[ -n "$(PROJ_HARDWARE)" ] || exit 0
	$(Q)$(INSTALL_DATA) $(ROOTDIR) $(HEX_SCRIPTSDIR)/functions ./usr/lib/hex_sdk
	$(Q)sed -e 's/@HARDWARE@/$(PROJ_HARDWARE)/' -e 's/@PRODUCTION@/$(PRODUCTION)/' $(HEX_DATADIR)/hex_hwdetect/hex_hwdetect.sh.in >$(ROOTDIR)/usr/sbin/hex_hwdetect
	$(Q)chmod 755 $(ROOTDIR)/usr/sbin/hex_hwdetect
	$(Q)touch --reference=$(HEX_DATADIR)/hex_hwdetect/hex_hwdetect.sh.in $(ROOTDIR)/usr/sbin/hex_hwdetect
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(RC_HWDETECT) ./etc/rc.hwdetect
	$(Q)mkdir -p $(ROOTDIR)/etc/hwdetect.d
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(HEX_DATADIR)/hex_hwdetect/hwdetect_functions.sh ./etc/hwdetect.d/hwdetect_functions
	$(Q)rm -f $(_ALL_MOD) $(ALL_MOD) $(_ALL_FRM) $(ALL_FRM)
	$(Q)[ ! -f $(SRCDIR)/*.modules ] || cat $(SRCDIR)/*.modules >>$(_ALL_MOD)
	$(Q)[ ! -f $(SRCDIR)/*.firmware ] || cat $(SRCDIR)/*.firmware >>$(_ALL_FRM)
	$(Q)for H in $(PROJ_HARDWARE) ; do \
	  [ ! -f $(ROOTDIR)/etc/hwdetect.d/$${H}_$@ ] || continue ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$$H.hwdetect ] || $(INSTALL_SCRIPT) $(ROOTDIR) $(HEX_DATADIR)/hex_hwdetect/$$H.hwdetect ./etc/hwdetect.d ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$$H.hwdetect ] || $(INSTALL_SCRIPT) $(ROOTDIR) $(PROJ_HARDWARE_DIR)/$$H.hwdetect ./etc/hwdetect.d ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$${H}_settings.sys ] || cat $(HEX_DATADIR)/hex_hwdetect/$${H}_settings.sys >> $(ROOTDIR)/etc/hwdetect.d/$${H}_settings.sys ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$${H}.kernel.args ] || cat $(PROJ_HARDWARE_DIR)/$${H}.kernel.args > $(ROOTDIR)/etc/hwdetect.d/$${H}.projkernel.args ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$${H}.kernel.args ] || cat $(HEX_DATADIR)/hex_hwdetect/$${H}.kernel.args > $(ROOTDIR)/etc/hwdetect.d/$${H}.kernel.args ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$${H}_settings.sys ] || cat $(PROJ_HARDWARE_DIR)/$${H}_settings.sys >> $(ROOTDIR)/etc/hwdetect.d/$${H}_settings.sys ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$${H}_settings.txt ] || cat $(HEX_DATADIR)/hex_hwdetect/$${H}_settings.txt >> $(ROOTDIR)/etc/hwdetect.d/$${H}_settings.txt ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$${H}_settings.txt ] || cat $(PROJ_HARDWARE_DIR)/$${H}_settings.txt >> $(ROOTDIR)/etc/hwdetect.d/$${H}_settings.txt ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$$H.rc ] || cat $(HEX_DATADIR)/hex_hwdetect/$$H.rc >> $(ROOTDIR)/etc/hwdetect.d/$$H.rc ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$$H.rc ] || cat $(PROJ_HARDWARE_DIR)/$$H.rc >> $(ROOTDIR)/etc/hwdetect.d/$$H.rc ; \
	  [ ! -f $(ROOTDIR)/etc/hwdetect.d/$$H.rc ] || chmod 755 $(ROOTDIR)/etc/hwdetect.d/$$H.rc ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$$H.ess.modules ] || (echo >>$(_ALL_MOD) ; cat $(HEX_DATADIR)/hex_hwdetect/$$H.ess.modules >>$(_ALL_MOD)) ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$$H.ess.modules ] || (echo >>$(_ALL_MOD) ; cat $(PROJ_HARDWARE_DIR)/$$H.ess.modules >>$(_ALL_MOD)) ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$$H.ess.firmware ] || (echo >>$(_ALL_FRM) ; cat $(HEX_DATADIR)/hex_hwdetect/$$H.ess.firmware >$(_ALL_FRM)) ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$$H.ess.firmware ] || (echo >>$(_ALL_FRM) ; cat $(PROJ_HARDWARE_DIR)/$$H.ess.firmware >>$(_ALL_FRM)) ; \
	  touch $(ROOTDIR)/etc/hwdetect.d/$${H}_$@ ; \
	done
	$(Q)cat $(_ALL_MOD) | sort -u | sed "/^$$/d" > $(ALL_MOD)
	$(Q)cat $(_ALL_FRM) | sort -u | sed "/^$$/d" > $(ALL_FRM)
	$(Q)$(INSTALL_MODULES) $(ROOTDIR) $(ALL_MOD)
	$(Q)$(INSTALL_FIRMWARE) $(ROOTDIR) $(ALL_FRM)

hex_hwdetect_install: hex_hwdetect_install_essential
	$(Q)$(INSTALL_SCRIPT) -f $(ROOTDIR) $(RC_NICDETECT) ./etc/rc.nicdetect
	$(Q)for H in $(PROJ_HARDWARE) ; do \
	  [ ! -f $(ROOTDIR)/etc/hwdetect.d/$${H}_$@ ] || continue ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$$H.modules ] || (echo >>$(_ALL_MOD) ; cat $(HEX_DATADIR)/hex_hwdetect/$$H.modules >>$(_ALL_MOD)) ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$$H.modules ] || (echo >>$(_ALL_MOD) ; cat $(PROJ_HARDWARE_DIR)/$$H.modules >>$(_ALL_MOD)) ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$$H.firmware ] || (echo >>$(_ALL_FRM) ; cat $(HEX_DATADIR)/hex_hwdetect/$$H.firmware >>$(_ALL_FRM)) ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$$H.firmware ] || (echo >>$(_ALL_FRM) ; cat $(PROJ_HARDWARE_DIR)/$$H.firmware >>$(_ALL_FRM)) ; \
	  [ ! -f $(HEX_DATADIR)/hex_hwdetect/$$H.sdk ] || $(INSTALL_SCRIPT) $(ROOTDIR) $(HEX_DATADIR)/hex_hwdetect/$$H.sdk ./etc/hwdetect.d ; \
	  [ ! -f $(PROJ_HARDWARE_DIR)/$$H.sdk ] || $(INSTALL_SCRIPT) $(ROOTDIR) $(PROJ_HARDWARE_DIR)/$$H.sdk ./etc/hwdetect.d ; \
	  touch $(ROOTDIR)/etc/hwdetect.d/$${H}_$@ ; \
	done
	$(Q)cat $(_ALL_MOD) | sort -u | sed "/^$$/d" > $(ALL_MOD)
	$(Q)cat $(_ALL_FRM) | sort -u | sed "/^$$/d" > $(ALL_FRM)
	$(Q)$(INSTALL_MODULES) $(ROOTDIR) $(ALL_MOD)
	$(Q)$(INSTALL_FIRMWARE) $(ROOTDIR) $(ALL_FRM)

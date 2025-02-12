# HEX SDK

#
# Deply Project PXE artifacts to a pxe server via NFS
#

PROJ_PXE_NAME := $(shell PR=$$(readlink $(PROJ_RELEASE)); echo $${PR%_*})

help::
	@echo "pxedeploy    Deploy project pxe artifacts to a pxe server via NFS (non-recursive)"

# Deploy to a running PXE server
.PHONY: pxedeploy
pxedeploy: $(PROJ_KERNEL) $(PROJ_PPU) $(PROJ_PXE_RD)
	$(Q)$(MAKECMD) PROJ_PPU_LONGNAME=$$(readlink $(PROJ_RELEASE)).pkg pxe_deploy

.PHONY: pxe_deploy
pxe_deploy:
	$(call RUN_CMD_TIMED,$(SHELL) $(HEX_SCRIPTSDIR)/makepxedeploy $(QUIET_FLAG) -s $(PROJ_NFS_SERVER) -p $(PROJ_NFS_PATH) $(PROJ_PXE_NAME) $(PROJ_KERNEL) $(PROJ_PXE_RD) $(PROJ_SHIPDIR)/$(PROJ_PPU_LONGNAME) $(PROJ_PXE_PROFILE),"  PXEDPL  $(PROJ_PPU_LONGNAME)")

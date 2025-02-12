# HEX SDK

# Always install hex_crashd into project

PROJ_BOOTSTRAP += $(HEX_DATADIR)/hex_crashd/bootstrap_hex_crashd

$(call PROJ_INSTALL_PROGRAM,,$(HEX_BINDIR)/hex_crashd,./usr/sbin)

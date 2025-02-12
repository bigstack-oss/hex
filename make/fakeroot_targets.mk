# HEX SDK

# Make sure tools are installed into the fakeroot before we build
BUILD += $(FAKEROOT_DIR) $(FAKEROOT_TOOLS)
DISTCLEAN += $(FAKEROOT_DIR)

$(FAKEROOT_DIR):
	$(call RUN_CMD,mkdir -p $@,"  MKDIR   $@")

help::
	@echo "froottools   Install tools into fakeroot"

.PHONY: froottools
froottools:: _reconfigure _froottools_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _froottools_r
_froottools_r:: _froottools
	$(call RECURSE,_froottools_r)

# Non-recursive
.PHONY: _froottools
_froottools:: $(FAKEROOT_DIR) $(FAKEROOT_TOOLS)
	@true

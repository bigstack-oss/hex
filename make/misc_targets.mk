# HEX SDK

#
# Miscellaneous targets
#

help::
	@echo "fixme        Generate list of all \"FIXME\" and \"TODO\" comments in code"

.PHONY: fixme
fixme:
	# FIXME: doesn't work, error 123
	#@find $(TOP_SRCDIR) -name \*.[ch] -o -name \*.cpp -o -name \*.sh -o -name Makefile -o -name \*.mk -type f | xargs grep 'TODO\|FIXME' | grep -v 'TODO_IGNORE\|FIXME_IGNORE'
	@find $(HEX_TOPDIR) -name \*.[ch] -o -name \*.cpp -o -name \*.sh -o -name Makefile -o -name \*.mk -type f | xargs grep 'TODO\|FIXME' | grep -v 'TODO_IGNORE\|FIXME_IGNORE'
	@find $(HEX_SCRIPTSDIR) -type f | xargs grep 'TODO\|FIXME' | grep -v 'TODO_IGNORE\|FIXME_IGNORE'

#
# Shell variables for convenience: use "eval `make devenv`"
#

# Export important variables
# Only export variable if it is non-empty
define HEX_DO_EXPORT_VARS
ifneq ($(1),)
export $(1)
endif
endef
$(foreach var,$(HEX_EXPORT_VARS),$(eval $(call HEX_DO_EXPORT_VARS,$(var))))

help::
	@echo "devenv       Generate shell variables used by make (non-recursive)"
	@echo "             Use with \"eval \`make devenv\`\""

.PHONY: devenv
devenv:
	@for V in $(HEX_EXPORT_VARS); do eval echo $$V=\$$$$V ; done

# For testing...
.PHONY: nothing
nothing::
	$(call RECURSE,nothing)

# showdirs
help::
	@echo "showdirs     Print out all directories"

.PHONY: showdirs
showdirs:: _reconfigure _showdirs_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _showdirs_r
_showdirs_r:: _showdirs
	$(call RECURSE,_showdirs_r)

# Non-recursive
.PHONY: _showdirs
_showdirs::
	@echo $(BLDDIR)



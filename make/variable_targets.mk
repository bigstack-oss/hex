# HEX SDK

# based on pass-in variable value to generate variable-target- target
define VARIABLE_TARGET
variable-target-$(shell echo $($(1)) | md5sum | cut -d ' ' -f 1):
	$(Q)echo $($(1)) > $$@
endef
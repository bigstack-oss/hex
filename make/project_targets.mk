# HEX SDK
        
# Make sure these directories get created before we try to build
BUILD := $(PROJ_LIBDIR) $(PROJ_IMGDIR) $(BUILD)
LD_LIBRARY_PATH := $(PROJ_LIBDIR):$(LD_LIBRARY_PATH)
HEX_EXPORT_VARS += LD_LIBRARY_PATH
DISTCLEAN += $(PROJ_LIBDIR) $(PROJ_IMGDIR)
$(PROJ_LIBDIR) $(PROJ_IMGDIR):
	$(call RUN_CMD,mkdir -p $@,"  MKDIR   $@")


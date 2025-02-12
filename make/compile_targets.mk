# HEX SDK

# Default compile/link rules

.SUFFIXES:
.SUFFIXES: .c .cpp .sh .o .xp

# Look for these files in the source directory
vpath %.c $(HEX_SRCDIR)
vpath %.cpp $(HEX_SRCDIR)
vpath %.h $(HEX_SRCDIR)
vpath %.sh $(HEX_SRCDIR)
vpath %.xp $(HEX_SRCDIR)
vpath %.py $(HEX_SRCDIR)

vpath %.c $(SRCDIR)
vpath %.cpp $(SRCDIR)
vpath %.h $(SRCDIR)
vpath %.sh $(SRCDIR)
vpath %.xp $(SRCDIR)
vpath %.py $(SRCDIR)

# Look for hex modules in the following locations
vpath %.o $(HEX_MODDIR)
ifneq ($(PROJ_MODDIR),)
# Look for project modules in the following locations
vpath %.o $(PROJ_MODDIR)
endif

define FIX_DEPEND
@cp $1.d $1.tmp
@sed -e 's%$1.o:%$1.d:%' < $1.d >> $1.tmp
@sed -e 's%^[^:]*: *%%' -e 's% *\\$$%%' -e 's%^ *%%' -e '/^$$/d' -e 's%$$%:%' < $1.d >> $1.tmp
@mv -f $1.tmp $1.d
endef

# COMPILE.x LINK.x OUTPUT_OPTION are implicit rules of Makefile. Using 'Make -np' to check\
# ex. COMPILE.c   = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
#     COMPILE.cpp = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

# Nothing to do, dependencies are generated during compilation
%.d: %.c
	@true

%.d: %.cpp
	@true

%: %.o
	$(call RUN_CMD,$(LINK.o) $^ $(LIBS) $(LDLIBS) -o $@,"  LD      $@")

%: %.c
	$(call RUN_CMD,$(LINK.c) $^ $(LIBS) $(LDLIBS) -o $@,"  LD      $@")
	$(call FIX_DEPEND,$*)

%.i: %.c
	$(call RUN_CMD,$(COMPILE.c) $(OUTPUT_OPTION) -E $< > $@,"  CC      $@")
	$(call FIX_DEPEND,$*)

%.s: %.c
	$(call RUN_CMD,$(COMPILE.c) $(OUTPUT_OPTION) -S $<,"  CC      $@")
	$(call FIX_DEPEND,$*)

%.o: %.c
	$(call RUN_CMD,$(COMPILE.c) $(OUTPUT_OPTION) $<,"  CC      $@")
	$(call FIX_DEPEND,$*)

%: %.cpp
	$(call RUN_CMD,$(LINK.cpp) $^ $(LIBS) $(LDLIBS) -o $@,"  LD      $@")
	$(call FIX_DEPEND,$*)

%.i: %.cpp
	$(call RUN_CMD,$(COMPILE.cpp) $(OUTPUT_OPTION) -E $< > $@,"  CXX     $@")
	$(call FIX_DEPEND,$*)

%.s: %.cpp
	$(call RUN_CMD,$(COMPILE.cpp) $(OUTPUT_OPTION) -S $<,"  CXX     $@")
	$(call FIX_DEPEND,$*)

%.o: %.cpp
	$(call RUN_CMD,$(COMPILE.cpp) $(OUTPUT_OPTION) $<,"  CXX     $@")
	$(call FIX_DEPEND,$*)

%: %.sh
	$(call RUN_CMD,cat $< >$@ && chmod a+x $@,"  GEN     $@")

%: %.xp
	$(call RUN_CMD,cat $< >$@ ,"  GEN     $@")

(%): %
	$(call RUN_CMD,$(AR) $(ARFLAGS) $@ $< $(QEND),"  AR      $$(basename $@)($<)")

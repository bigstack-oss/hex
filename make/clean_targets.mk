# HEX SDK

BUILDCLEAN += *.d *.o *.i *.s *.a *.so variable-target-*

PKGCLEAN += *.cgz *.size *.img

TESTCLEAN += *.fifo
TESTCLEAN += test*.pkg test*.log test*.in test*.out test*.img test_*
TESTCLEAN += *.test
TESTCLEAN += *.testlog *.valgrind.log
TESTCLEAN += test_*.bin test*.tgz test*.pcap log_test*

CLEAN += tmp.* core.*

#
# Clean targets 
#     - pkgclean
#     - buildclean
#     - clean = clean + testclean + pkgclean + buildclean
#     - distclean = distclean + clean
#

help::
	@echo "testclean    Remove all test output"

.PHONY: testclean
testclean:: _reconfigure _testclean_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _testclean_r
_testclean_r:: _testclean
	$(call RECURSE,_testclean_r)

# Non-recursive
.PHONY: _testclean
_testclean:: $(TESTCLEAN_TARGETS)
ifneq ($(THDD_PTH),)
	@ps awwx | grep qemu-kvm | grep "$(THDD_PTH:/=)" | grep -v grep | awk '{print $$1}' | xargs -i kill {}
	@mount | grep tmpfs | grep -o "$(THDD_PTH:/=)" | xargs -i umount {}
	@rm -rf $(THDD_PTH)
endif
	$(if $(wildcard $(TESTCLEAN)),$(call RUN_CMD,$(RM) -r $(TESTCLEAN),"  CLEAN   $(wildcard $(TESTCLEAN))"),@true)

help::
	@echo "pkgclean     Remove all packaged output (rootfs, initramfs, usb, iso, ...)"
	@echo "             Does not remove objects, libraries, and executables"

.PHONY: pkgclean
pkgclean:: _reconfigure _pkgclean_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _pkgclean_r
_pkgclean_r:: _pkgclean
	$(call RECURSE,_pkgclean_r)

# Non-recursive
.PHONY: _pkgclean
_pkgclean:: $(PKGCLEAN_TARGETS)
	$(if $(wildcard $(PKGCLEAN)),$(call RUN_CMD,$(RM) -r $(PKGCLEAN),"  CLEAN   $(wildcard $(PKGCLEAN))"),@true)

help::
	@echo "buildclean   Remove all objects, libraries, and executables"

.PHONY: buildclean
buildclean:: _reconfigure _buildclean_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _buildclean_r
_buildclean_r:: _buildclean
	$(call RECURSE,_buildclean_r)

# Non-recursive
.PHONY: _buildclean
_buildclean:: $(BUILDCLEAN_TARGETS)
	$(if $(wildcard $(BUILDCLEAN)),$(call RUN_CMD,$(RM) -r $(BUILDCLEAN),"  CLEAN   $(wildcard $(BUILDCLEAN))"),@true)

help::
	@echo "clean        Remove all build, test, and packaging output"
	@echo "             Same as \"covclean buildclean pkgclean testclean beamclean\""

.PHONY: clean
clean:: _reconfigure _clean_r
	@true

# Recursive-helper to avoid running reconfigure/covclean more than once
.PHONY: _clean_r
_clean_r:: _clean
	$(call RECURSE,_clean_r)

# Non-recursive
.PHONY: _clean
_clean:: _buildclean _pkgclean _testclean $(CLEAN_TARGETS)
	$(if $(wildcard $(CLEAN)),$(call RUN_CMD,$(RM) -r $(CLEAN),"  CLEAN   $(wildcard $(CLEAN))"),@true)

help::
	@echo "distclean    Remove all redistributable output (implies clean)"

.PHONY: distclean
distclean:: _reconfigure _distclean_r
	@true

# Recursive-helper to avoid running reconfigure/covclean more than once
.PHONY: _distclean_r
_distclean_r:: _distclean
	$(call RECURSE,_distclean_r)

# Non-recursive
.PHONY: _distclean
_distclean:: _clean $(DISTCLEAN_TARGETS)
	$(if $(wildcard $(DISTCLEAN)),$(call RUN_CMD,$(RM) -r $(DISTCLEAN),"  CLEAN   $(wildcard $(DISTCLEAN))"),@true)


# HEX SDK

#
# Main build targets
#

help::
	@echo "build_cpp    Preprocess all C/C++ sources to .i files"

.PHONY: build_cpp
build_cpp:: _reconfigure _build_cpp_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _build_cpp_r
_build_cpp_r:: _build_cpp
	$(call RECURSE,_build_cpp_r)

# Non-recursive
.PHONY: _build_cpp
_build_cpp:: $(BUILD_CPP)
	@true

help::
	@echo "build_asm    Compile but do not assemble all C/C++ sources to .s files"

.PHONY: build_asm
build_asm:: _reconfigure _build_asm_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _build_asm_r
_build_asm_r:: _build_asm
	$(call RECURSE,_build_asm_r)

# Non-recursive
.PHONY: _build_asm
_build_asm:: $(BUILD_ASM)
	@true

help::
	@echo "build        Compile and link all objects, libraries, and programs"

.PHONY: build
build:: _reconfigure _build_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _build_r
_build_r:: _build
	$(call RECURSE,_build_r)

# Non-recursive
.PHONY: _build
_build:: $(BUILD)
	@true

help::
	@echo "all          Build all distributable packages"
	@echo "             (implies build)"

.PHONY: all
all:: _reconfigure _all_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _all_r
_all_r:: _all
	$(call RECURSE,_all_r)

# Non-recursive
.PHONY: _all
_all:: _build $(ALL)
	@true

help::
	@echo "full         Build extra packages not built by \"all\" (e.g. usb, iso, pxe)"
	@echo "             (implies all)"

.PHONY: full
full:: _reconfigure _full_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _full_r
_full_r:: _full
	$(call RECURSE,_full_r)

# Non-recursive
.PHONY: _full
_full:: _all $(FULL)
	@true

help::
	@echo "testbuild    Compile and link all test programs (implies build)"
	@echo "             Does not create test packages or run tests"

.PHONY: testbuild
testbuild:: _reconfigure _testbuild_r
	@true

# Recursive-helper to avoid running reconfigure more than once
.PHONY: _testbuild_r
_testbuild_r:: _testbuild
	$(call RECURSE,_testbuild_r)

# Non-recursive
.PHONY: _testbuild
_testbuild:: _build $(TESTBUILD)
	@true


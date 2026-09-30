# HEX SDK

#
# Sign project deliverables with cosign
#
# Every artifact that lands in $(PROJ_SHIPDIR) -- pkg, pkg iso, iso, usb img, pxe bundle, pxe
# server img/iso, test hotfixes, SBOM and vulnerability report -- gets a sigstore bundle next to
# it, named <artifact>.sigstore.json. The pkg's split parts (<release>_N.pkg: makeppu's separate
# ext4 images of the rootfs, one per large top-level tree) are deliberately left unsigned. They
# are not slices of the pkg, so its bundle says nothing about them. The public key ships as
# <release>_pub.key. A consumer checks any of them with:
#
#   cosign verify-blob --key <release>_pub.key --bundle <artifact>.sigstore.json <artifact>
#
# (add --insecure-ignore-tlog if the build ran with PROJ_COSIGN_TLOG=0).
#
# Each artifact's own build recipe signs it with $(call PROJ_COSIGN,<files>), right after the
# artifact is written. Signing at the end of `make full` instead would leave `make iso`, `make
# usb`, ... producing unsigned images, and a stale bundle from an earlier build next to a fresh
# image. The recipes already delete <artifact>* before regenerating, which takes the old bundle
# with it.
#
# This file is included before the per-artifact makefiles, so the variables below are defined
# when their rules are read; with PROJ_BUILD_SIGN unset, PROJ_COSIGN expands to a no-op.
#

# See projsbom.mk
GITHUB_DL_BASE ?= https://github.com

# Pinned; see projsbom.mk for why.
COSIGN_VER := 3.1.3
COSIGN_RPM := $(GITHUB_DL_BASE)/sigstore/cosign/releases/download/v$(COSIGN_VER)/cosign-$(COSIGN_VER)-1.x86_64.rpm

# Signing key: a key file or a KMS URI (awskms://, gcpkms://, hashivault://, ...). Left empty, a
# throwaway key pair is generated in the build directory once and reused for every artifact of
# that build. It proves the artifacts belong together, not who built them -- set this for a
# real release. A password-protected key file reads its password from COSIGN_PASSWORD.
PROJ_COSIGN_KEY ?=

# 1: record each signature in the public Rekor transparency log (cosign's default).
# 0: sign offline, for a build network that cannot reach sigstore.dev.
PROJ_COSIGN_TLOG ?= 1

PROJ_COSIGN_PUB := cosign.pub
PROJ_COSIGN_BUNDLE_EXT := .sigstore.json

ifeq ($(PROJ_COSIGN_KEY),)
_COSIGN_KEY := cosign.key
# The generated key has an empty password; do not let a COSIGN_PASSWORD meant for some other
# key make cosign fail to decrypt it.
_COSIGN_ENV := COSIGN_PASSWORD=
else
_COSIGN_KEY := $(PROJ_COSIGN_KEY)
_COSIGN_ENV :=
endif

ifeq ($(PROJ_COSIGN_TLOG),1)
_COSIGN_SIGN_FLAGS :=
_COSIGN_VERIFY_FLAGS :=
else
_COSIGN_SIGN_FLAGS := --use-signing-config=false --tlog-upload=false
_COSIGN_VERIFY_FLAGS := --insecure-ignore-tlog
endif

help::
	$(Q)echo "sign         Sign all built deliverables in the ship directory (cosign)"

ifeq ($(PROJ_BUILD_SIGN),1)

# $(1): files (shell globs allowed) to sign; one that does not exist is skipped, so a glob that
#       matches nothing is not an error.
# $(2): what to print (optional; defaults to the file names)
# Verified right after signing, so a bundle that would not check out never ships.
define PROJ_COSIGN
$(call RUN_CMD_TIMED, mkdir -p $(PROJ_SHIPDIR) ; cp -f $(PROJ_COSIGN_PUB) $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_pub.key ; for F in $(1) ; do [ -f "$$F" ] || continue ; $(_COSIGN_ENV) cosign sign-blob --yes --key $(_COSIGN_KEY) $(_COSIGN_SIGN_FLAGS) --bundle "$$F$(PROJ_COSIGN_BUNDLE_EXT)" "$$F" ; cosign verify-blob --key $(PROJ_COSIGN_PUB) $(_COSIGN_VERIFY_FLAGS) --bundle "$$F$(PROJ_COSIGN_BUNDLE_EXT)" "$$F" ; chmod 0644 "$$F$(PROJ_COSIGN_BUNDLE_EXT)" ; done,"  SIGN    $(if $(2),$(2),$(notdir $(1)))")
endef

# A configured key is re-read every time: the public key must follow PROJ_COSIGN_KEY, not
# whichever key an earlier build in this directory used.
ifneq ($(PROJ_COSIGN_KEY),)
.PHONY: $(PROJ_COSIGN_PUB)
endif

$(PROJ_COSIGN_PUB):
	$(call RUN_CMD_TIMED, command -v cosign >/dev/null && cosign version 2>/dev/null | grep -q "v$(COSIGN_VER)$$" || dnf install -y $(COSIGN_RPM),"  DNF     cosign")
ifeq ($(PROJ_COSIGN_KEY),)
	$(call RUN_CMD_TIMED, rm -f cosign.key cosign.pub ; COSIGN_PASSWORD= cosign generate-key-pair,"  GEN     cosign keypair")
else
	$(call RUN_CMD_TIMED, rm -f $@ ; cosign public-key --key $(PROJ_COSIGN_KEY) --outfile $@,"  GEN     $@")
endif

PKGCLEAN += cosign.key $(PROJ_COSIGN_PUB)

# Re-sign whatever is currently in the ship directory, e.g. after a build without signing, or to
# sign with a release key an image that was built with the throwaway one. A pkg's split part is
# recognised by what it is a part of -- <X>_N.pkg next to <X>.pkg -- not by a numeric suffix: the
# release name itself can end in _<digits> when the build description is an all-digit hash.
.PHONY: sign
sign: $(PROJ_COSIGN_PUB)
	$(call PROJ_COSIGN,$$(find $(PROJ_SHIPDIR) -maxdepth 1 -type f ! -name "*$(PROJ_COSIGN_BUNDLE_EXT)" ! -name "*.md5" ! -name "*.sha256" ! -name "*_pub.key" | sort | while read F ; do [ "$${F%.pkg}" != "$$F" ] && [ -f "$${F%_*}.pkg" ] && continue ; echo "$$F" ; done),ship directory)

else

define PROJ_COSIGN
@true
endef

.PHONY: sign
sign:
	@echo "Signing is disabled (PROJ_BUILD_SIGN != 1)" >&2 ; exit 1

endif

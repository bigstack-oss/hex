# HEX SDK

#
# Sign project deliverables with cosign
#
# Signing is split between the build and the publish, because keyless signing needs an OIDC
# token that lives for minutes while a build takes hours:
#
#   build    `make full` writes <release>_SHA256SUMS: sha256sum of every deliverable in
#            $(PROJ_SHIPDIR). This is the only step that reads the images, and it needs no
#            token or key.
#   publish  `make sign`    signs the manifest -> <release>_SHA256SUMS.sigstore.json
#            `make attest`  attests the package-level SBOM to every image that contains the
#                           rootfs -> <image>.sbom.sigstore.json
#            `make verify`  checks all of it, including that every file still matches the
#                           manifest, before anything is published
#
# sign and attest never read an image: attest-blob takes the digest from the manifest (--hash).
# Together they are one cosign call per image plus one for the manifest, done in seconds, so a
# token handed to the publish job only has to outlive that.
#
# The pkg's split parts (<release>_N.pkg: makeppu's separate ext4 images of the rootfs, one per
# large top-level tree) are deliberately left out of the manifest. They are not slices of the
# pkg, so nothing here says anything about them.
#
# Three ways to sign, from weakest to strongest:
#
#   default           a throwaway key pair generated per build directory. The public key ships
#                     as <release>_pub.key, next to what it signs, so it proves the artifacts
#                     belong together and nothing about who built them. For testing.
#   PROJ_COSIGN_KEY   a key you hold: a key file or a KMS URI. <release>_pub.key is its public
#                     key; publish it somewhere an attacker who can replace the images cannot.
#   PROJ_COSIGN_KEYLESS=1
#                     sigstore keyless: each signature gets a short-lived Fulcio certificate for
#                     the OIDC identity PROJ_COSIGN_IDENTITY, and the signing event is recorded
#                     in the public Rekor log. There is no key to hold or leak; the trust anchor
#                     is the identity, which ships as <release>_cosign_identity.txt and should be
#                     published out of band too. Every signature is a permanent public record
#                     naming that identity.
#
# A consumer checks a release with (keyless: --certificate-identity <identity>
# --certificate-oidc-issuer <issuer> in place of --key; add --insecure-ignore-tlog if it was
# signed with PROJ_COSIGN_TLOG=0):
#
#   cosign verify-blob --key <release>_pub.key \
#          --bundle <release>_SHA256SUMS.sigstore.json <release>_SHA256SUMS
#   sha256sum -c --ignore-missing <release>_SHA256SUMS
#
# and what an image contains with:
#
#   cosign verify-blob-attestation --key <release>_pub.key --type cyclonedx \
#          --bundle <image>.sbom.sigstore.json \
#          --digest $(sha256sum < <image> | cut -d' ' -f1) --digestAlg sha256
#
# --digest rather than the image itself: given the file, cosign 3.1.3 refuses one over 128 MiB
# (COSIGN_MAX_ATTACHMENT_SIZE), and every image is ~10 GB.
#
# The attested images are the ones that contain the CubeCOS rootfs: pkg, pkg iso, iso, usb img,
# pxe bundle, pxe server img. Not the pxe server iso, which is built without the pkg and strips
# it from the pxe bundle it embeds. The predicate is <release>_sbom_packages.json, the
# package-level SBOM makesbompackages derives from the full one: the full SBOM is 128 MB, and
# public Rekor refuses a request over ~24 MiB.
#

# See projsbom.mk
GITHUB_DL_BASE ?= https://github.com

# Pinned; see projsbom.mk for why.
COSIGN_VER := 3.1.3
COSIGN_RPM := $(GITHUB_DL_BASE)/sigstore/cosign/releases/download/v$(COSIGN_VER)/cosign-$(COSIGN_VER)-1.x86_64.rpm

# Signing key: a key file or a KMS URI (awskms://, gcpkms://, hashivault://, ...). Left empty, a
# throwaway key pair is generated in the build directory once and reused. It proves the
# artifacts belong together, not who built them. A password-protected key file reads its
# password from COSIGN_PASSWORD.
PROJ_COSIGN_KEY ?=

# 1: sign keyless instead of with a key; see above.
PROJ_COSIGN_KEYLESS ?= 0
# The signer's identity as it appears in the Fulcio certificate (an email address for a user
# account, a workflow URI for CI) and the OIDC issuer that vouches for it, e.g.
# https://github.com/login/oauth. Both are what a consumer pins when verifying, and the build
# verifies each signature against them.
PROJ_COSIGN_IDENTITY ?=
PROJ_COSIGN_OIDC_ISSUER ?=
# A command that prints the OIDC identity token for that identity, e.g. `cat <file>`. It runs
# right before each signature. Left empty, cosign uses an ambient provider it can detect
# (GitHub Actions, and the others `cosign env` lists) -- and without one, falls back to an
# interactive browser login, which a CI job cannot complete.
PROJ_COSIGN_ID_TOKEN_CMD ?=

# 1: record each signature in the public Rekor transparency log (cosign's default).
# 0: sign offline, for testing on a network that cannot reach sigstore.dev.
PROJ_COSIGN_TLOG ?= 1

# 1: verify each signature and attestation as soon as it is made.
# 0: leave that to `make verify`, which checks the same and more. For a keyless publish: the
#    token from sigstore's login lives 60 s, and every cosign call that signs must start inside
#    them, so nothing that does not need the token should run in between.
PROJ_COSIGN_SELFCHECK ?= 1

PROJ_COSIGN_PUB := cosign.pub
PROJ_COSIGN_BUNDLE_EXT := .sigstore.json
PROJ_COSIGN_ATTEST_EXT := .sbom$(PROJ_COSIGN_BUNDLE_EXT)
# Symlink to the manifest in $(PROJ_SHIPDIR), like the other proj.* targets
PROJ_SHA256SUMS := proj.sha256sums

# Largest predicate attested while signatures go to public Rekor: its ~24 MiB request cap, less
# the base64 encoding (x4/3) and the envelope around it.
PROJ_COSIGN_ATTEST_MAX := 16777216

ifeq ($(PROJ_COSIGN_KEYLESS),1)
# What sign/attest/verify need first: cosign installed, and the identity recorded for publishing.
PROJ_COSIGN_SETUP := cosign.identity
_COSIGN_SIGN_ID = $(if $(PROJ_COSIGN_ID_TOKEN_CMD),--identity-token "$$T")
_COSIGN_VERIFY_ID := --certificate-identity '$(PROJ_COSIGN_IDENTITY)' --certificate-oidc-issuer '$(PROJ_COSIGN_OIDC_ISSUER)'
_COSIGN_ENV :=
# Shell run before and after each cosign call that signs: fetch the token into a private temp
# file, then remove it. cosign takes a path for --identity-token, so the token never appears on
# a command line.
_COSIGN_TOKEN_GET = $(if $(PROJ_COSIGN_ID_TOKEN_CMD),T=$$(mktemp) && chmod 0600 $$T && ( $(PROJ_COSIGN_ID_TOKEN_CMD) ) > $$T || { rm -f $$T ; exit 1 ; } ;)
_COSIGN_TOKEN_PUT = $(if $(PROJ_COSIGN_ID_TOKEN_CMD),rm -f $$T ;)
_COSIGN_PUBLISH = cp -f $(PROJ_COSIGN_SETUP) $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_cosign_identity.txt
else
PROJ_COSIGN_SETUP := $(PROJ_COSIGN_PUB)
ifeq ($(PROJ_COSIGN_KEY),)
_COSIGN_KEY := cosign.key
# The generated key has an empty password; do not let a COSIGN_PASSWORD meant for some other
# key make cosign fail to decrypt it.
_COSIGN_ENV := COSIGN_PASSWORD=
else
_COSIGN_KEY := $(PROJ_COSIGN_KEY)
_COSIGN_ENV :=
endif
_COSIGN_SIGN_ID := --key $(_COSIGN_KEY)
_COSIGN_VERIFY_ID := --key $(PROJ_COSIGN_PUB)
_COSIGN_TOKEN_GET :=
_COSIGN_TOKEN_PUT :=
_COSIGN_PUBLISH = cp -f $(PROJ_COSIGN_PUB) $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_pub.key
endif

ifeq ($(PROJ_COSIGN_TLOG),1)
_COSIGN_SIGN_FLAGS :=
_COSIGN_VERIFY_FLAGS :=
else
_COSIGN_SIGN_FLAGS := --use-signing-config=false --tlog-upload=false
_COSIGN_VERIFY_FLAGS := --insecure-ignore-tlog
endif

# Shell names for the manifest and its signature in $(PROJ_SHIPDIR), for use in recipes
_SUMS = $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_SHA256SUMS
_SUMS_BUNDLE = $(_SUMS)$(PROJ_COSIGN_BUNDLE_EXT)

help::
	$(Q)echo "sums         Write the SHA256SUMS manifest of the ship directory"
	$(Q)echo "sign         Sign the SHA256SUMS manifest (cosign)"
	$(Q)echo "attest       Attest the SBOM to every image that contains the rootfs (cosign)"
	$(Q)echo "verify       Check the manifest signature, the attestations and every file's digest"

ifeq ($(PROJ_BUILD_SIGN),1)

ifeq ($(PROJ_COSIGN_KEYLESS),1)
ifneq ($(PROJ_COSIGN_KEY),)
$(error PROJ_COSIGN_KEYLESS=1 and PROJ_COSIGN_KEY are mutually exclusive)
endif
ifeq ($(PROJ_COSIGN_IDENTITY),)
$(error PROJ_COSIGN_KEYLESS=1 needs PROJ_COSIGN_IDENTITY, the identity a consumer verifies against)
endif
ifeq ($(PROJ_COSIGN_OIDC_ISSUER),)
$(error PROJ_COSIGN_KEYLESS=1 needs PROJ_COSIGN_OIDC_ISSUER)
endif
# A Fulcio certificate is valid for minutes; only the log entry made while it was valid lets a
# signature verify afterwards.
ifneq ($(PROJ_COSIGN_TLOG),1)
$(error PROJ_COSIGN_KEYLESS=1 needs PROJ_COSIGN_TLOG=1)
endif
endif

# Every step below ends in `|| exit 1` (or runs under pipefail) because nothing else would stop
# it: RUN_CMD_TIMED runs its command as `if ( set -e ; ... )`, and bash ignores set -e inside a
# subshell whose status an `if` tests, so only the last command's status would count. (With
# VERBOSE set, RUN_CMD_TIMED is the bare command, which has no set -e at all.)

# The manifest. Its prerequisites -- every deliverable -- are added in hex_sdk.mk once every
# artifact makefile has been read. It lists every file in the ship directory except checksums,
# signatures, the verification material and the pkg's split parts, by base name so that
# `sha256sum -c` works from wherever the files are copied to. A split part is recognised by what
# it is a part of -- <X>_N.pkg next to <X>.pkg -- not by a numeric suffix: the release name
# itself ends in _<digits> when the build description is an all-digit hash. Regenerating it
# drops the signature and the attestations, which covered the old digests.
.PHONY: sums
sums: $(PROJ_SHA256SUMS)
	@true

$(PROJ_SHA256SUMS):
	$(call RUN_CMD_TIMED, set -o pipefail ; R=$$(readlink $(PROJ_RELEASE)) && L=$(CURDIR)/sha256sums.list && T=$(CURDIR)/sha256sums.tmp && cd $(PROJ_SHIPDIR) || exit 1 ; rm -f $(PROJ_NAME)_$(PROJ_VERSION)*_SHA256SUMS* *$(PROJ_COSIGN_ATTEST_EXT) || exit 1 ; find . -maxdepth 1 -type f ! -name "*.md5" ! -name "*.sha256" ! -name "*$(PROJ_COSIGN_BUNDLE_EXT)" ! -name "*_pub.key" ! -name "*_cosign_identity.txt" -printf '%f\n' | sort | while read F ; do [ "$${F%.pkg}" != "$$F" ] && [ -f "$${F%_*}.pkg" ] && continue ; echo "$$F" ; done > $$L || exit 1 ; [ -s $$L ] || { echo "nothing to list in $(PROJ_SHIPDIR)" ; exit 1 ; } ; xargs -r -d '\n' -P 4 -n 1 sha256sum < $$L | sort -k 2 > $$T || exit 1 ; [ $$(wc -l < $$T) -eq $$(wc -l < $$L) ] || exit 1 ; rm -f $$L ; chmod 0644 $$T && mv -f $$T "$${R}_SHA256SUMS" || exit 1,"  GEN     SHA256SUMS")
	$(Q)ln -sf $(_SUMS) $@

PKGCLEAN += $(PROJ_SHA256SUMS) sha256sums.list sha256sums.tmp

# A configured key or identity is re-read every time: what ships must follow the current
# settings, not whichever an earlier run in this directory used.
ifneq ($(PROJ_COSIGN_KEY)$(filter 1,$(PROJ_COSIGN_KEYLESS)),)
.PHONY: $(PROJ_COSIGN_SETUP)
endif

$(PROJ_COSIGN_SETUP):
	$(call RUN_CMD_TIMED, command -v cosign >/dev/null && cosign version 2>/dev/null | grep -q "v$(COSIGN_VER)$$" || dnf install -y $(COSIGN_RPM),"  DNF     cosign")
ifeq ($(PROJ_COSIGN_KEYLESS),1)
	$(call RUN_CMD_TIMED, printf 'certificate-identity: %s\ncertificate-oidc-issuer: %s\n' '$(PROJ_COSIGN_IDENTITY)' '$(PROJ_COSIGN_OIDC_ISSUER)' > $@,"  GEN     $@")
else ifeq ($(PROJ_COSIGN_KEY),)
	$(call RUN_CMD_TIMED, rm -f cosign.key cosign.pub ; COSIGN_PASSWORD= cosign generate-key-pair,"  GEN     cosign keypair")
else
	$(call RUN_CMD_TIMED, rm -f $@ ; cosign public-key --key $(PROJ_COSIGN_KEY) --outfile $@,"  GEN     $@")
endif

PKGCLEAN += cosign.key $(PROJ_COSIGN_PUB) cosign.identity

# Refuse to sign or attest a manifest that no longer describes the ship directory. sign and
# attest do not depend on $(PROJ_SHA256SUMS) through make: a stale image would then be rebuilt,
# and an hours-long build is the last thing a publish job holding a token should start. A file
# newer than the manifest is caught here; one whose content changed without its mtime is caught
# by `make verify` before anything is published.
_COSIGN_SUMS_FRESH = S=$(_SUMS) ; [ -f "$$S" ] || { echo "no $$S: run make sums" ; exit 1 ; } ; cut -d' ' -f3- "$$S" | while read F ; do [ -f "$(PROJ_SHIPDIR)/$$F" ] || { echo "$$F is in the manifest but missing" ; exit 1 ; } ; [ "$(PROJ_SHIPDIR)/$$F" -nt "$$S" ] && { echo "$$F is newer than the manifest: run make sums" ; exit 1 ; } ; true ; done || exit 1

# Sign the manifest. Phony: every run signs afresh -- a signature from an earlier run may have
# been made in another mode or by another identity, and this one takes seconds.
.PHONY: sign
sign: $(PROJ_COSIGN_SETUP)
	$(call RUN_CMD_TIMED, $(_COSIGN_SUMS_FRESH) ; $(_COSIGN_PUBLISH) || exit 1 ; rm -f $(_SUMS_BUNDLE) ; $(_COSIGN_TOKEN_GET) $(_COSIGN_ENV) cosign sign-blob --yes $(_COSIGN_SIGN_ID) $(_COSIGN_SIGN_FLAGS) --bundle $(_SUMS_BUNDLE) $(_SUMS) || { $(_COSIGN_TOKEN_PUT) rm -f $(_SUMS_BUNDLE) ; exit 1 ; } ; $(_COSIGN_TOKEN_PUT) $(if $(filter 1,$(PROJ_COSIGN_SELFCHECK)),cosign verify-blob $(_COSIGN_VERIFY_ID) $(_COSIGN_VERIFY_FLAGS) --bundle $(_SUMS_BUNDLE) $(_SUMS) || { rm -f $(_SUMS_BUNDLE) ; exit 1 ; } ;) chmod 0644 $(_SUMS_BUNDLE) || exit 1,"  SIGN    SHA256SUMS")

# Attest the package-level SBOM to each image, by the digest the manifest recorded for it.
# All images at once: each attest-blob uploads the ~12 MB predicate to Rekor, ~9 s apiece, and one
# after the other the six took 52 of a keyless token's 60 s (publish #40). Each runs in its own
# subshell with its own copy of the token; any one failing fails the step, naming the image and
# cosign's reason. Phony for the same reason as sign. Each attestation is verified as it is made (unless
# PROJ_COSIGN_SELFCHECK=0), and its
# predicate type is checked from the envelope itself: cosign 3.0.5 given the file accepted a
# CycloneDX attestation for --type spdxjson (3.1.3 given --digest rejects it), so the build does
# not depend on which. attest-blob prints the whole envelope, SBOM and all, to stdout.
.PHONY: attest
attest: $(PROJ_COSIGN_SETUP)
	$(call RUN_CMD_TIMED, $(if $(PROJ_SBOM_ATTESTED),,echo "no SBOM in this build (PROJ_BUILD_SBOM != 1)" ; exit 1 ;) $(_COSIGN_SUMS_FRESH) ; P=$(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_sbom_packages.json ; grep -q "  $$(basename $$P)$$" $(_SUMS) || { echo "$$P is not in the manifest" ; exit 1 ; } ; $(if $(filter 1,$(PROJ_COSIGN_TLOG)),[ $$(stat -c %s $$P) -le $(PROJ_COSIGN_ATTEST_MAX) ] || { echo "$$P is $$(stat -c %s $$P) bytes: over PROJ_COSIGN_ATTEST_MAX ($(PROJ_COSIGN_ATTEST_MAX)) and too big for public Rekor" ; exit 1 ; } ;) $(_COSIGN_PUBLISH) || exit 1 ; D=$$(mktemp -d) || exit 1 ; pids="" ; for L in $(PROJ_SBOM_ATTESTED) ; do ( N=$$(basename $$(readlink -f $$L)) ; B=$(PROJ_SHIPDIR)/$$N$(PROJ_COSIGN_ATTEST_EXT) ; H=$$(awk -v n="$$N" '$$2 == n { print $$1 }' $(_SUMS)) ; [ -n "$$H" ] || { echo "$$N is not in the manifest" ; exit 1 ; } ; rm -f $$B ; $(_COSIGN_TOKEN_GET) $(_COSIGN_ENV) cosign attest-blob --yes $(_COSIGN_SIGN_ID) $(_COSIGN_SIGN_FLAGS) --predicate $$P --type cyclonedx --hash $$H --bundle $$B $(PROJ_SHIPDIR)/$$N >/dev/null 2>$$D/$$N.log || { $(_COSIGN_TOKEN_PUT) rm -f $$B ; echo "attesting $$N failed: $$(tail -n 1 $$D/$$N.log)" ; exit 1 ; } ; $(_COSIGN_TOKEN_PUT) $(if $(filter 1,$(PROJ_COSIGN_SELFCHECK)),cosign verify-blob-attestation $(_COSIGN_VERIFY_ID) $(_COSIGN_VERIFY_FLAGS) --type cyclonedx --bundle $$B --digest $$H --digestAlg sha256 >/dev/null 2>$$D/$$N.vlog || { rm -f $$B ; echo "verifying the attestation of $$N failed: $$(tail -n 1 $$D/$$N.vlog)" ; exit 1 ; } ;) grep -o '"payload": *"[^"]*"' $$B | cut -d'"' -f4 | base64 -d | grep -q '"predicateType": *"https://cyclonedx.org/bom"' || { echo "$$B: predicate type is not https://cyclonedx.org/bom" ; rm -f $$B ; exit 1 ; } ; chmod 0644 $$B || exit 1 ) & pids="$$pids $$!" ; done ; bad=0 ; for p in $$pids ; do wait $$p || bad=$$(( bad + 1 )) ; done ; rm -rf $$D ; [ $$bad -eq 0 ] || { echo "$$bad of $(words $(PROJ_SBOM_ATTESTED)) attestations failed" ; exit 1 ; },"  ATTEST  sbom -> $(PROJ_SBOM_ATTESTED)")

# What a publish job runs last, and what a consumer would: the manifest's signature, each
# attestation against the digest the manifest records, then every listed file against the
# manifest. The last step reads every image once; it is the only check here that catches an
# image changed in place.
.PHONY: verify
verify: $(PROJ_COSIGN_SETUP)
	$(call RUN_CMD_TIMED, [ -f $(_SUMS_BUNDLE) ] || { echo "$(_SUMS) is not signed: run make sign" ; exit 1 ; } ; cosign verify-blob $(_COSIGN_VERIFY_ID) $(_COSIGN_VERIFY_FLAGS) --bundle $(_SUMS_BUNDLE) $(_SUMS) || exit 1,"  VERIFY  SHA256SUMS signature")
	$(call RUN_CMD_TIMED, for L in $(PROJ_SBOM_ATTESTED) ; do N=$$(basename $$(readlink -f $$L)) ; B=$(PROJ_SHIPDIR)/$$N$(PROJ_COSIGN_ATTEST_EXT) ; H=$$(awk -v n="$$N" '$$2 == n { print $$1 }' $(_SUMS)) ; [ -n "$$H" ] && [ -f $$B ] || { echo "$$N: no manifest entry or no attestation" ; exit 1 ; } ; cosign verify-blob-attestation $(_COSIGN_VERIFY_ID) $(_COSIGN_VERIFY_FLAGS) --type cyclonedx --bundle $$B --digest $$H --digestAlg sha256 || exit 1 ; grep -o '"payload": *"[^"]*"' $$B | cut -d'"' -f4 | base64 -d | grep -q '"predicateType": *"https://cyclonedx.org/bom"' || { echo "$$B: predicate type is not https://cyclonedx.org/bom" ; exit 1 ; } ; done,"  VERIFY  sbom attestations")
	$(call RUN_CMD_TIMED, cd $(PROJ_SHIPDIR) && sha256sum -c --strict --quiet $$(readlink $(CURDIR)/$(PROJ_RELEASE))_SHA256SUMS || exit 1,"  VERIFY  SHA256SUMS contents")

else

.PHONY: sums sign attest verify
sums sign attest verify:
	@echo "Signing is disabled (PROJ_BUILD_SIGN != 1)" >&2 ; exit 1

endif

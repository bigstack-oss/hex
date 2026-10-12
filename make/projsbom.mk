# HEX SDK

#
# Generte project security artifacts
#

# Host serving GitHub release assets. Public by default; a project whose build network is
# throttled against GitHub's object CDN can point it at an internal mirror instead. cubecos sets
# it in its own project.mk, which is included before this file, so the `?=` here only supplies
# the default for a project that does not.
GITHUB_DL_BASE ?= https://github.com

# Pinned rather than resolved from /releases/latest on every parse. Three reasons:
#
#   - a version that moves on its own cannot be mirrored, and these rpms are ~112 MB (cosign
#     included, pinned in projsign.mk) fetched on every build;
#   - the old form scraped a version out of GitHub's *HTML* (`grep -o "tag_name=.*&amp"`). If
#     that markup ever changes the variable silently becomes empty and the URL 404s, with
#     nothing naming the real cause;
#   - it cost three network round trips at *parse* time of every make invocation, before a
#     single rule ran -- minutes on a throttled link, whatever the target.
#
# Bump these deliberately. How current the scanner is should be a decision someone made, not a
# side effect of what day the build ran.
#
# Each version comes with the SHA-256 of its rpm, and the rpm is installed only if it matches
# (scripts/fetchverified): a pinned version says which release, the digest says these are its
# bytes -- which matters all the more since GITHUB_DL_BASE can point the download at a mirror.
# Bump the digest with the version. Take it from the checksums file the project publishes with
# the release, after checking that file against the project's own release workflow:
#
#   cosign verify-blob --certificate syft_<ver>_checksums.txt.pem \
#       --signature syft_<ver>_checksums.txt.sig \
#       --certificate-identity-regexp '^https://github.com/anchore/syft/' \
#       --certificate-oidc-issuer https://token.actions.githubusercontent.com \
#       syft_<ver>_checksums.txt
#   grep ' syft_<ver>_linux_amd64.rpm$' syft_<ver>_checksums.txt
#
# and the same for grype (anchore/grype). The rpms themselves are not GPG-signed.
SYFT_VER := 1.52.0
SYFT_SHA256 := 32964662e330c3e1b73af99ce0e4aed5ab9f560d0e6f20cdc601761e8d7aa803
GRYPE_VER := 0.119.0
GRYPE_SHA256 := 24872be8acdb53e5ccc16f15e2bb2cf1ca9ed9a33fe9bf0da1aeda3611ef5d54

SYFT_RPM := $(GITHUB_DL_BASE)/anchore/syft/releases/download/v$(SYFT_VER)/syft_$(SYFT_VER)_linux_amd64.rpm
GRYPE_RPM := $(GITHUB_DL_BASE)/anchore/grype/releases/download/v$(GRYPE_VER)/grype_$(GRYPE_VER)_linux_amd64.rpm

# Vulnerability DB the report is scanned against. Either way, <release>_vuln.json records the DB
# it used, under descriptor.db.status: "schemaVersion", "built", and "from", the archive URL with
# its sha256 in the ?checksum= parameter. Nothing is added on top of that.
#
#   - Empty (the default): the newest DB at scan time. grype checks for one at most every 2h, and
#     if the check fails it scans with the DB it has, up to 5 days old; "from" still says which.
#   - An archive URL with ?checksum=sha256:<digest>, as `grype db list` prints them: that DB.
#     grype downloads it once into its own cache directory, refuses it if the digest doesn't
#     match, and scans it without update or age checks, so a DB older than 5 days still works.
#     The checksum is required: a local file or a URL without one is recorded as "manual import"
#     or with no digest, and the report would no longer say which DB it was. To keep a pin usable
#     after grype.anchore.io drops the archive, mirror it and pin the mirror's URL with the same
#     checksum.
#
# Changing GRYPE_DB rescans on the next `make sbom`. To check a shipped release for new CVEs, no
# rebuild is needed: scan its published <release>_sbom.json with a newer DB, passing the same
# --vex documents the release used (listed in the report's descriptor):
#   grype sbom:<release>_sbom.json --output=json > <release>_vuln_<date>.json
GRYPE_DB ?=

ifneq ($(GRYPE_DB),)
GRYPE_DB_SHA256 := $(patsubst checksum=sha256:%,%,$(filter checksum=sha256:%,$(subst ?, ,$(subst &, ,$(GRYPE_DB)))))
ifneq ($(words $(GRYPE_DB_SHA256)),1)
$(error GRYPE_DB must be an archive URL with ?checksum=sha256:<digest>, as `grype db list` prints them: $(GRYPE_DB))
endif
ifneq ($(findstring ',$(GRYPE_DB)),)
$(error GRYPE_DB must not contain a single quote: $(GRYPE_DB))
endif
# One directory per DB, outside the build tree: an unpacked DB is about 3 GB, and `make clean`
# should not cost a re-download. Remove old ones by hand.
GRYPE_DB_DIR := $(HOME)/.cache/grype/pinned/$(GRYPE_DB_SHA256)
GRYPE_DB_ENV := GRYPE_DB_CACHE_DIR=$(GRYPE_DB_DIR) GRYPE_DB_AUTO_UPDATE=false GRYPE_DB_VALIDATE_AGE=false
endif

help::
	$(Q)echo "sbom         Create SBOM (syft) and vulnerability report (grype)"

# Holds the GRYPE_DB the report was last scanned with, rewritten only when it changes, so that a
# changed pin rescans and an unchanged one doesn't.
grype-db.pin: $(if $(subst x$(GRYPE_DB),,x$(file <grype-db.pin)),FORCE_GRYPE_DB_PIN)
	$(Q)printf '%s' '$(GRYPE_DB)' > $@

.PHONY: FORCE_GRYPE_DB_PIN

.PHONY: sbom
sbom: $(PROJ_SBOM)
	$(Q)true

# The SBOM, the package-level SBOM derived from it and the vulnerability report ship like any
# other deliverable: projsign.mk covers them with the signed SHA256SUMS manifest, and attests
# the package-level SBOM to each image (it says why that one, not the full SBOM).
$(PROJ_SBOM): syft-fs-cubecos.cdx.json $(HEX_SCRIPTSDIR)/makesbompackages grype-db.pin
	$(call RUN_CMD_TIMED, mkdir -p $(PROJ_SHIPDIR) ; rm -f $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*_sbom.json* $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*_sbom_packages.json* $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*_vuln.json* $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*_bndl.json,"  RM      old sbom")
ifneq ($(GRYPE_DB),)
	$(call RUN_CMD_TIMED, $(GRYPE_DB_ENV) grype db status --output=json 2>/dev/null | grep -qF '"from": "$(GRYPE_DB)"' || $(GRYPE_DB_ENV) grype db import '$(GRYPE_DB)',"  IMPORT  grype db $(GRYPE_DB_SHA256)")
endif
	$(call RUN_CMD_TIMED, $(GRYPE_DB_ENV) grype sbom:$< --output=json > $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_vuln.json,"  SCAN    vuln")
	$(call RUN_CMD_TIMED, cp -f $< $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_sbom.json,"  COPY    $<")
	$(call RUN_CMD_TIMED, $(SHELL) $(HEX_SCRIPTSDIR)/makesbompackages $< $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_sbom_packages.json,"  GEN     sbom_packages.json")
	$(call RUN_CMD_TIMED, ln -sf $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_sbom.json $@,"  GEN     $@")

syft-fs-cubecos.cdx.json: $(PROJ_BASE_ROOTFS)
	$(call RUN_CMD_TIMED, $(SHELL) $(HEX_SCRIPTSDIR)/fetchverified $(SYFT_RPM) $(SYFT_SHA256) $(notdir $(SYFT_RPM)) && $(SHELL) $(HEX_SCRIPTSDIR)/fetchverified $(GRYPE_RPM) $(GRYPE_SHA256) $(notdir $(GRYPE_RPM)) && dnf install -y ./$(notdir $(SYFT_RPM)) ./$(notdir $(GRYPE_RPM)),"  DNF     syft grype")
	$(call RUN_CMD_TIMED, cd $< && syft --config $(SRCDIR)/syft.yml --source-version $(PROJ_NAME)_$(PROJ_VERSION) ./,"  GEN     $@")

PKGCLEAN += $(PROJ_SBOM) syft-fs-cubecos.cdx.json grype-db.pin
# Downloads: kept by `make clean`, so the next build reuses them once their digest checks out
DISTCLEAN += $(notdir $(SYFT_RPM) $(GRYPE_RPM))

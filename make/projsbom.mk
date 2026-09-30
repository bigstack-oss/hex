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
SYFT_VER := 1.52.0
GRYPE_VER := 0.119.0

SYFT_RPM := $(GITHUB_DL_BASE)/anchore/syft/releases/download/v$(SYFT_VER)/syft_$(SYFT_VER)_linux_amd64.rpm
GRYPE_RPM := $(GITHUB_DL_BASE)/anchore/grype/releases/download/v$(GRYPE_VER)/grype_$(GRYPE_VER)_linux_amd64.rpm

help::
	$(Q)echo "sbom         Create SBOM (syft) and vulnerability report (grype), signed if PROJ_BUILD_SIGN=1"

.PHONY: sbom
sbom: $(PROJ_SBOM)
	$(Q)true

# The SBOM and the vulnerability report are signed like every other deliverable (projsign.mk):
# the copies in the ship directory, since those are what a consumer receives.
$(PROJ_SBOM): syft-fs-cubecos.cdx.json
	$(call RUN_CMD_TIMED, mkdir -p $(PROJ_SHIPDIR) ; rm -f $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*_sbom.json* $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*_vuln.json* $(PROJ_SHIPDIR)/$(PROJ_NAME)_$(PROJ_VERSION)*_bndl.json,"  RM      old sbom")
	$(call RUN_CMD_TIMED, grype sbom:$< --output=json > $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_vuln.json,"  SCAN    vuln")
	$(call RUN_CMD_TIMED, cp -f $< $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_sbom.json,"  COPY    $<")
	$(call PROJ_COSIGN,$(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_sbom.json $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_vuln.json,sbom + vuln)
	$(call RUN_CMD_TIMED, ln -sf $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_sbom.json $@,"  GEN     $@")

syft-fs-cubecos.cdx.json: $(PROJ_BASE_ROOTFS)
	$(call RUN_CMD_TIMED, dnf install -y $(SYFT_RPM) $(GRYPE_RPM),"  DNF     syft grype")
	$(call RUN_CMD_TIMED, cd $< && syft --config $(SRCDIR)/syft.yml --source-version $(PROJ_NAME)_$(PROJ_VERSION) ./,"  GEN     $@")

PKGCLEAN += $(PROJ_SBOM) syft-fs-cubecos.cdx.json

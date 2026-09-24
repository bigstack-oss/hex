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
#   - a version that moves on its own cannot be mirrored, and these three rpms are ~112 MB
#     fetched on every build;
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
COSIGN_VER := 3.1.3

SYFT_RPM := $(GITHUB_DL_BASE)/anchore/syft/releases/download/v$(SYFT_VER)/syft_$(SYFT_VER)_linux_amd64.rpm
GRYPE_RPM := $(GITHUB_DL_BASE)/anchore/grype/releases/download/v$(GRYPE_VER)/grype_$(GRYPE_VER)_linux_amd64.rpm
COSIGN_RPM := $(GITHUB_DL_BASE)/sigstore/cosign/releases/download/v$(COSIGN_VER)/cosign-$(COSIGN_VER)-1.x86_64.rpm

help::
	$(Q)echo "sbom         Create SBOM (syft), vulnerability report (grype) and signature bundle (cosign)"

.PHONY: sbom
sbom: $(PROJ_SBOM)
	$(Q)true

$(PROJ_SBOM): syft-fs-cubecos.cdx.json
	$(call RUN_CMD_TIMED, rm -f cosign.key cosign.pub ; COSIGN_PASSWORD= cosign generate-key-pair,"  GEN     cosign keypair")
	$(call RUN_CMD_TIMED, mkdir -p $(PROJ_SHIPDIR) ; cp -f cosign.pub $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_pub.key,"  COPY    cosign pubkey")
	$(call RUN_CMD_TIMED, COSIGN_PASSWORD= cosign sign-blob --key cosign.key --bundle=$(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_bndl.json $<,"  SIGN    sbom")
	$(call RUN_CMD_TIMED, COSIGN_PASSWORD= cosign verify-blob --key cosign.pub --bundle=$(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_bndl.json $<,"  VERIFY  sbom + bundle")
	$(call RUN_CMD_TIMED, grype sbom:$< --output=json > $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_vuln.json,"  SCAN    vuln")
	$(call RUN_CMD_TIMED, cp -f $< $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_sbom.json,"  COPY    $<")
	$(call RUN_CMD_TIMED, ln -sf $(PROJ_SHIPDIR)/$$(readlink $(PROJ_RELEASE))_sbom.json $@,"  GEN     $@")

syft-fs-cubecos.cdx.json: $(PROJ_BASE_ROOTFS)
	$(call RUN_CMD_TIMED, dnf install -y $(SYFT_RPM) $(GRYPE_RPM) $(COSIGN_RPM),"  DNF     syft grype cosign")
	$(call RUN_CMD_TIMED, cd $< && syft --config $(SRCDIR)/syft.yml --source-version $(PROJ_NAME)_$(PROJ_VERSION) ./,"  GEN     $@")

PKGCLEAN += $(PROJ_SBOM) syft-fs-cubecos.cdx.json cosign.key cosign.pub

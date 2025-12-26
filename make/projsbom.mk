# HEX SDK

#
# Generte project security artifacts
#

SYFT_URL := https://github.com/anchore/syft/releases
SYFT_VER := $(shell curl -sL $(SYFT_URL)/latest | grep -o "tag_name=.*&amp" | cut -d "v" -f2 | cut -d "&" -f1)
SYFT_RPM := $(SYFT_URL)/download/v$(SYFT_VER)/syft_$(SYFT_VER)_linux_amd64.rpm

GRYPE_URL := https://github.com/anchore/grype/releases
GRYPE_VER := $(shell curl -sL $(GRYPE_URL)/latest | grep -o "tag_name=.*&amp" | cut -d "v" -f2 | cut -d "&" -f1)
GRYPE_RPM := $(GRYPE_URL)/download/v$(GRYPE_VER)/grype_$(GRYPE_VER)_linux_amd64.rpm

COSIGN_URL := https://github.com/sigstore/cosign/releases
COSIGN_VER := $(shell curl -sL $(COSIGN_URL)/latest | grep -o "tag_name=.*&amp" | cut -d "v" -f2 | cut -d "&" -f1)
COSIGN_RPM := $(COSIGN_URL)/download/v$(COSIGN_VER)/cosign-$(COSIGN_VER)-1.x86_64.rpm

help::
	$(Q)echo "sbom         Create SBOM (syft), vulnerability report (grype) and signature bundle (cosign)"

.PHONY: sbom
sbom: $(PROJ_SBOM)
	$(Q)true

$(PROJ_SBOM): syft-fs-cubecos.cdx.json
	$(call RUN_CMD_TIMED, rm -f cosign.key cosign.pub ; COSIGN_PASSWORD= cosign generate-key-pair,"  GEN     cosign keypair")
	$(call RUN_CMD_TIMED, mkdir -p $(PROJ_SHIPDIR) ; cp -f cosign.pub  $(shell readlink $(PROJ_SHIPDIR)/$(PROJ_RELEASE))_pub.key,"  COPY    cosign pubkey")
	$(call RUN_CMD_TIMED, COSIGN_PASSWORD= cosign sign-blob --key cosign.key --bundle=$(PROJ_SHIPDIR)/$(shell readlink $(PROJ_RELEASE))_bndl.json $<,"  SIGN    sbom")
	$(call RUN_CMD_TIMED, COSIGN_PASSWORD= cosign verify-blob --key cosign.pub --bundle=$(shell readlink $(PROJ_RELEASE))_bndl.json $<,"  VERIFY  sbom + bundle")
	$(call RUN_CMD_TIMED, grype sbom:$< --output=json > $(PROJ_SHIPDIR)/$(shell readlink $(PROJ_RELEASE))_vuln.json,"  SCAN    vuln")
	$(call RUN_CMD_TIMED, cp -f $< $(PROJ_SHIPDIR)/$(shell readlink $(PROJ_RELEASE))_sbom.json,"  COPY    $<")
	$(call RUN_CMD_TIMED, ln -sf $(PROJ_SHIPDIR)/$(shell readlink $(PROJ_RELEASE))_sbom.json $@,"  GEN     $@")

syft-fs-cubecos.cdx.json: $(PROJ_BASE_ROOTFS)
	$(call RUN_CMD_TIMED, dnf install -y $(SYFT_RPM) $(GRYPE_RPM) $(COSIGN_RPM),"  DNF     syft grype cosign")
	$(call RUN_CMD_TIMED, cd $< && syft --config $(SRCDIR)/syft.yml --source-version $(PROJ_NAME)_$(PROJ_VERSION) ./,"  GEN     $@")

PKGCLEAN += $(PROJ_SBOM) syft-fs-cubecos.cdx.json cosign.key cosign.pub

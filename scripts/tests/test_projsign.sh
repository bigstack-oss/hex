#!/bin/bash
#
# Unit test for make/projsign.mk (sums, sign, attest, verify) and scripts/makesbompackages:
# a small stand-in ship directory, a throwaway key and no transparency log, so it runs in
# seconds with no build tree and no network. Keyless signing is not covered: it needs a login.
#
# Needs bash, GNU make, coreutils, python3, and cosign at projsign.mk's COSIGN_VER on PATH --
# the test fails rather than let projsign.mk fall back to `dnf install`.
# Run: bash test_projsign.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HEX="$(cd "$DIR/../.." && pwd)"

VER=$(sed -n 's/^COSIGN_VER *:= *//p' "$HEX/make/projsign.mk")
cosign version 2>/dev/null | grep -q "v$VER$" || { echo "needs cosign v$VER on PATH (projsign.mk's COSIGN_VER)"; exit 1; }

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT

# Anything that reaches for the network fails at once instead of hanging, so an offline
# signature that quietly starts needing sigstore.dev shows up here.
export HTTPS_PROXY=http://127.0.0.1:9 HTTP_PROXY=http://127.0.0.1:9 https_proxy=http://127.0.0.1:9 http_proxy=http://127.0.0.1:9 NO_PROXY= no_proxy=

cat > "$T/Makefile" <<EOF
VERBOSE := 0
include $HEX/make/run_cmd_definitions.mk
PROJ_NAME := test
PROJ_VERSION := 1.0
PROJ_SHIPDIR := \$(CURDIR)/ship
PROJ_RELEASE := proj.release
PROJ_BUILD_SIGN := 1
PROJ_COSIGN_TLOG := 0
PROJ_SBOM_ATTESTED := proj.pkg proj.iso
include $HEX/make/projsign.mk
EOF

pass=0 fail=0
chk(){ if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }
mk(){ OUT=$(make -C "$T" --no-print-directory "$@" 2>&1); RC=$?; }
has(){ grep -q -- "$2" <<<"$1" && echo yes || echo no; }

# --- makesbompackages: drops file components, syft's properties and edges to dropped refs
cat > "$T/full.cdx.json" <<'EOF'
{"bomFormat":"CycloneDX","specVersion":"1.6",
 "metadata":{"component":{"bom-ref":"root","type":"file","name":"rootfs"}},
 "components":[
  {"bom-ref":"pkg:a","type":"library","name":"a","version":"1","properties":[{"name":"syft:x","value":"y"}]},
  {"bom-ref":"pkg:b","type":"library","name":"b","version":"2"},
  {"bom-ref":"file:1","type":"file","name":"/usr/bin/a"}],
 "dependencies":[
  {"ref":"root","dependsOn":["pkg:a"]},
  {"ref":"pkg:a","dependsOn":["pkg:b","file:1"]},
  {"ref":"file:1","dependsOn":[]}]}
EOF
bash "$HEX/scripts/makesbompackages" "$T/full.cdx.json" "$T/pkgs.cdx.json"; chk "makesbompackages exits 0" "$?" "0"
q(){ python3 -c "import json,sys; b=json.load(open('$T/pkgs.cdx.json')); print($1)"; }
chk "package components kept" "$(q '",".join(c["bom-ref"] for c in b["components"])')" "pkg:a,pkg:b"
chk "properties dropped" "$(q 'sum("properties" in c for c in b["components"])')" "0"
chk "edges to files dropped" "$(q '[d["dependsOn"] for d in b["dependencies"] if d["ref"]=="pkg:a"][0]')" "['pkg:b']"
chk "file entries dropped" "$(q '",".join(d["ref"] for d in b["dependencies"])')" "root,pkg:a"
chk "metadata kept" "$(q 'b["metadata"]["component"]["name"]')" "rootfs"
bash "$HEX/scripts/makesbompackages" "$T/full.cdx.json" 2>/dev/null >/dev/null; chk "usage error with one arg" "$?" "1"

# --- a ship directory as a build leaves it
R=test_1.0_label_desc
S="$T/ship"; mkdir -p "$S"
ln -s $R "$T/proj.release"
head -c 65536 /dev/urandom > "$S/$R.pkg"
head -c 4096 /dev/urandom > "$S/${R}_1.pkg"              # split part: not in the manifest
head -c 65536 /dev/urandom > "$S/$R.iso"
md5sum < "$S/$R.pkg" > "$S/$R.pkg.md5"; sha256sum < "$S/$R.pkg" > "$S/$R.pkg.sha256"
cp "$T/full.cdx.json" "$S/${R}_sbom.json"; cp "$T/pkgs.cdx.json" "$S/${R}_sbom_packages.json"
ln -s "$S/$R.pkg" "$T/proj.pkg"; ln -s "$S/$R.iso" "$T/proj.iso"

mk sums; chk "sums exits 0" "$RC" "0"
M="$S/${R}_SHA256SUMS"
chk "manifest lists the deliverables" "$(cut -d' ' -f3- "$M" | tr '\n' ' ')" "$R.iso $R.pkg ${R}_sbom.json ${R}_sbom_packages.json "
chk "manifest checks out" "$(cd "$S" && sha256sum -c --strict --quiet "${R}_SHA256SUMS" >/dev/null 2>&1; echo $?)" "0"
chk "proj.sha256sums links the manifest" "$(readlink "$T/proj.sha256sums")" "$M"

mk sign; chk "sign exits 0" "$RC" "0"
chk "manifest signed" "$([ -s "$M.sigstore.json" ] && echo yes)" "yes"
chk "public key published" "$([ -s "$S/${R}_pub.key" ] && echo yes)" "yes"
mk attest; chk "attest exits 0" "$RC" "0"
chk "pkg attested" "$([ -s "$S/$R.pkg.sbom.sigstore.json" ] && echo yes)" "yes"
chk "iso attested" "$([ -s "$S/$R.iso.sbom.sigstore.json" ] && echo yes)" "yes"
mk verify; chk "verify passes" "$RC" "0"

# A consumer's check, straight from projsign.mk's header, with nothing but the shipped files
chk "consumer verify-blob" "$(cd "$S" && cosign verify-blob --key ${R}_pub.key --insecure-ignore-tlog --bundle ${R}_SHA256SUMS.sigstore.json ${R}_SHA256SUMS >/dev/null 2>&1; echo $?)" "0"
H=$(awk -v n=$R.iso '$2 == n { print $1 }' "$M")
chk "consumer verify-blob-attestation" "$(cd "$S" && cosign verify-blob-attestation --key ${R}_pub.key --insecure-ignore-tlog --type cyclonedx --bundle $R.iso.sbom.sigstore.json --digest $H --digestAlg sha256 >/dev/null 2>&1; echo $?)" "0"

# --- every check below starts again from this signed, verified state
cp -a "$S" "$T/good"
reset(){ rm -rf "$S"; cp -a "$T/good" "$S"; }

reset; printf 'x' | dd of="$S/$R.iso" bs=1 seek=100 conv=notrunc 2>/dev/null
mk verify; chk "verify fails on a changed byte" "$RC" "2"
chk "  and names the file" "$(has "$OUT" "$R.iso: FAILED")" "yes"

reset; sed -i "s/  $R.iso\$/  $R.iso.renamed/" "$M"; touch -r "$T/good/${R}_SHA256SUMS" "$M"
mk verify; chk "verify fails on an edited manifest" "$RC" "2"

reset; rm -f "$S/$R.pkg.sbom.sigstore.json"
mk verify; chk "verify fails on a missing attestation" "$RC" "2"
chk "  and names the image" "$(has "$OUT" "$R.pkg: no manifest entry or no attestation")" "yes"

# Signed by the same key over the right digest, but not a CycloneDX SBOM
reset; H=$(awk -v n=$R.pkg '$2 == n { print $1 }' "$M"); echo '{"x":1}' > "$T/other.json"
(cd "$T" && COSIGN_PASSWORD= cosign attest-blob --yes --key cosign.key --use-signing-config=false --tlog-upload=false --predicate other.json --type https://example.com/other --hash $H --bundle "$S/$R.pkg.sbom.sigstore.json" "$S/$R.pkg" >/dev/null 2>&1)
chk "  (substitute attestation made)" "$?" "0"
mk verify; chk "verify fails on the wrong predicate type" "$RC" "2"

reset; touch -d '1 minute ago' "$M"
mk sign; chk "sign refuses a stale manifest" "$RC" "2"
chk "  and says why" "$(has "$OUT" "is newer than the manifest: run make sums")" "yes"

reset; rm -f "$S/$R.iso"
mk attest; chk "attest refuses a manifest naming a missing file" "$RC" "2"
chk "  and says which" "$(has "$OUT" "$R.iso is in the manifest but missing")" "yes"

# Regenerating the manifest drops the signature and attestations, which covered the old digests
reset; head -c 65536 /dev/urandom > "$S/$R.iso"; rm -f "$T/proj.sha256sums"
mk sums; chk "re-sums exits 0" "$RC" "0"
chk "re-sums drops the old signature" "$(ls "$S" | grep -c 'sigstore.json$')" "0"
chk "re-sums lists no verification material" "$(grep -c -e sigstore -e _pub.key "$M")" "0"
mk verify; chk "verify fails on an unsigned manifest" "$RC" "2"
chk "  and says so" "$(has "$OUT" "is not signed: run make sign")" "yes"

echo "pass=$pass fail=$fail"; [ $fail -eq 0 ]

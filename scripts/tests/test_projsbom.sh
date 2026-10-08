#!/bin/bash
#
# Unit test for make/projsbom.mk's PROJ_VEX wiring: which --vex flags reach grype, that a
# changed document triggers a rescan, and that a missing one stops the scan. grype is a stand-in
# that records its arguments, so it runs in seconds with no scanner, no DB and no network.
#
# Needs bash, GNU make, coreutils and python3.
# Run: bash test_projsbom.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HEX="$(cd "$DIR/../.." && pwd)"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT

mkdir -p "$T/bin"
cat > "$T/bin/grype" <<EOF
#!/bin/bash
echo "\$*" > "$T/grype.args"
echo '{"matches":[]}'
EOF
chmod +x "$T/bin/grype"
export PATH="$T/bin:$PATH"

cat > "$T/Makefile" <<EOF
VERBOSE := 0
include $HEX/make/run_cmd_definitions.mk
PROJ_NAME := test
PROJ_VERSION := 1.0
PROJ_SHIPDIR := \$(CURDIR)/ship
PROJ_RELEASE := proj.release
PROJ_SBOM := proj.sbom
PROJ_BASE_ROOTFS := rootfs.stamp
HEX_SCRIPTSDIR := $HEX/scripts
SRCDIR := \$(CURDIR)
include $HEX/make/projsbom.mk
EOF

pass=0 fail=0
chk(){ if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }
mk(){ rm -f "$T/grype.args"; OUT=$(make -C "$T" --no-print-directory "$@" 2>&1); RC=$?; }
args(){ cat "$T/grype.args" 2>/dev/null; }

# A rootfs older than its SBOM, so make never reaches the syft rule (it would dnf install).
touch -d '2 hours ago' "$T/rootfs.stamp"
echo '{"bomFormat":"CycloneDX","specVersion":"1.6","components":[]}' > "$T/syft-fs-cubecos.cdx.json"
touch -d '90 minutes ago' "$T/syft-fs-cubecos.cdx.json"
ln -s test_1.0_label_desc "$T/proj.release"
echo '{}' > "$T/a.openvex.json"; echo '{}' > "$T/b.openvex.json"

mk sbom; chk "no PROJ_VEX: exits 0" "$RC" "0"
chk "no PROJ_VEX: no --vex" "$(args)" "sbom:syft-fs-cubecos.cdx.json --output=json"
chk "vuln.json written" "$(cat "$T/ship/test_1.0_label_desc_vuln.json")" '{"matches":[]}'

mk sbom PROJ_VEX=a.openvex.json; chk "one document: exits 0" "$RC" "0"
chk "one document: not rescanned without a change" "$(args)" ""

# Only the document is newer than the report, so only it can trigger the rescan.
touch -d '1 hour ago' "$T/proj.sbom"; touch "$T/a.openvex.json"
mk sbom PROJ_VEX=a.openvex.json; chk "changed document: rescans" "$RC" "0"
chk "one document: --vex" "$(args)" "sbom:syft-fs-cubecos.cdx.json --vex a.openvex.json --output=json"

rm -f "$T/proj.sbom"
mk sbom "PROJ_VEX=a.openvex.json b.openvex.json"; chk "two documents: exits 0" "$RC" "0"
chk "two documents: --vex each" "$(args)" "sbom:syft-fs-cubecos.cdx.json --vex a.openvex.json --vex b.openvex.json --output=json"

rm -f "$T/proj.sbom"
mk sbom PROJ_VEX=missing.openvex.json; chk "missing document: fails" "$RC" "2"
chk "missing document: grype not run" "$(args)" ""
chk "missing document: named" "$(grep -c 'missing.openvex.json' <<<"$OUT")" "1"

echo "test_projsbom: $pass passed, $fail failed"
[ "$fail" -eq 0 ]

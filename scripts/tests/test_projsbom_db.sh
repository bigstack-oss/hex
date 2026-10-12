#!/bin/bash
#
# Unit test for make/projsbom.mk's GRYPE_DB pin: no pin scans as before; a pin is imported once
# into its own cache and scanned without update or age checks; a changed pin rescans; a pin
# without a checksum is refused. grype is a stand-in that records its calls, so it runs in
# seconds with no scanner, no DB and no network.
#
# Needs bash, GNU make (4.2 or later, for $(file <)) and coreutils.
# Run: bash test_projsbom_db.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HEX="$(cd "$DIR/../.." && pwd)"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
export HOME="$T/home"

# The stand-in: `db status` reports the "from" of the last import into $GRYPE_DB_CACHE_DIR,
# `db import` records one, and a scan prints an empty report.
mkdir -p "$T/bin"
cat > "$T/bin/grype" <<EOF
#!/bin/bash
echo "\${GRYPE_DB_CACHE_DIR:--} \${GRYPE_DB_AUTO_UPDATE:--} \${GRYPE_DB_VALIDATE_AGE:--} \$*" >> "$T/grype.calls"
case "\$1 \$2" in
"db status") [ -f "\$GRYPE_DB_CACHE_DIR/from" ] || exit 1; printf '{\n "from": "%s"\n}\n' "\$(cat "\$GRYPE_DB_CACHE_DIR/from")" ;;
"db import") mkdir -p "\$GRYPE_DB_CACHE_DIR"; printf '%s' "\$3" > "\$GRYPE_DB_CACHE_DIR/from" ;;
*) echo '{"matches":[]}' ;;
esac
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
mk(){ rm -f "$T/grype.calls"; OUT=$(make -C "$T" --no-print-directory "$@" 2>&1); RC=$?; }
calls(){ cat "$T/grype.calls" 2>/dev/null; }

# A rootfs older than its SBOM, so make never reaches the syft rule (it would dnf install).
touch -d '2 hours ago' "$T/rootfs.stamp"
echo '{"bomFormat":"CycloneDX","specVersion":"1.6","components":[]}' > "$T/syft-fs-cubecos.cdx.json"
touch -d '90 minutes ago' "$T/syft-fs-cubecos.cdx.json"
ln -s test_1.0_label_desc "$T/proj.release"

SHA=$(printf 'a%.0s' {1..64})
PIN="https://db.example/v6/vulnerability-db_v6.1.10_x.tar.zst?checksum=sha256:$SHA"
SHA2=$(printf 'b%.0s' {1..64})
PIN2="https://db.example/v6/vulnerability-db_v6.1.10_y.tar.zst?checksum=sha256:$SHA2"
CACHE="$HOME/.cache/grype/pinned/$SHA"

mk sbom; chk "no pin: exits 0" "$RC" "0"
chk "no pin: scans as before" "$(calls)" "- - - sbom:syft-fs-cubecos.cdx.json --output=json"
chk "vuln.json written" "$(cat "$T/ship/test_1.0_label_desc_vuln.json")" '{"matches":[]}'

mk sbom; chk "no pin again: not rescanned" "$(calls)" ""

mk sbom "GRYPE_DB=$PIN"; chk "pin: exits 0" "$RC" "0"
chk "pin: imported, then scanned without update or age checks" "$(calls)" "$CACHE false false db status --output=json
$CACHE false false db import $PIN
$CACHE false false sbom:syft-fs-cubecos.cdx.json --output=json"

mk sbom "GRYPE_DB=$PIN"; chk "same pin: not rescanned" "$(calls)" ""

rm -f "$T/proj.sbom"
mk sbom "GRYPE_DB=$PIN"; chk "pin already imported: scanned without import" "$(calls)" "$CACHE false false db status --output=json
$CACHE false false sbom:syft-fs-cubecos.cdx.json --output=json"

mk sbom "GRYPE_DB=$PIN2"; chk "changed pin: rescans with the new DB" "$(calls | tail -1)" \
  "$HOME/.cache/grype/pinned/$SHA2 false false sbom:syft-fs-cubecos.cdx.json --output=json"

mk sbom; chk "pin removed: rescans with the newest DB" "$(calls)" "- - - sbom:syft-fs-cubecos.cdx.json --output=json"

mk sbom "GRYPE_DB=https://db.example/v6/x.tar.zst"; chk "no checksum: fails" "$RC" "2"
chk "  without running grype" "$(calls)" ""
chk "  and says why" "$(grep -c 'GRYPE_DB must be an archive URL with ?checksum=sha256:' <<<"$OUT")" "1"

mk sbom "GRYPE_DB=$T/db.tar.zst"; chk "local file: fails" "$RC" "2"

mk sbom "GRYPE_DB=$PIN'"; chk "single quote: fails" "$RC" "2"
chk "  and says why" "$(grep -c 'must not contain a single quote' <<<"$OUT")" "1"

echo "test_projsbom_db: $pass passed, $fail failed"
[ "$fail" -eq 0 ]

#!/bin/bash
#
# Unit test for scripts/installbootstrap: fragments land exactly once however
# often the install re-runs. Self-contained. Run: bash test_installbootstrap.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT="$DIR/../installbootstrap"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ROOT="$T/rootfs"; mkdir -p "$ROOT/usr/sbin"
printf '#!/bin/sh\n# os header\nhostnamectl set-hostname unconfigured\n' > "$ROOT/usr/sbin/bootstrap"
printf '# Start crash daemon\n/usr/sbin/hex_crashd || true\n' > "$T/frag_crashd"
printf '#!/bin/sh\nexport BOOTSTRAP_CUBE_MARKER=/run/cube_commit_done\necho cube part\n' > "$T/frag_cube"

pass=0 fail=0
chk(){ if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }
count(){ grep -c -- "$1" "$ROOT/usr/sbin/bootstrap"; }

sh "$SCRIPT" "$ROOT" "$T/frag_crashd" "$T/frag_cube"; chk "first run exits 0" "$?" "0"
chk "os header kept" "$(head -1 "$ROOT/usr/sbin/bootstrap")" "#!/bin/sh"
chk "header hostname line kept" "$(count 'set-hostname unconfigured')" "1"
chk "crashd fragment once" "$(count 'hex_crashd')" "1"
chk "cube fragment once" "$(count 'echo cube part')" "1"
chk "fragment order" "$(grep -n 'hex_crashd\|echo cube part' "$ROOT/usr/sbin/bootstrap" | cut -d: -f2- | tr '\n' '|')" "/usr/sbin/hex_crashd || true|echo cube part|"
first=$(md5sum < "$ROOT/usr/sbin/bootstrap")

# second run: no second copy
sh "$SCRIPT" "$ROOT" "$T/frag_crashd" "$T/frag_cube"; chk "second run exits 0" "$?" "0"
chk "second run is a no-op" "$(md5sum < "$ROOT/usr/sbin/bootstrap")" "$first"
chk "crashd still once" "$(count 'hex_crashd')" "1"
chk "cube still once" "$(count 'echo cube part')" "1"

# changed fragment replaces, never stacks
printf '#!/bin/sh\necho cube part v2\n' > "$T/frag_cube"
sh "$SCRIPT" "$ROOT" "$T/frag_crashd" "$T/frag_cube"
chk "old fragment gone" "$(count 'echo cube part$')" "0"
chk "new fragment once" "$(count 'echo cube part v2')" "1"
chk "header survives replace" "$(count 'set-hostname unconfigured')" "1"

# legacy unmarked copy (pre-marker image): never stack a third
printf '#!/bin/sh\n# os header\n' > "$ROOT/usr/sbin/bootstrap"
cat "$T/frag_crashd" >> "$ROOT/usr/sbin/bootstrap"      # legacy unmarked append
sh "$SCRIPT" "$ROOT" "$T/frag_crashd"
sh "$SCRIPT" "$ROOT" "$T/frag_crashd"
chk "legacy copy + one marked copy, never more" "$(count 'hex_crashd')" "2"

sh "$SCRIPT" 2>/dev/null; chk "usage error without args" "$?" "1"
sh "$SCRIPT" "$T/nope" "$T/frag_crashd" 2>/dev/null; chk "missing rootdir fails" "$?" "1"

echo "pass=$pass fail=$fail"; [ $fail -eq 0 ]

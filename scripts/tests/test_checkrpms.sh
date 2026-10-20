#!/bin/bash
#
# Unit test for scripts/checkrpms: an rpm passes only on a signature that verifies against the
# rootfs keyring, or on the sha256 a pin gives for its file name; every other rpm is named with
# the reason, and the exit status is 1. rpmkeys is a stand-in that answers as rpm 4.16 does, from
# a verdict written in each test file, and records the root it was handed.
#
# Needs bash and coreutils.
# Run: bash test_checkrpms.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHECK="$DIR/../checkrpms"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0 fail=0
chk(){ if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }

# Each test rpm's first line is its verdict: OK <key>, NOKEY <key>, BAD <key> (signed, payload
# altered) or UNSIGNED.
mkdir -p "$T/bin"
cat > "$T/bin/rpmkeys" <<EOF
#!/bin/bash
[ "\$1" = --root ] || { echo "no --root" >&2; exit 2; }
echo "\$2" >> "$T/roots"; mode=\$3; shift 3
echo "warning: unsupported version of key: V3" >&2
rc=0
for f in "\$@"; do
    read -r verdict key < "\$f"
    if [ "\$mode" = -K ]; then
        case \$verdict in
        OK) echo "\$f: digests signatures OK" ;;
        NOKEY) echo "\$f: digests SIGNATURES NOT OK"; rc=1 ;;
        BAD) echo "\$f: DIGESTS SIGNATURES NOT OK"; rc=1 ;;
        UNSIGNED) echo "\$f: digests OK" ;;
        esac
        continue
    fi
    echo "\$f:"
    case \$verdict in
    OK|NOKEY) s=\$verdict; [ \$s = OK ] || rc=1
        printf '    Header V4 RSA/SHA256 Signature, key ID %s: %s\n    Header SHA256 digest: OK\n    Payload SHA256 digest: OK\n    V4 RSA/SHA256 Signature, key ID %s: %s\n' \$key \$s \$key \$s ;;
    BAD) rc=1
        printf '    Header V4 RSA/SHA256 Signature, key ID %s: OK\n    Header SHA256 digest: OK\n    Payload SHA256 digest: BAD (Expected 1f != 2e)\n    V4 RSA/SHA256 Signature, key ID %s: BAD\n' \$key \$key ;;
    UNSIGNED) printf '    Header SHA256 digest: OK\n    Payload SHA256 digest: OK\n' ;;
    esac
done
exit \$rc
EOF
chmod +x "$T/bin/rpmkeys"
export PATH="$T/bin:$PATH"

mkdir -p "$T/root" "$T/rpms"
mk(){ printf '%s\n' "$2" > "$T/rpms/$1"; }
sha(){ sha256sum < "$T/rpms/$1" | cut -d' ' -f1; }
run(){ rm -f "$T/roots"; OUT=$(bash "$CHECK" "$T/root" "$T/rpms" "$@" 2>&1); RC=$?; }

# Nothing to check
run
chk "an empty directory passes" "$RC" "0"
chk "an empty directory runs no rpmkeys" "$([ -e "$T/roots" ] && echo ran || echo none)" "none"

mk a-1.0-1.el9.x86_64.rpm "OK 8483c65d"
mk b-2.0-1.el9.noarch.rpm "OK 8483c65d"
echo "not an rpm" > "$T/rpms/readme.txt"
run
chk "trusted signatures pass" "$RC" "0"
chk "trusted signatures print nothing" "$OUT" ""
chk "checked against the rootfs keyring" "$(sort -u "$T/roots")" "$T/root"
chk "one batch call when every rpm passes" "$(wc -l < "$T/roots" | tr -d ' ')" "1"

# Unsigned: rpmkeys exits 0, so only the result text can tell it apart
mk c-3.0-1.el9.x86_64.rpm "UNSIGNED"
run
chk "an unsigned rpm fails" "$RC" "1"
chk "an unsigned rpm is named" "$(grep -c 'c-3.0-1.el9.x86_64.rpm: not signed, and no sha256 pinned' <<< "$OUT")" "1"
chk "signed rpms are not listed" "$(grep -c 'a-1.0\|b-2.0' <<< "$OUT")" "0"

echo "$(sha c-3.0-1.el9.x86_64.rpm) c-3.0-1.el9.x86_64.rpm" > "$T/pins"
run "$T/pins"
chk "an unsigned rpm passes on its pin" "$RC" "0"

# A pin that does not match, the last pin for a name winning, comments skipped
{ echo "# $(sha c-3.0-1.el9.x86_64.rpm) c-3.0-1.el9.x86_64.rpm"; echo "$(sha c-3.0-1.el9.x86_64.rpm) c-3.0-1.el9.x86_64.rpm"
  echo "$(printf 0%.0s {1..64}) c-3.0-1.el9.x86_64.rpm"; } > "$T/pins"
run "$T/pins"
chk "a later wrong pin wins" "$RC" "1"
chk "a wrong pin names both sums" "$(grep -c "c-3.0-1.el9.x86_64.rpm: sha256 $(sha c-3.0-1.el9.x86_64.rpm), pinned $(printf 0%.0s {1..64})" <<< "$OUT")" "1"
{ echo "# $(printf 0%.0s {1..64}) c-3.0-1.el9.x86_64.rpm"; echo "$(sha c-3.0-1.el9.x86_64.rpm) c-3.0-1.el9.x86_64.rpm"; } > "$T/pins"
run "$T/pins"
chk "a commented pin is skipped" "$RC" "0"
chk "a missing pins file is no pin" "$(bash "$CHECK" "$T/root" "$T/rpms" "$T/nopins" >/dev/null 2>&1; echo $?)" "1"

# Signed with a key the rootfs lacks, and signed but altered
mk d-4.0-1.el9.x86_64.rpm "NOKEY 10458545"
mk e-5.0-1.el9.x86_64.rpm "BAD 8483c65d"
run "$T/pins"
chk "untrusted and altered rpms fail" "$RC" "1"
chk "the count is reported" "$(head -1 <<< "$OUT" | grep -o '2 rpm(s)')" "2 rpm(s)"
chk "an untrusted key is named" "$(grep -c 'd-4.0-1.el9.x86_64.rpm: signed with key 10458545, which is not in the rootfs keyring, and no sha256 pinned' <<< "$OUT")" "1"
chk "an altered payload is named" "$(grep -c 'e-5.0-1.el9.x86_64.rpm: Payload SHA256 digest: BAD (Expected 1f != 2e), and no sha256 pinned' <<< "$OUT")" "1"
chk "rpmkeys warnings stay out of the report" "$(grep -c warning <<< "$OUT")" "0"

# A pin lets a signed rpm whose key the rootfs lacks through, but never one already accepted
echo "$(sha d-4.0-1.el9.x86_64.rpm) d-4.0-1.el9.x86_64.rpm" >> "$T/pins"
rm "$T/rpms/e-5.0-1.el9.x86_64.rpm"
run "$T/pins"
chk "a pinned rpm with an unknown key passes" "$RC" "0"

# Usage
chk "a missing rpm dir is usage" "$(bash "$CHECK" "$T/root" "$T/none" >/dev/null 2>&1; echo $?)" "1"
chk "a missing root dir is usage" "$(bash "$CHECK" "$T/none" "$T/rpms" >/dev/null 2>&1; echo $?)" "1"
chk "too many arguments is usage" "$(bash "$CHECK" "$T/root" "$T/rpms" "$T/pins" x 2>&1 | head -1 | cut -d' ' -f1)" "Usage:"

echo "checkrpms: $pass passed, $fail failed"
[ $fail -eq 0 ]

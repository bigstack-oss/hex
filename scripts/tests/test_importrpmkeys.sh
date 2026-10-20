#!/bin/bash
#
# Unit test for scripts/importrpmkeys: a key file is imported into the rootfs keyring only when
# the primary keys it holds are exactly the fingerprints its line pins, and anything else stops
# the import. gpg is the real one, reading keys generated here; rpm is a stand-in that records
# what it would import.
#
# Needs bash, coreutils and gpg.
# Run: bash test_importrpmkeys.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMPORT="$DIR/../importrpmkeys"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0 fail=0
chk(){ if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }

mkdir -p "$T/bin" "$T/root" "$T/keys" "$T/tmp"
# rpm keeps each imported key armored in its gpg-pubkey package's description
cat > "$T/bin/rpm" <<EOF
#!/bin/bash
case "\$3" in
--import) echo "\$*" >> "$T/rpm.calls"; cat "\$4" >> "$T/keyring" ;;
-q) [ -s "$T/keyring" ] && cat "$T/keyring" || { echo "package gpg-pubkey is not installed"; exit 1; } ;;
esac
EOF
chmod +x "$T/bin/rpm"

# Two signing keys, made in a home of their own
export GNUPGHOME="$T/gen"; mkdir -m 700 "$GNUPGHOME"
for n in a b; do
    gpg --batch --quiet --passphrase '' --quick-gen-key "Key $n <$n@example.com>" ed25519 sign never 2>/dev/null
    gpg --batch --armor --export "$n@example.com" > "$T/keys/$n.asc"
done
fpr(){ gpg --batch --with-colons --fingerprint "$1@example.com" 2>/dev/null | awk -F: '$1 == "fpr" { print $10; exit }'; }
FA=$(fpr a) FB=$(fpr b)
cat "$T/keys/a.asc" "$T/keys/b.asc" > "$T/keys/ab.asc"
gpgconf --kill all 2>/dev/null
before=$(find "$GNUPGHOME" -type f | sort | xargs sha256sum)

export PATH="$T/bin:$PATH" TMPDIR="$T/tmp"
run(){ rm -f "$T/rpm.calls" "$T/keyring"; printf '%s\n' "$@" > "$T/list"; OUT=$(bash "$IMPORT" "$T/root" "$T/list" 2>&1); RC=$?
    CALLS=$(cat "$T/rpm.calls" 2>/dev/null); }

run "$T/keys/a.asc $FA"
chk "a pinned key imports" "$RC" "0"
chk "into the rootfs keyring" "$CALLS" "--root $T/root --import $T/keys/a.asc"

run "# vendor keys" "" "$T/keys/a.asc $(echo $FA | tr A-F a-f)" "$T/keys/b.asc $FB"
chk "comments, blanks and lower case fingerprints" "$RC" "0"
chk "each listed file is imported" "$(wc -l <<< "$CALLS" | tr -d ' ')" "2"

run "keys/b.asc $FB"
chk "a relative key file is read from the list's directory" "$RC $CALLS" "0 --root $T/root --import $T/keys/b.asc"

run "$T/keys/ab.asc $FB $FA"
chk "a file of two keys imports with both pinned" "$RC" "0"

run "$T/keys/ab.asc $FA"
chk "a file with a key not pinned stops" "$RC" "1"
chk "and is not imported" "$CALLS" ""
chk "the message names both lists" "$(grep -c "ab.asc holds key(s) \[$(printf '%s\n' $FA $FB | sort | xargs)\], pinned \[$FA\]" <<< "$OUT")" "1"

run "$T/keys/a.asc $FB"
chk "a key that is not the pinned one stops" "$RC" "1"

run "$T/keys/a.asc $FA" "$T/keys/a.asc $FB"
chk "a bad line stops the import there" "$RC" "1"
chk "the lines before it are imported" "$CALLS" "--root $T/root --import $T/keys/a.asc"

echo "not a key" > "$T/keys/junk.asc"
run "$T/keys/junk.asc $FA"
chk "a file with no key stops" "$RC" "1"
chk "and says it holds none" "$(grep -c 'holds key(s) \[none\]' <<< "$OUT")" "1"

run "$T/keys/missing.asc $FA"
chk "a missing key file stops" "$RC" "1"
run "$T/keys/a.asc"
chk "a key without a pin stops" "$RC" "1"
chk "and is not imported" "$CALLS" ""

chk "the temporary gpg home is removed" "$(ls -A "$T/tmp")" ""
chk "the caller's gpg home is untouched" "$(find "$GNUPGHOME" -type f | sort | xargs sha256sum)" "$before"
# A keyring that already holds a line's keys skips it, even when the file has gone
rm -f "$T/rpm.calls" "$T/keyring"
printf '%s\n' "$T/keys/a.asc $FA" > "$T/list"
bash "$IMPORT" "$T/root" "$T/list" >/dev/null 2>&1
printf '%s\n' "$T/keys/gone.asc $FA" "$T/keys/b.asc $FB" "$T/keys/ab.asc $FA $FB" > "$T/list"
OUT=$(bash "$IMPORT" "$T/root" "$T/list" 2>&1); RC=$?
chk "a rerun imports only what is new" "$RC $(cut -d' ' -f4 "$T/rpm.calls" | xargs -n1 basename | xargs)" "0 a.asc b.asc"

# The audit: a key in the keyring that the list does not pin fails, and nothing is imported
rm -f "$T/rpm.calls" "$T/keyring"
printf '%s\n' "$T/keys/ab.asc $FA $FB" > "$T/list"
bash "$IMPORT" "$T/root" "$T/list" >/dev/null 2>&1; rm -f "$T/rpm.calls"
chk "the audit passes a keyring the list pins" "$(bash "$IMPORT" -c "$T/root" "$T/list" 2>&1; echo $?)" "0"
printf '%s\n' "# $T/keys/b.asc $FB" "$T/keys/a.asc $FA" "$T/keys/new.asc $(printf 'C%.0s' {1..40})" > "$T/list"
OUT=$(bash "$IMPORT" -c "$T/root" "$T/list" 2>&1); RC=$?
chk "the audit fails a key the list does not pin" "$RC" "1"
chk "and names it" "$(grep -c "does not pin: $FB\$" <<< "$OUT")" "1"
chk "the audit imports nothing" "$([ -e "$T/rpm.calls" ] && echo imported || echo none)" "none"
rm -f "$T/keyring"
chk "the audit passes an empty keyring" "$(bash "$IMPORT" -c "$T/root" "$T/list" 2>&1; echo $?)" "0"

chk "a missing list is usage" "$(bash "$IMPORT" "$T/root" "$T/nolist" >/dev/null 2>&1; echo $?)" "1"

echo "importrpmkeys: $pass passed, $fail failed"
[ $fail -eq 0 ]

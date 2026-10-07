#!/bin/bash
#
# Unit test for scripts/fetchverified: a download is kept only with the pinned digest, and a
# good copy already on disk is reused without downloading. Self-contained (file:// URLs).
# Run: bash test_fetchverified.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT="$DIR/../fetchverified"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
head -c 4096 /dev/urandom > "$T/src.rpm"
GOOD=$(sha256sum < "$T/src.rpm" | cut -d' ' -f1)
BAD=$(printf '%064d' 0)
URL="file://$T/src.rpm"

pass=0 fail=0
chk(){ if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }
run(){ ERR=$(bash "$SCRIPT" "$@" 2>&1 >/dev/null); RC=$?; }

run "$URL" $GOOD "$T/a.rpm"; chk "good digest exits 0" "$RC" "0"
chk "file kept" "$(cmp -s "$T/src.rpm" "$T/a.rpm" && echo same)" "same"
chk "no partial left" "$(ls "$T" | grep -c '\.part$')" "0"

run "$URL" $BAD "$T/b.rpm"; chk "wrong digest exits 1" "$RC" "1"
chk "nothing kept" "$([ -e "$T/b.rpm" ] && echo yes || echo no)" "no"
chk "no partial left after a mismatch" "$(ls "$T" | grep -c '\.part$')" "0"
chk "names both digests" "$(grep -c "has sha256 $GOOD, expected $BAD" <<<"$ERR")" "1"

# A good copy on disk is reused: the URL is not even read
run "file://$T/nowhere.rpm" $GOOD "$T/a.rpm"; chk "good copy reused without download" "$RC" "0"
# A bad copy on disk is replaced
printf 'tampered' > "$T/c.rpm"
run "$URL" $GOOD "$T/c.rpm"; chk "bad copy replaced" "$RC" "0"
chk "  with the pinned file" "$(cmp -s "$T/src.rpm" "$T/c.rpm" && echo same)" "same"
# A bad copy is not kept when the download fails
printf 'tampered' > "$T/d.rpm"
run "file://$T/nowhere.rpm" $GOOD "$T/d.rpm"; chk "failed download exits 1" "$RC" "1"
chk "  and says so" "$(grep -c "cannot download" <<<"$ERR")" "1"
chk "  and the bad copy is gone" "$([ -e "$T/d.rpm" ] && echo yes || echo no)" "no"

run "$URL" nothex "$T/e.rpm"; chk "malformed digest exits 1" "$RC" "1"
run "$URL" $GOOD; chk "usage error with two args" "$RC" "1"

echo "pass=$pass fail=$fail"; [ $fail -eq 0 ]

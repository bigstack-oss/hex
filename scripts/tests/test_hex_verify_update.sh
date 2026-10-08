#!/bin/bash
#
# Unit test for data/hex_install/hex_verify_update.sh, the release-signature check hex_install
# runs before an update is mounted. A stand-in release signed with a throwaway key and no
# transparency log stands for a keyless one, which cannot be made offline; the identity path is
# checked up to cosign refusing a signature that is not the identity's. A stub logger records
# what reaches the system log.
#
# Needs bash, coreutils and cosign on PATH. Run: bash test_hex_verify_update.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT="$DIR/../../data/hex_install/hex_verify_update.sh"
command -v cosign >/dev/null || { echo "needs cosign on PATH"; exit 1; }

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
export HTTPS_PROXY=http://127.0.0.1:9 https_proxy=http://127.0.0.1:9 NO_PROXY= no_proxy=

# Stub logger: one line per call in $T/syslog
mkdir -p "$T/bin"
printf '#!/bin/sh\necho "$*" >> %s/syslog\n' "$T" > "$T/bin/logger"; chmod +x "$T/bin/logger"
export PATH="$T/bin:$PATH"

pass=0 fail=0
chk(){ if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }
has(){ grep -q -- "$2" <<<"$1" && echo yes || echo no; }
run(){ : > "$T/syslog"; OUT=$(SETTINGS="$T/settings.sys" TRUSTED_ROOT="${TR:-$T/none}" bash "$SCRIPT" "$@" 2>&1); RC=$?; LOG=$(cat "$T/syslog"); }

# --- a signed stand-in release: pkg, a split part, the manifest and its signature
R=TEST_1.0_label_desc
G="$T/good"; mkdir -p "$G"
head -c 65536 /dev/urandom > "$G/$R.pkg"; head -c 4096 /dev/urandom > "$G/${R}_1.pkg"
(cd "$G" && sha256sum $R.pkg ${R}_1.pkg > ${R}_SHA256SUMS)
(cd "$T" && COSIGN_PASSWORD= cosign generate-key-pair >/dev/null 2>&1)
(cd "$G" && COSIGN_PASSWORD= cosign sign-blob --yes --key "$T/cosign.key" --use-signing-config=false --tlog-upload=false --bundle ${R}_SHA256SUMS.sigstore.json ${R}_SHA256SUMS >/dev/null 2>&1)
chk "(stand-in signed)" "$([ -s "$G/${R}_SHA256SUMS.sigstore.json" ] && echo yes)" "yes"
: > "$T/settings.sys"
U="$T/update"
reset(){ rm -rf "$U"; cp -a "$G" "$U"; }
K=(-k "$T/cosign.pub" -n)
FILES=("$U/$R.pkg" "$U/${R}_1.pkg")

reset; run "${K[@]}" "$U" $R "${FILES[@]}"
chk "signed and intact: exit 0" "$RC" "0"
chk "  says verified" "$(has "$OUT" "Verified: $R (2 package file(s))")" "yes"
chk "  logs it" "$(has "$LOG" "user.info.*update of $R verified")" "yes"

reset; rm -f "$U/${R}_SHA256SUMS.sigstore.json"; run "${K[@]}" "$U" $R "${FILES[@]}"
chk "unsigned: exit 0 (installs)" "$RC" "0"
chk "  warns it is NOT signed" "$(has "$OUT" "WARNING: $R is NOT signed.")" "yes"
chk "  says what to copy" "$(has "$OUT" "copy its _SHA256SUMS and _SHA256SUMS.sigstore.json")" "yes"
chk "  says it continues" "$(has "$OUT" "Continuing with the upgrade")" "yes"
chk "  logs a warning" "$(has "$LOG" "user.warning.*$R is not signed")" "yes"

reset; run "$U" $R "${FILES[@]}"
chk "no signer configured: exit 0" "$RC" "0"
chk "  warns it cannot check" "$(has "$OUT" "no release signer configured")" "yes"
chk "  logs a warning" "$(has "$LOG" "user.warning.*no release signer configured")" "yes"

reset; rm -f "$U/${R}_SHA256SUMS"; run "${K[@]}" "$U" $R "${FILES[@]}"
chk "signature but no list: refused" "$RC" "1"
chk "  says the list is missing" "$(has "$OUT" "its list of digests is missing")" "yes"
chk "  says nothing changed" "$(has "$OUT" "stopped before anything was changed")" "yes"
chk "  logs an error with what is wrong" "$(has "$LOG" "user.err.*update of $R refused: $R is signed, but its list of digests is missing")" "yes"

reset; sed -i '1s/^./0/' "$U/${R}_SHA256SUMS"; run "${K[@]}" "$U" $R "${FILES[@]}"
chk "list edited after signing: refused" "$RC" "1"
chk "  says the signature does not verify" "$(has "$OUT" "the signature on ${R}_SHA256SUMS does not verify")" "yes"

reset; printf 'x' | dd of="$U/$R.pkg" bs=1 seek=100 conv=notrunc 2>/dev/null; run "${K[@]}" "$U" $R "${FILES[@]}"
chk "pkg changed: refused" "$RC" "1"
chk "  names it" "$(has "$OUT" "ERROR: $R.pkg does not match the signed list of digests")" "yes"
chk "  shows the expected digest" "$(has "$OUT" "expected sha256 $(awk -v n=$R.pkg '$2 == n { print $1 }' "$G/${R}_SHA256SUMS")")" "yes"
chk "  shows the actual digest" "$(has "$OUT" "actual   sha256 $(sha256sum < "$U/$R.pkg" | cut -d' ' -f1)")" "yes"

reset; printf 'x' | dd of="$U/${R}_1.pkg" bs=1 seek=10 conv=notrunc 2>/dev/null; run "${K[@]}" "$U" $R "${FILES[@]}"
chk "split part changed: refused" "$RC" "1"
chk "  names it" "$(has "$OUT" "ERROR: ${R}_1.pkg does not match")" "yes"

reset; head -c 100 /dev/urandom > "$U/${R}_9.pkg"; run "${K[@]}" "$U" $R "${FILES[@]}" "$U/${R}_9.pkg"
chk "file not in the list: refused" "$RC" "1"
chk "  names it" "$(has "$OUT" "${R}_9.pkg is not in the signed list")" "yes"
chk "  and logs which file" "$(has "$LOG" "refused: ${R}_9.pkg is not in the signed list")" "yes"

# --- the identity path: signer and issuer from settings, offline against a trusted root
printf 'sys.update.signer.identity = release@example.com\nsys.update.signer.issuer = https://github.com/login/oauth\n' > "$T/settings.sys"
reset; run "$U" $R "${FILES[@]}"
chk "identity, no trust data on the system: refused" "$RC" "1"
chk "  says so" "$(has "$OUT" "no Sigstore trust data")" "yes"
echo '{}' > "$T/trusted_root.json"
reset; TR="$T/trusted_root.json" run "$U" $R "${FILES[@]}"
chk "identity, signature not the identity's: refused" "$RC" "1"
chk "  names the expected signer" "$(has "$OUT" "was not signed by")" "yes"
chk "  and who" "$(has "$OUT" "release@example.com")" "yes"
reset; rm -f "$U/${R}_SHA256SUMS.sigstore.json"; TR="$T/trusted_root.json" run "$U" $R "${FILES[@]}"
chk "identity, unsigned: warns, naming the identity" "$(has "$OUT" "release@example.com or that it has not been modified")" "yes"
: > "$T/settings.sys"

# --- a real keyless signature, offline against the trusted root the rootfs ships: sigstore's own
# release checksums, signed by its release identity. Names do not matter to the signature, so the
# fixture stands in for a release's manifest and bundle.
TRR="$DIR/../../data/sigstore/trusted_root.json"; FX="$DIR/fixtures/sigstore"
KR=COSIGN_3.1.3; KD="$T/keyless"; mkdir -p "$KD"
cp "$FX/cosign_checksums.txt" "$KD/${KR}_SHA256SUMS"; cp "$FX/cosign_checksums.txt.sigstore.json" "$KD/${KR}_SHA256SUMS.sigstore.json"
echo "not the real binary" > "$KD/cosign-linux-amd64"      # listed in the manifest, wrong content
printf 'sys.update.signer.identity = keyless@projectsigstore.iam.gserviceaccount.com\nsys.update.signer.issuer = https://accounts.google.com\n' > "$T/settings.sys"
TR="$TRR" run "$KD" $KR "$KD/cosign-linux-amd64"
chk "keyless, right identity: signature verifies offline (reaches the digest check)" "$(has "$OUT" "Checking 1 package file(s)")" "yes"
chk "  then refuses the file that does not match" "$(has "$OUT" "ERROR: cosign-linux-amd64 does not match")" "yes"
chk "  naming the keyless signer" "$(has "$OUT" "signed by keyless@projectsigstore.iam.gserviceaccount.com")" "yes"
printf 'sys.update.signer.identity = release@example.com\nsys.update.signer.issuer = https://accounts.google.com\n' > "$T/settings.sys"
TR="$TRR" run "$KD" $KR "$KD/cosign-linux-amd64"
chk "keyless, another identity: refused at the signature" "$(has "$OUT" "the signature on ${KR}_SHA256SUMS does not verify")" "yes"
chk "  before any file is read" "$(has "$OUT" "Checking 1 package file")" "no"
chk "  without blaming key rotation" "$(has "$OUT" "Sigstore trust data is from")" "no"
printf 'sys.update.signer.identity = keyless@projectsigstore.iam.gserviceaccount.com\nsys.update.signer.issuer = https://github.com/login/oauth\n' > "$T/settings.sys"
TR="$TRR" run "$KD" $KR "$KD/cosign-linux-amd64"
chk "keyless, another issuer: refused at the signature" "$(has "$OUT" "does not verify")" "yes"
: > "$T/settings.sys"

# --- no cosign on the system
mkdir -p "$T/nocosign"; for c in bash sh sed awk grep cut sha256sum basename date cat head tail dd; do ln -sf "$(command -v $c)" "$T/nocosign/$c"; done; ln -sf "$T/bin/logger" "$T/nocosign/logger"
reset; : > "$T/syslog"; OUT=$(PATH="$T/nocosign" SETTINGS="$T/settings.sys" "$T/nocosign/bash" "$SCRIPT" "${K[@]}" "$U" $R "${FILES[@]}" 2>&1); RC=$?
chk "signed, but no cosign: refused" "$RC" "1"
chk "  says so" "$(has "$OUT" "has no cosign")" "yes"

bash "$SCRIPT" "$U" 2>/dev/null >/dev/null; chk "usage error" "$?" "2"

echo "pass=$pass fail=$fail"; [ $fail -eq 0 ]

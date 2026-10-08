#!/bin/bash
# HEX SDK
#
# Check a firmware update against its release signature before anything of it is mounted or run.
# Called by hex_install on the update path; runs on the system being updated, so every trust
# anchor comes from this system, never from the download:
#
#   - the signer's identity and OIDC issuer: sys.update.signer.identity / .issuer in
#     /etc/settings.sys, written at build time (projrootfs.mk);
#   - Sigstore's trusted root, shipped at $TRUSTED_ROOT, so the check works offline;
#   - cosign, installed in the rootfs.
#
# It never stops an update: an update must always be able to complete, offline included. Every
# outcome other than "verified" is a warning -- on the console and in the system log -- that
# says what is wrong and what to do, and the update goes on. It exits 0 whatever it finds;
# hex_install ignores its status anyway, so not even a fault in this script can block one.
#   verified                          "Verified: ..."
#   not signed (e.g. an RC under test) WARNING ... NOT signed
#   signed, but does not check out    WARNING ... signature does not verify / file does not match
#                                     / not in the list / list or tools missing

PROG=$(basename $0)
TRUSTED_ROOT=${TRUSTED_ROOT:-/usr/share/hex/sigstore/trusted_root.json}
SETTINGS=${SETTINGS:-/etc/settings.sys}

Usage()
{
    echo "Usage: $PROG [-i identity -o issuer | -k pubkey] [-n] <dir> <release> <file>..."
    echo "  Check <file>... (the package files of <release>, in <dir>) against"
    echo "  <dir>/<release>_SHA256SUMS and its signature <release>_SHA256SUMS.sigstore.json."
    echo "  The signer defaults to sys.update.signer.identity/.issuer in $SETTINGS."
    echo "  -k and -n (key, no transparency log) are for testing only."
    exit 2
}

IDENTITY= ISSUER= PUBKEY= TLOG=1
while getopts "i:o:k:n" opt ; do
    case $opt in
        i) IDENTITY=$OPTARG ;;
        o) ISSUER=$OPTARG ;;
        k) PUBKEY=$OPTARG ;;
        n) TLOG=0 ;;
        *) Usage ;;
    esac
done
shift $((OPTIND - 1))
[ $# -ge 3 ] || Usage
DIR=$1 R=$2
shift 2

setting() { sed -n "s/^$1 *= *//p" "$SETTINGS" 2>/dev/null | tail -1 ; }
[ -n "$PUBKEY$IDENTITY" ] || { IDENTITY=$(setting sys.update.signer.identity) ; ISSUER=$(setting sys.update.signer.issuer) ; }

SUMS=${R}_SHA256SUMS
BUNDLE=$SUMS.sigstore.json
SIGNER=${IDENTITY:-the holder of $PUBKEY}

# Everything said here goes to the console and, as one line, to the system log
log() { local pri=$1 ; shift ; logger -t hex_install -p user.$pri -- "$*" 2>/dev/null || true ; }
# Report one thing that does not check out, and go on
Problem()
{
    local what=$1
    {
        echo "WARNING: $what"
        shift
        for l in "$@" ; do echo "  $l" ; done
    } >&2
    log warning "update of $R NOT VERIFIED: $what${1:+ $1}"
}
# ... and, for a problem that leaves nothing more to check, finish
Unverified()
{
    Problem "$@"
    echo "  Continuing with the upgrade anyway. Do not reboot into it until the warning above is" >&2
    echo "  understood. This warning is recorded in the system log." >&2
    exit 0
}

if [ -z "$PUBKEY$IDENTITY" ] || [ -z "$PUBKEY" -a -z "$ISSUER" ] ; then
    cat >&2 <<EOF
WARNING: this system has no release signer configured (sys.update.signer.identity and
  .issuer in $SETTINGS), so it cannot check whether $R was released by its vendor.
  Continuing with the upgrade. This warning is recorded in the system log.
EOF
    log warning "update of $R not verified: no release signer configured"
    exit 0
fi

if [ ! -f "$DIR/$BUNDLE" ] ; then
    cat >&2 <<EOF
WARNING: $R is NOT signed.
  $DIR has no $BUNDLE
  next to it, so this system cannot confirm that the package was released by
  $SIGNER or that it has not been modified since.
  - If this is a signed release, copy its _SHA256SUMS and _SHA256SUMS.sigstore.json
    files into $DIR next to the .pkg and run the upgrade again.
  - If this is an intentionally unsigned build, such as a release candidate under
    test, you can ignore this warning.
  Continuing with the upgrade. This warning is recorded in the system log.
EOF
    log warning "update of $R is not signed (no $BUNDLE); installing anyway"
    exit 0
fi

# Signed from here on: anything that does not check out is reported as NOT VERIFIED
[ -f "$DIR/$SUMS" ] || Unverified "$R is signed, but its list of digests is missing." \
    "$DIR has $BUNDLE but no $SUMS." \
    "Copy $SUMS from the original source into $DIR and retry."
command -v cosign >/dev/null || Unverified "$R is signed, but this system has no cosign to check the signature with." \
    "Contact support."

if [ -n "$PUBKEY" ] ; then
    WHO=(--key "$PUBKEY")
else
    [ -f "$TRUSTED_ROOT" ] || Unverified "$R is signed, but this system has no Sigstore trust data ($TRUSTED_ROOT) to check it against." \
        "Contact support."
    WHO=(--certificate-identity "$IDENTITY" --certificate-oidc-issuer "$ISSUER" --trusted-root "$TRUSTED_ROOT")
fi
[ $TLOG -eq 1 ] || WHO+=(--insecure-ignore-tlog)

if ! OUT=$(cd "$DIR" && cosign verify-blob "${WHO[@]}" --bundle "$BUNDLE" "$SUMS" 2>&1) ; then
    WHY=$(echo "$OUT" | grep -v '^$' | tail -1)
    HINT=()
    # A system much older than the release may predate a rotation of Sigstore's certificate
    # authority: say so, rather than let a genuine release look tampered with. Only for a failure
    # in the certificate chain -- a signature by anyone else fails on the transparency log or the
    # identity, and must not be explained away as rotation.
    if [ -z "$PUBKEY" ] && echo "$OUT" | grep -q -i -E 'x509|certificate chain|unknown authority|verifying certificate|root certificate' ; then
        HINT=("This system's Sigstore trust data is from $(date -r "$TRUSTED_ROOT" +%Y-%m-%d). If it is older than" \
              "the release, upgrade through an intermediate release first, or contact support.")
    fi
    Unverified "the signature on $SUMS does not verify:" \
        "$WHY" \
        "The list of package digests was modified after signing, or was not signed by" \
        "$SIGNER." \
        "Download the release's _SHA256SUMS and _SHA256SUMS.sigstore.json again from the" \
        "original source and retry. If it still fails, contact support." \
        "${HINT[@]}"
fi

echo "Checking $# package file(s) of $R against the signed list of digests"
BAD=0
for F in "$@" ; do
    N=$(basename "$F")
    WANT=$(awk -v n="$N" '$2 == n { print $1 }' "$DIR/$SUMS")
    if [ -z "$WANT" ] ; then
        Problem "$N is not in the signed list of digests ($SUMS)." \
            "Only files listed there belong to this release; remove $N from $DIR."
        BAD=$((BAD + 1)) ; continue
    fi
    GOT=$(sha256sum < "$F" | cut -d' ' -f1)
    if [ "$GOT" != "$WANT" ] ; then
        Problem "$N does not match the signed list of digests." \
            "expected sha256 $WANT  ($SUMS, signed by $SIGNER)" \
            "actual   sha256 $GOT" \
            "The file is incomplete, corrupted or modified: copy it again from the original source."
        BAD=$((BAD + 1))
    fi
done
if [ $BAD -gt 0 ] ; then
    Unverified "$BAD of $# package file(s) of $R do not match its signed list of digests (above)." \
        "What gets installed is not exactly the signed release. Replace those files with copies" \
        "from the original source and run the upgrade again."
fi

INDEX=$(grep -oE '"logIndex": *"?[0-9]+' "$DIR/$BUNDLE" | head -1 | grep -oE '[0-9]+$' || true)
echo "Verified: $R ($# package file(s)) signed by $SIGNER${INDEX:+, Rekor entry $INDEX}."
log info "update of $R verified: $# file(s), signed by $SIGNER${INDEX:+, Rekor entry $INDEX}"
exit 0

#!/bin/bash
#
# Unit test for scripts/scanimages.py: one scan per unique image in a registry's storage, an
# index resolved to linux/amd64, blobs copied in as regular files, one grype database for every
# image, the image named in each match's location and in the SBOM, the rootfs inputs left as they
# are, and a missing manifest or blob, or a failed scan, stopping it. syft and grype are stand-ins
# that check what they are handed and record their arguments.
#
# Needs bash, coreutils and python3.
# Run: bash test_scanimages.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCAN="$DIR/../scanimages.py"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0 fail=0
chk(){ if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }
q(){ python3 -c "import json,sys; d=json.load(open(sys.argv[1])); print($2)" "$1"; }

mkdir -p "$T/bin"
cat > "$T/bin/syft" <<EOF
#!/bin/bash
# scan oci-dir:<layout> -q --source-name N --source-version V -o cyclonedx-json=<out>
L=\${2#oci-dir:}; out=\${9#cyclonedx-json=}
echo "\$5:\$7" >> "$T/syft.calls"
[ -f "\$L/index.json" ] || { echo "no index.json" >&2; exit 1; }
for b in "\$L"/blobs/sha256/*; do [ -f "\$b" ] && [ ! -L "\$b" ] || { echo "\$b is not a regular file" >&2; exit 1; }; done
ls "\$L/blobs/sha256" | wc -l >> "$T/syft.blobs"
[ "\$5" = "broken" ] && { echo "syft broke" >&2; exit 3; }
cat > "\$out" <<'J'
{"bomFormat":"CycloneDX","components":[{"bom-ref":"pkg-1","type":"library","name":"x","version":"1"},
 {"bom-ref":"os-1","type":"operating-system","name":"debian"}],"dependencies":[{"ref":"pkg-1","dependsOn":["os-1"]}]}
J
EOF
cat > "$T/bin/grype" <<EOF
#!/bin/bash
echo "\$* auto=\${GRYPE_DB_AUTO_UPDATE:-unset}" >> "$T/grype.calls"
[ "\$1" = db ] && exit 0
echo '{"matches":[{"vulnerability":{"id":"CVE-1","severity":"High"},"artifact":{"name":"x","locations":[{"path":"/usr/bin/x"}]}}],
 "ignoredMatches":[{"vulnerability":{"id":"CVE-2","severity":"Low"},"artifact":{"name":"x","locations":[{"path":"/usr/bin/x"}]}}],
 "distro":{"name":"debian","version":"12"}}'
EOF
chmod +x "$T/bin/syft" "$T/bin/grype"
export PATH="$T/bin:$PATH"

# A registry: a:1 and lib/a:1 share one docker v2 manifest, b:2 is an index whose linux/amd64
# entry is used. Blobs are content-addressed, as the real storage is.
V="$T/root/opt/docker/registry/docker/registry/v2"
put(){ local h; h=$(printf %s "$1" | sha256sum | cut -c1-64); mkdir -p "$V/blobs/sha256/${h:0:2}/$h"; printf %s "$1" > "$V/blobs/sha256/${h:0:2}/$h/data"; echo "sha256:$h"; }
tag(){ mkdir -p "$V/repositories/$1/_manifests/tags/$2/current"; echo -n "$3" > "$V/repositories/$1/_manifests/tags/$2/current/link"; }
CA=$(put config-a); L1=$(put layer-1); L2=$(put layer-2); CB=$(put config-b); L3=$(put layer-3)
MA=$(put "{\"schemaVersion\":2,\"mediaType\":\"application/vnd.docker.distribution.manifest.v2+json\",\"config\":{\"digest\":\"$CA\"},\"layers\":[{\"digest\":\"$L1\"},{\"digest\":\"$L2\"}]}")
MB=$(put "{\"schemaVersion\":2,\"mediaType\":\"application/vnd.oci.image.manifest.v1+json\",\"config\":{\"digest\":\"$CB\"},\"layers\":[{\"digest\":\"$L3\"}]}")
IX=$(put "{\"schemaVersion\":2,\"mediaType\":\"application/vnd.oci.image.index.v1+json\",\"manifests\":[{\"digest\":\"sha256:$(printf '0%.0s' {1..64})\",\"platform\":{\"os\":\"linux\",\"architecture\":\"arm64\"}},{\"digest\":\"$MB\",\"platform\":{\"os\":\"linux\",\"architecture\":\"amd64\"}}]}")
tag a 1 "$MA"; tag lib/a 1 "$MA"; tag b 2 "$IX"

echo '{"bomFormat":"CycloneDX","components":[{"bom-ref":"root-1","type":"library","name":"r"}],"dependencies":[]}' > "$T/sbom.json"
echo '{"matches":[{"vulnerability":{"id":"CVE-0","severity":"Critical"},"artifact":{"name":"r","locations":[{"path":"/usr/lib/r"}]}}],"descriptor":{"name":"grype"}}' > "$T/vuln.json"
echo '{}' > "$T/a.openvex.json"
before=$(sha256sum "$T/sbom.json" "$T/vuln.json")

scan(){ rm -f "$T/syft.calls" "$T/syft.blobs" "$T/grype.calls" "$T/out.sbom.json" "$T/out.vuln.json"
    OUT=$(python3 "$SCAN" --root "$T/root" --registry /opt/docker/registry --sbom "$T/sbom.json" --vuln "$T/vuln.json" \
        --out-sbom "$T/out.sbom.json" --out-vuln "$T/out.vuln.json" --work "$T/work" "$@" 2>&1); RC=$?; }

scan --vex "$T/a.openvex.json"
chk "exits 0" "$RC" "0"
chk "one scan per unique image" "$(sort "$T/syft.calls" | tr '\n' ' ')" "a:1 b:2 "
chk "each layout carries manifest, config and layers" "$(tr '\n' ' ' < "$T/syft.blobs")" "4 3 "
chk "database updated once, first" "$(head -1 "$T/grype.calls")" "db update auto=unset"
chk "images judged with the database held" "$(grep -c 'auto=false' "$T/grype.calls")" "2"
chk "VEX reaches grype" "$(grep -c -- "--vex $T/a.openvex.json" "$T/grype.calls")" "2"
chk "work dir removed" "$([ -e "$T/work" ] && echo left || echo gone)" "gone"
chk "inputs unchanged" "$(sha256sum "$T/sbom.json" "$T/vuln.json")" "$before"

chk "report keeps the rootfs match first" "$(q "$T/out.vuln.json" "d['matches'][0]['artifact']['locations'][0]['path']")" "/usr/lib/r"
chk "report adds each image's matches" "$(q "$T/out.vuln.json" "len(d['matches']), len(d['ignoredMatches'])")" "3 2"
chk "image match names its image" "$(q "$T/out.vuln.json" "sorted(m['artifact']['locations'][0]['path'] for m in d['matches'][1:])")" \
    "['/opt/docker/registry/a:1!/usr/bin/x', '/opt/docker/registry/b:2!/usr/bin/x']"
chk "descriptor lists both tags of a shared image" "$(q "$T/out.vuln.json" "[i['tags'] for i in d['descriptor']['images']]")" "[['a:1', 'lib/a:1'], ['b:2']]"
chk "an index resolves to its amd64 manifest" "$(q "$T/out.vuln.json" "d['descriptor']['images'][1]['digest']")" "$MB"
chk "descriptor counts severities" "$(q "$T/out.vuln.json" "d['descriptor']['images'][0]['severities']")" "{'High': 1}"

chk "SBOM keeps the rootfs and adds a container per image" "$(q "$T/out.sbom.json" "[c['type'] for c in d['components']]")" "['library', 'container', 'container']"
chk "image components nest under their container" "$(q "$T/out.sbom.json" "[len(c.get('components', [])) for c in d['components']]")" "[0, 2, 2]"
chk "bom-refs stay unique" "$(q "$T/out.sbom.json" "(lambda r: len(r) == len(set(r)))([c['bom-ref'] for c in d['components']] + [s['bom-ref'] for c in d['components'] for s in c.get('components', [])])")" "True"
chk "dependencies follow the prefixed refs" "$(q "$T/out.sbom.json" "all(x['ref'].startswith('image:') and all(y.startswith('image:') for y in x['dependsOn']) for x in d['dependencies'])")" "True"

# Failures stop the scan and leave no output behind.
mv "$V/repositories/b" "$T/b.saved"
H=${L1#sha256:}; mv "$V/blobs/sha256/${H:0:2}/$H/data" "$T/l1.saved"
scan; chk "missing blob: fails" "$RC" "1"; chk "missing blob: named" "$(grep -c "blob $L1 is missing" <<<"$OUT")" "1"
chk "missing blob: no report" "$([ -e "$T/out.vuln.json" ] && echo written || echo none)" "none"
mv "$T/l1.saved" "$V/blobs/sha256/${H:0:2}/$H/data"; mv "$T/b.saved" "$V/repositories/b"

tag broken 9 "$MB"
scan; chk "failed syft: fails" "$RC" "1"; chk "failed syft: reported" "$(grep -c 'syft failed (3): syft broke' <<<"$OUT")" "1"
chk "failed syft: no SBOM" "$([ -e "$T/out.sbom.json" ] && echo written || echo none)" "none"
rm -rf "$V/repositories/broken"

python3 - "$V" "$IX" <<'EOF'
import json, os, sys
v, ix = sys.argv[1], sys.argv[2].split(':')[1]
p = os.path.join(v, 'blobs/sha256', ix[:2], ix, 'data')
d = json.load(open(p)); d['manifests'] = [m for m in d['manifests'] if m['platform']['architecture'] != 'amd64']
open(p, 'w').write(json.dumps(d))
EOF
scan; chk "index without amd64: fails" "$RC" "1"; chk "index without amd64: named" "$(grep -c 'no linux/amd64 manifest' <<<"$OUT")" "1"

echo "test_scanimages: $pass passed, $fail failed"
[ "$fail" -eq 0 ]

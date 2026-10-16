#!/bin/bash
#
# Unit test for scripts/fixgoversions.py: which Go main modules take their rpm's version, that the
# purl and CPEs follow, and that bundled, non-rpm, versioned and dependency entries are untouched.
# Self-contained: the binaries are files holding a synthetic Go build info blob, and rpm is a stub.
#
# Needs bash and python3.
# Run: bash test_fixgoversions.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT="$DIR/../fixgoversions.py"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
R=$T/root

# A file holding a Go 1.18+ build info blob whose main module is $2 at version $3. $4 = decoy puts
# the magic in as a plain string first, as a binary that imports debug/buildinfo has it.
gobin(){ mkdir -p "$(dirname "$R$1")"; python3 - "$R$1" "$2" "$3" "${4:-}" <<'EOF'
import sys
path, mod, ver, decoy = sys.argv[1:]
def uv(n):
    out = b''
    while True:
        b = n & 0x7f; n >>= 7
        out += bytes([b | (0x80 if n else 0)])
        if not n: return out
def s(b): return uv(len(b)) + b
info = (bytes.fromhex('3077af0c9274080241e1c107e6d618e6')
        + f'path\t{mod}/cmd\nmod\t{mod}\t{ver}\t\ndep\tgolang.org/x/net\tv0.61.0\th1:x=\n'.encode()
        + bytes.fromhex('f932433186182072008242104116d8f2'))
hdr = b'\xff Go buildinf:' + bytes([8, 2]) + b'\0' * 16
blob = (b'xx\xff Go buildinf:not this one' if decoy else b'') + b'\0' * 64 + hdr + s(b'go1.26.9') + s(info) + b'\0' * 64
open(path, 'wb').write(blob)
EOF
}
gobin /usr/share/grafana/bin/grafana github.com/grafana/grafana '(devel)' decoy
gobin /usr/share/grafana/bin/grafana-server github.com/grafana/grafana '(devel)'
gobin /usr/libexec/docker/cli-plugins/docker-compose github.com/docker/compose/v5 '(devel)'
gobin /usr/bin/runc github.com/opencontainers/runc '(devel)'
gobin /usr/local/bin/lachesis github.com/bigstack-oss/lachesis '(devel)'
gobin /usr/bin/kapacitord github.com/influxdata/kapacitor v1.8.8
gobin /usr/bin/odd github.com/example/odd '(devel)'

# rpm -qf stub: path -> "NAME VERSION"; anything not listed is owned by no package
mkdir -p "$T/bin"
cat > "$T/bin/rpm" <<'EOF'
#!/bin/bash
case "${@: -1}" in
/usr/share/grafana/bin/grafana*) echo "grafana-enterprise 12.4.13" ;;
/usr/libexec/docker/cli-plugins/docker-compose) echo "docker-compose-plugin 5.6.0" ;;
/usr/bin/runc) echo "containerd.io 2.4.2" ;;
/usr/bin/kapacitord) echo "kapacitor 1.8.9" ;;
/usr/bin/odd) echo "odd 1.0~rc1" ;;
*) echo "file ${@: -1} is not owned by any package"; exit 1 ;;
esac
EOF
chmod +x "$T/bin/rpm"
export PATH="$T/bin:$PATH"

# syft's entries: name, version, location, and whether it is the binary's main module
comp(){ echo '{"name": "'$1'", "version": "'$2'", "purl": "pkg:golang/'$1'@'$2'", "cpe": "cpe:2.3:a:x:'${1##*/}':'$2':*:*:*:*:*:*:*", "properties": [{"name": "syft:package:foundBy", "value": "go-module-binary-cataloger"}, {"name": "syft:location:0:path", "value": "'$3'"}, {"name": "syft:metadata:mainModule", "value": "'$4'"}, {"name": "syft:cpe23", "value": "cpe:2.3:a:x:'${1##*/}':'$2':*:*:*:*:*:*:*"}]}'; }
cat > "$T/sbom.json" <<EOF
{"bomFormat": "CycloneDX", "components": [
 $(comp github.com/grafana/grafana v3.53.2 /usr/share/grafana/bin/grafana github.com/grafana/grafana),
 $(comp github.com/grafana/grafana UNKNOWN /usr/share/grafana/bin/grafana-server github.com/grafana/grafana),
 $(comp golang.org/x/net v0.61.0 /usr/share/grafana/bin/grafana github.com/grafana/grafana),
 $(comp github.com/docker/compose/v5 UNKNOWN /usr/libexec/docker/cli-plugins/docker-compose github.com/docker/compose/v5),
 $(comp github.com/opencontainers/runc v1.5.2-0-g29dd3dc2 /usr/bin/runc github.com/opencontainers/runc),
 $(comp github.com/bigstack-oss/lachesis UNKNOWN /usr/local/bin/lachesis github.com/bigstack-oss/lachesis),
 $(comp github.com/influxdata/kapacitor v1.8.8 /usr/bin/kapacitord github.com/influxdata/kapacitor),
 $(comp github.com/example/odd UNKNOWN /usr/bin/odd github.com/example/odd),
 {"name": "openssl", "version": "1:3.5.8-2.el9_8", "purl": "pkg:rpm/redhat/openssl@3.5.8-2.el9_8"}
]}
EOF
cp "$T/sbom.json" "$T/orig.json"

out=$(python3 "$SCRIPT" "$T/sbom.json" --root "$R" 2>&1)

pass=0 fail=0
chk() { if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }
get() { python3 -c "
import json,sys
d=json.load(open('$T/sbom.json'))
c=[c for c in d['components'] if c['name']=='$1' and any(p['value']=='$2' for p in c.get('properties',[]))][0]
print(eval(sys.argv[1]))" "$3"; }

# 1. a guessed version, the module's own rpm: the rpm's version, with purl and CPEs following
chk "grafana version" "$(get github.com/grafana/grafana /usr/share/grafana/bin/grafana "c['version']")" v12.4.13
chk "grafana purl" "$(get github.com/grafana/grafana /usr/share/grafana/bin/grafana "c['purl']")" pkg:golang/github.com/grafana/grafana@v12.4.13
chk "grafana cpe" "$(get github.com/grafana/grafana /usr/share/grafana/bin/grafana "c['cpe']")" "cpe:2.3:a:x:grafana:v12.4.13:*:*:*:*:*:*:*"
chk "grafana syft:cpe23" "$(get github.com/grafana/grafana /usr/share/grafana/bin/grafana "[p['value'] for p in c['properties'] if p['name']=='syft:cpe23'][0]")" "cpe:2.3:a:x:grafana:v12.4.13:*:*:*:*:*:*:*"
# 2. UNKNOWN, and a /vN module path against a -plugin rpm
chk "grafana-server" "$(get github.com/grafana/grafana /usr/share/grafana/bin/grafana-server "c['version']")" v12.4.13
chk "compose" "$(get github.com/docker/compose/v5 /usr/libexec/docker/cli-plugins/docker-compose "c['version']")" v5.6.0
# 3. a dependency of a fixed binary is not its main module
chk "x/net untouched" "$(get golang.org/x/net /usr/share/grafana/bin/grafana "c['version']")" v0.61.0
# 4. a binary its rpm only bundles keeps syft's version
chk "runc untouched" "$(get github.com/opencontainers/runc /usr/bin/runc "c['version']")" v1.5.2-0-g29dd3dc2
# 5. no owning rpm
chk "lachesis untouched" "$(get github.com/bigstack-oss/lachesis /usr/local/bin/lachesis "c['version']")" UNKNOWN
# 6. build info carries a real version: the binary says what it is, even when the rpm differs
chk "kapacitor untouched" "$(get github.com/influxdata/kapacitor /usr/bin/kapacitord "c['version']")" v1.8.8
# 7. an rpm version that is no module version
chk "odd untouched" "$(get github.com/example/odd /usr/bin/odd "c['version']")" UNKNOWN
# 8. the report names each change, and nothing else in the SBOM moved
chk "changes reported" "$(echo "$out" | grep -c ' -> v')" 3
chk "summary" "$(echo "$out" | tail -1)" "3 Go main module version(s) taken from their rpm"
chk "rest identical" "$(python3 -c "
import json
a=json.load(open('$T/orig.json'))['components']; b=json.load(open('$T/sbom.json'))['components']
print(sum(1 for x,y in zip(a,b) if x!=y))")" 3

# 9. nothing to fix: the SBOM is not rewritten
cp "$T/sbom.json" "$T/again.json"; touch -d '2000-01-01' "$T/again.json"
python3 "$SCRIPT" "$T/again.json" --root "$R" >/dev/null 2>&1
chk "idempotent" "$(stat -c %Y "$T/again.json")" "$(date -d '2000-01-01' +%s)"

echo "pass=$pass fail=$fail"
[ $fail -eq 0 ]

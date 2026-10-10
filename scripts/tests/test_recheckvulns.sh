#!/bin/bash
#
# Unit test for scripts/recheckvulns.py: which kernel matches the kernel CNA's records move to
# ignoredMatches, the fix and severity of the ones that stay, and that nothing else is touched.
# Self-contained: vulns.git is a local repo and Red Hat's security data a file:// directory.
#
# Needs bash, git and python3.
# Run: bash test_recheckvulns.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT="$DIR/../recheckvulns.py"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT

# The CNA's records, in the shapes vulns.git uses
mkdir -p "$T/vulns/cve/published/2026" "$T/rh/cve"
cna(){ cat > "$T/vulns/cve/published/2026/$1.json" <<EOF
{"containers": {"cna": {"providerMetadata": {"orgId": "f4215fc3-5b6b-47ff-a258-f7189bd81038"}, $2}}}
EOF
}
main(){ echo '{"defaultStatus": "affected", "versions": [{"version": "'$1'", "status": "affected"}, {"version": "0", "lessThan": "'$1'", "status": "unaffected", "versionType": "semver"}'$2', {"version": "'$3'", "lessThanOrEqual": "*", "status": "unaffected", "versionType": "original_commit_for_fix"}]}'; }
fix(){ echo ', {"version": "'$1'", "lessThanOrEqual": "'${1%.*}'.*", "status": "unaffected", "versionType": "semver"}'; }
win(){ echo '{"version": "'$1'", "lessThan": "'$2'", "status": "affected", "versionType": "semver"}'; }

cna CVE-2026-0001 '"affected": ['"$(main 6.1 "$(fix 6.6.90)$(fix 6.12.100)" 6.14)"']'
cna CVE-2026-0002 '"affected": ['"$(main 6.14 "" 6.16)"']'
cna CVE-2026-0003 '"affected": ['"$(main 5.10 "$(fix 6.12.113)" 6.17)"']'
cna CVE-2026-0004 '"affected": ['"$(main 6.6 "" 6.18)"'], "metrics": [{"cvssV3_1": {"version": "3.1", "vectorString": "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H", "baseScore": 9.8, "baseSeverity": "CRITICAL"}}]'
# Fixed in mainline before 6.12 branched
cna CVE-2026-0005 '"affected": ['"$(main 5.15 "$(fix 6.6.50)" 6.11)"']'
# Only backports carry it: 6.12 from the start of history to 6.12.100, 6.18 to 6.18.54
cna CVE-2026-0006 '"affected": [{"defaultStatus": "unaffected", "versions": ['"$(win 0 6.12.100), $(win 0 6.18.54)"']}]'
# A backport brought it into 6.12.105, fixed in 6.12.115
cna CVE-2026-0008 '"affected": [{"defaultStatus": "unaffected", "versions": ['"$(win 6.12.105 6.12.115)"']}]'
git -C "$T/vulns" init -q -b master && git -C "$T/vulns" add -A &&
    git -C "$T/vulns" -c user.name=t -c user.email=t@t -c commit.gpgsign=false commit -q -m one
export KERNEL_VULNS_GIT="file://$T/vulns" REDHAT_SECDATA="file://$T/rh"

echo '{"threat_severity": "Moderate", "cvss3": {"cvss3_base_score": "5.5", "cvss3_scoring_vector": "CVSS:3.1/AV:L/AC:L/PR:L/UI:N/S:U/C:N/I:N/A:H", "status": "verified"}}' > "$T/rh/cve/CVE-2026-0003.json"
echo '{"threat_severity": "Important"}' > "$T/rh/cve/CVE-2026-0007.json"

# grype's report: the vmlinuz (NVD), the kernel rpms (Red Hat's 5.14 data) and bystanders
m(){ echo '{"vulnerability": {"id": "'$1'", "namespace": "'$2'", "severity": "'$3'", "cvss": [{"source": "nvd@nist.gov", "type": "Primary", "version": "3.1", "vector": "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H", "metrics": {"baseScore": 9.8}}], "fix": {"versions": ["6.18.5"], "state": "fixed"}}, "artifact": {"name": "'$4'", "version": "'$5'", "type": "'$6'"}}'; }
K='linux-kernel 6.12.110-1.el9.x86_64 linux-kernel'
report(){ cat > "$T/vuln.json" <<EOF
{"descriptor": {"name": "grype"}, "matches": [
 $(m CVE-2026-0001 nvd:cpe High $K), $(m CVE-2026-0002 nvd:cpe High $K), $(m CVE-2026-0003 nvd:cpe Critical $K),
 $(m CVE-2026-0004 nvd:cpe High $K), $(m CVE-2026-0005 nvd:cpe High $K), $(m CVE-2026-0006 nvd:cpe High $K),
 $(m CVE-2026-0007 nvd:cpe Critical $K), $(m CVE-2026-0008 nvd:cpe High $K),
 $(m CVE-2026-0001 redhat:distro:redhat:9 Medium kernel-core 6.12.110-1.el9 rpm),
 $(m CVE-2026-0003 redhat:distro:redhat:9 Medium kernel-modules 6.12.110-1.el9 rpm),
 $(m CVE-2026-0007 redhat:distro:redhat:9 Medium kernel-modules 6.12.110-1.el9 rpm),
 $(m CVE-2026-0009 redhat:distro:redhat:9 Medium kernel-core 6.12.110-1.el9 rpm),
 $(m CVE-2026-0001 redhat:distro:redhat:9 Medium kernel-headers 5.14.0-611.el9 rpm),
 $(m CVE-2026-0001 redhat:distro:redhat:9 Medium perf 5.14.0-611.el9 rpm),
 $(m CVE-2026-0003 nvd:cpe Critical openssl 3.5.1 binary)]}
EOF
}
q(){ python3 -c "import json, sys; r = json.load(open('$T/vuln.json')); print($1)"; }
kept="' '.join(m['vulnerability']['id'] + '/' + m['artifact']['name'] for m in r['matches'])"
why(){ q "next(m['appliedIgnoreRules'][0]['reason'].split(' (vulns.git')[0] for m in r['ignoredMatches'] if m['vulnerability']['id'] == '$1' and m['artifact']['name'] == '$2')"; }
# get <id> <package> <python over v, the match's vulnerability>: one value, or several space-separated
get(){ q "' '.join(map(str, next(($3,) for m in r['matches'] if m['vulnerability']['id'] == '$1' and m['artifact']['name'] == '$2' for v in [m['vulnerability']])))"; }

pass=0 fail=0
chk(){ if [ "$2" = "$3" ]; then pass=$((pass+1)); else fail=$((fail+1)); echo "FAIL: $1: got [$2] want [$3]"; fi; }
run(){ OUT=$(python3 "$SCRIPT" "$@" 2>&1); RC=$?; }

report; run "$T/vuln.json" "$T/checkout"; chk "first run clones and exits 0" "$RC" "0"
chk "kept" "$(q "$kept")" "CVE-2026-0003/linux-kernel CVE-2026-0004/linux-kernel CVE-2026-0007/linux-kernel CVE-2026-0008/linux-kernel CVE-2026-0003/kernel-modules CVE-2026-0007/kernel-modules CVE-2026-0001/kernel-headers CVE-2026-0001/perf CVE-2026-0003/openssl"
chk "fixed on the branch" "$(why CVE-2026-0001 linux-kernel)" "kernel CNA: fixed in 6.12.100"
chk "  the rpm too" "$(why CVE-2026-0001 kernel-core)" "kernel CNA: fixed in 6.12.100"
chk "introduced later" "$(why CVE-2026-0002 linux-kernel)" "kernel CNA: introduced in 6.14"
chk "fixed in mainline before 6.12" "$(why CVE-2026-0005 linux-kernel)" "kernel CNA: fixed in 6.11"
chk "another branch's range" "$(why CVE-2026-0006 linux-kernel)" "kernel CNA: affected only in the stable releases it lists, not this one"
chk "fix on the branch" "$(get CVE-2026-0003 linux-kernel "v['fix']")" "{'versions': ['6.12.113'], 'state': 'fixed'}"
chk "no fix on the branch" "$(get CVE-2026-0004 linux-kernel "v['fix']")" "{'versions': [], 'state': 'not-fixed'}"
chk "a backport's range" "$(get CVE-2026-0008 linux-kernel "v['fix']['versions']")" "['6.12.115']"
chk "no CNA record: fix untouched" "$(get CVE-2026-0007 linux-kernel "v['fix']['versions']")" "['6.18.5']"
chk "  an rpm NVD matches on the image too stays" "$(get CVE-2026-0007 kernel-modules "v['fix']['versions']")" "['6.18.5']"
chk "  one it does not is ignored" "$(why CVE-2026-0009 kernel-core)" "NVD: no match on the kernel image, and no kernel CNA record"
chk "Red Hat rates first" "$(get CVE-2026-0003 linux-kernel "v['severity']")" "Medium"
chk "  its score goes first" "$(get CVE-2026-0003 linux-kernel "v['cvss'][0]['source'], v['cvss'][0]['metrics']['baseScore']")" "secalert@redhat.com 5.5"
chk "  the rpm too" "$(get CVE-2026-0003 kernel-modules "v['severity']")" "Medium"
chk "Important is High" "$(get CVE-2026-0007 linux-kernel "v['severity']")" "High"
chk "then the CNA" "$(get CVE-2026-0004 linux-kernel "v['severity'], v['cvss'][0]['source']")" "Critical f4215fc3-5b6b-47ff-a258-f7189bd81038"
chk "then NVD" "$(get CVE-2026-0008 linux-kernel "v['severity'], len(v['cvss'])")" "High 1"
chk "RHEL's 5.14 rpm untouched" "$(get CVE-2026-0001 kernel-headers "v['severity']")" "Medium"
chk "not a kernel: untouched" "$(get CVE-2026-0003 openssl "v['severity'], v['fix']['versions']")" "Critical ['6.18.5']"
chk "commit recorded" "$(q "r['descriptor']['kernelRecheck']['cna']['commit']")" "$(git -C "$T/vulns" rev-parse HEAD)"
chk "summary" "$OUT" "kernel matches: 5 ignored by the CNA, 1 by NVD, 6 kept; rated by Red Hat 4, by the CNA 1, by NVD 1"

# The next scan fetches what changed: CVE-2026-0003 is now fixed in 6.12.105
cna CVE-2026-0003 '"affected": ['"$(main 5.10 "$(fix 6.12.105)" 6.17)"']'
git -C "$T/vulns" -c user.name=t -c user.email=t@t -c commit.gpgsign=false commit -q -am two
report; run "$T/vuln.json" "$T/checkout"; chk "second run updates and exits 0" "$RC" "0"
chk "  by the new record" "$(why CVE-2026-0003 linux-kernel)" "kernel CNA: fixed in 6.12.105"
chk "  checkout keeps only the records" "$(cd "$T/checkout" && git ls-files | grep -vc '^cve/published/.*\.json$')" "0"

report; cp "$T/vuln.json" "$T/before.json"
REDHAT_SECDATA=http://127.0.0.1:9 run "$T/vuln.json" "$T/checkout"; chk "Red Hat unreachable fails" "$RC" "1"
chk "  and says so" "$(grep -c 'Red Hat security data unreachable' <<<"$OUT")" "1"
chk "  report untouched" "$(cmp -s "$T/before.json" "$T/vuln.json" && echo same)" "same"

KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout2"; chk "vulns.git unreachable fails" "$RC" "1"
chk "  and says so" "$(grep -c "cannot fetch the kernel CNA records from file://$T/nowhere" <<<"$OUT")" "1"
chk "  report untouched" "$(cmp -s "$T/before.json" "$T/vuln.json" && echo same)" "same"

# No kernel image, nothing to judge: no fetch, and the report is left byte for byte
echo '{"matches":[]}' > "$T/vuln.json"
KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout3"; chk "no kernel image exits 0" "$RC" "0"
chk "  report untouched" "$(cat "$T/vuln.json")" '{"matches":[]}'
chk "  nothing fetched" "$([ -e "$T/checkout3" ] && echo yes || echo no)" "no"

run "$T/vuln.json"; chk "usage error with one arg" "$RC" "2"

# RHEL python rpms (--sbom): the SBOM's rpms, Red Hat's records and the rpms' changelogs
c(){ echo '{"type": "library", "name": "'$1'", "version": "'$2'", "publisher": "'$3'", "purl": "pkg:rpm/centos/'$1'@'$2'", "properties": [{"name": "syft:metadata:sourceRpm", "value": "'$4'-'$2'.src.rpm"}]}'; }
cat > "$T/sbom.json" <<EOF
{"components": [$(c python3-urllib3 1.26.5-8.el9 CentOS python-urllib3), $(c python3-mako 1.1.4-6.el9 CentOS python-mako),
 $(c python3-setuptools 65.5.1-2.el9s "CentOS Cloud SIG" python-setuptools), $(c python3-protobuf 3.14.0-17.el9 CentOS protobuf),
 $(c python3-lxml 4.6.5-3.el9 CentOS python-lxml), $(c python3-jinja2 3.0.1-2.el9s CentOS python-jinja2),
 {"type": "library", "name": "urllib3", "version": "1.26.5", "purl": "pkg:pypi/urllib3@1.26.5"}]}
EOF
rel(){ echo '{"product_name": "'"${3:-Red Hat Enterprise Linux 9}"'", "package": "'$1'", "advisory": "'$2'"}'; }
st(){ echo '{"product_name": "Red Hat Enterprise Linux 9", "package_name": "'$1'", "fix_state": "'$2'"}'; }
echo '{"threat_severity": "Important", "affected_release": ['"$(rel python-urllib3-0:1.26.5-6.el9_7.1 RHSA-2026:1)"']}' > "$T/rh/cve/CVE-2026-1001.json"
echo '{"threat_severity": "Important", "affected_release": ['"$(rel python-urllib3-0:1.26.5-8.el9_8 RHSA-2026:2)"']}' > "$T/rh/cve/CVE-2026-1002.json"
echo '{"threat_severity": "Important", "cvss3": {"cvss3_base_score": "7.5", "cvss3_scoring_vector": "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:H"}}' > "$T/rh/cve/CVE-2026-1003.json"
echo '{"threat_severity": "Moderate", "package_state": ['"$(st python-mako 'Not affected')"']}' > "$T/rh/cve/CVE-2026-1004.json"
echo '{"threat_severity": "Important", "affected_release": ['"$(rel python-setuptools-0:53.0.0-12.el9_4.1 RHSA-2026:5)"']}' > "$T/rh/cve/CVE-2026-1005.json"
echo '{"threat_severity": "Moderate", "affected_release": ['"$(rel protobuf-0:3.14.0-13.el9_2.1 RHSA-2026:6 'Red Hat Enterprise Linux 9.2 Extended Update Support')"']}' > "$T/rh/cve/CVE-2026-1006.json"
echo '{"threat_severity": "Moderate", "package_state": ['"$(st python-lxml 'Fix deferred')"']}' > "$T/rh/cve/CVE-2026-1007.json"
# The Cloud SIG ships its jinja2 as "CentOS": only the el9s dist tag says it is not RHEL's 2.11.3
echo '{"threat_severity": "Important", "package_state": ['"$(st python-jinja2 'Not affected')"']}' > "$T/rh/cve/CVE-2026-1008.json"
mkdir -p "$T/bin" "$T/root"
printf '* Wed Jun 03 2026 Packager - 1.26.5-8\n- Security fix for CVE-2026-1002 and CVE-2026-10020\n' > "$T/changelog"
printf '#!/bin/bash\n[ "$1 $2 $3 $4" = "--root %s -q --changelog" ] || exit 2\n[ "$5" = python3-urllib3 ] && cat "%s/changelog"\n' "$T/root" "$T" > "$T/bin/rpm"; chmod +x "$T/bin/rpm"
SITE=/usr/lib/python3.9/site-packages
p(){ echo '{"vulnerability": {"id": "'$1'", "namespace": "github:language:python", "severity": "High", "cvss": [], "fix": {"versions": ["9.9"], "state": "fixed"}}, "relatedVulnerabilities": [{"id": "'$2'"}], "artifact": {"name": "'$3'", "version": "'$4'", "type": "python", "locations": [{"path": "'$5'/'$3'-'$4'.dist-info/METADATA"}]}}'; }
pyreport(){ cat > "$T/vuln.json" <<EOF
{"descriptor": {"name": "grype"}, "matches": [
 $(p GHSA-0001 CVE-2026-1001 urllib3 1.26.5 $SITE), $(p GHSA-0002 CVE-2026-1002 urllib3 1.26.5 $SITE),
 $(p CVE-2026-1003 CVE-2026-1003 urllib3 1.26.5 $SITE), $(p GHSA-0004 CVE-2026-1004 Mako 1.1.4.dev0 $SITE),
 $(p GHSA-0005 CVE-2026-1005 setuptools 65.5.1 $SITE), $(p GHSA-0006 CVE-2026-1006 protobuf 3.14.0 /usr/lib64/python3.9/site-packages),
 $(p GHSA-0007 CVE-2026-1007 lxml 4.6.5 /usr/lib64/python3.9/site-packages), $(p GHSA-0008 CVE-2026-1008 Jinja2 3.0.1 $SITE),
 $(p GHSA-0001 CVE-2026-1001 urllib3 1.26.5 /usr/local/lib/python3.9/site-packages)]}
EOF
}
pkept="' '.join(m['vulnerability']['id'] + '/' + m['artifact']['locations'][0]['path'].split('/')[2] for m in r['matches'])"
pwhy(){ q "next(m['appliedIgnoreRules'][0]['reason'] for m in r['ignoredMatches'] if m['vulnerability']['id'] == '$1')"; }
pget(){ q "' '.join(map(str, next(($2,) for m in r['matches'] if m['vulnerability']['id'] == '$1' and '/local/' not in m['artifact']['locations'][0]['path'] for v in [m['vulnerability']])))"; }

pyreport; PATH="$T/bin:$PATH" KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout4" --sbom "$T/sbom.json" --root "$T/root"
chk "python pass exits 0 with no kernel" "$RC" "0"
chk "  no kernel fetch" "$([ -e "$T/checkout4" ] && echo yes || echo no)" "no"
chk "  kept" "$(q "$pkept")" "CVE-2026-1003/lib GHSA-0005/lib GHSA-0006/lib64 GHSA-0007/lib64 GHSA-0008/lib GHSA-0001/local"
chk "  a Red Hat fix this release sorts above" "$(pwhy GHSA-0001)" "Red Hat: fixed in python-urllib3-0:1.26.5-6.el9_7.1 (RHSA-2026:1), this build is 1.26.5-8.el9"
chk "  a z-stream rebuild above it, cleared by the changelog" "$(pwhy GHSA-0002)" "rpm changelog: python-urllib3-1.26.5-8.el9 backports CVE-2026-1002"
chk "  not affected, through a .dev0 dist-info" "$(pwhy GHSA-0004)" "Red Hat: python-mako is not affected in Red Hat Enterprise Linux 9"
chk "  no fix: rated by Red Hat" "$(pget CVE-2026-1003 "v['severity'], v['cvss'][0]['metrics']['baseScore']")" "High 7.5"
chk "  a SIG build keeps grype's verdict" "$(pget GHSA-0005 "v['severity'], len(v['cvss'])")" "High 0"
chk "  so does one the SIG publishes as CentOS" "$(pget GHSA-0008 "v['severity'], len(v['cvss'])")" "High 0"
chk "  an EUS-only fix does not count" "$(pget GHSA-0006 "v['severity']")" "Medium"
chk "  fix deferred stays, rated" "$(pget GHSA-0007 "v['severity']")" "Medium"
chk "  /usr/local untouched" "$(q "next(m['vulnerability']['severity'] for m in r['matches'] if '/local/' in m['artifact']['locations'][0]['path'])")" "High"
chk "  descriptor" "$(q "r['descriptor']['backportRecheck']['ignored'], r['descriptor']['backportRecheck']['changelogs']")" "{'Red Hat': 2, 'changelog': 1} True"
chk "  summary" "$OUT" "RHEL python rpm matches: 2 ignored by Red Hat, 1 by the rpm changelog, 3 kept; rated by Red Hat 3, by grype 0"

pyreport; PATH="$T/bin:$PATH" KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout4" --sbom "$T/sbom.json"
chk "without --root no changelog clears it" "$(q "'GHSA-0002' in [m['vulnerability']['id'] for m in r['matches']]")" "True"

pyreport; cp "$T/vuln.json" "$T/before.json"
REDHAT_SECDATA=http://127.0.0.1:9 KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout4" --sbom "$T/sbom.json"
chk "python pass: Red Hat unreachable fails" "$RC" "1"
chk "  report untouched" "$(cmp -s "$T/before.json" "$T/vuln.json" && echo same)" "same"

vc(){ python3 -B -c "import importlib.util as u; s = u.spec_from_file_location('r', '$SCRIPT'); m = u.module_from_spec(s); s.loader.exec_module(m); print(m.vercmp('$1', '$2'))"; }
chk "vercmp newer base release" "$(vc 8.el9 6.el9_7.1)" "1"
chk "vercmp z-stream rebuild sorts above" "$(vc 8.el9 8.el9_8)" "-1"
chk "vercmp equal" "$(vc 6.el9 6.el9)" "0"
chk "vercmp numeric segments" "$(vc 17.el9 9.el9)" "1"
chk "vercmp tilde before release" "$(vc 1.0~rc1 1.0)" "-1"
chk "vercmp caret after release" "$(vc 1.0^git1 1.0)" "1"
chk "vercmp caret before a later release" "$(vc 1.0^git1 1.0.1)" "-1"

echo "test_recheckvulns: $pass passed, $fail failed"
[ "$fail" -eq 0 ]

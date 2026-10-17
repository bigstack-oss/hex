#!/bin/bash
#
# Unit test for scripts/recheckvulns.py: which kernel matches the kernel CNA's records move to
# ignoredMatches, the fix and severity of the ones that stay, and that nothing else is touched;
# likewise for RHEL python rpms by Red Hat's records, OpenStack's packages by the OSSAs, and Go modules
# at a pseudo-version by their repository's history. Self-contained: vulns.git, ossa.git and the Go
# module's repository are local repos, Red Hat's security data a file:// directory, and grype a
# stand-in that answers from files.
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
rc(){ echo '{"type": "library", "name": "'$1'", "version": "'$2'", "publisher": "'$3'", "purl": "pkg:rpm/centos/'$1'@'$2'", "properties": [{"name": "syft:metadata:sourceRpm", "value": "'$4'.src.rpm"}]}'; }
cat > "$T/sbom.json" <<EOF
{"components": [$(c python3-urllib3 1.26.5-8.el9 CentOS python-urllib3), $(c python3-mako 1.1.4-6.el9 CentOS python-mako),
 $(c python3-setuptools 65.5.1-2.el9s "CentOS Cloud SIG" python-setuptools), $(c python3-protobuf 3.14.0-17.el9 CentOS protobuf),
 $(c python3-lxml 4.6.5-3.el9 CentOS python-lxml), $(c python3-jinja2 3.0.1-2.el9s CentOS python-jinja2),
 $(rc nginx-core 2:1.20.1-32.el9 CentOS nginx-1.20.1-32.el9), $(rc conmon 3:2.2.1-2.el9 CentOS conmon-2.2.1-2.el9),
 $(rc kernel-core 6.12.110-1.el9 CentOS kernel-6.12.110-1.el9), $(rc openvswitch3.3 3.3.4-1.el9s CentOS openvswitch3.3-3.3.4-1.el9s),
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
# The rpms themselves: nginx measured against a module stream though RHEL fixed its own line (twice,
# both below this build), conmon Red Hat calls not affected, and a kernel and a SIG rpm left alone
echo '{"threat_severity": "Important", "affected_release": ['"$(rel nginx-1:1.20.1-14.el9_2.1 RHSA-2026:11), $(rel nginx-2:1.20.1-24.el9_7.2 RHSA-2026:12)"']}' > "$T/rh/cve/CVE-2026-1010.json"
echo '{"threat_severity": "Moderate", "package_state": ['"$(st conmon 'Not affected')"']}' > "$T/rh/cve/CVE-2026-1011.json"
echo '{"threat_severity": "Important", "package_state": ['"$(st kernel 'Not affected')"']}' > "$T/rh/cve/CVE-2026-1012.json"
echo '{"threat_severity": "Important", "package_state": ['"$(st openvswitch3.3 'Not affected')"']}' > "$T/rh/cve/CVE-2026-1013.json"
# Fixed in an older minor's z-stream below this build and in the newest one above it: not proof
echo '{"threat_severity": "Important", "affected_release": ['"$(rel python-urllib3-0:1.26.5-6.el9_7.1 RHSA-2026:9), $(rel python-urllib3-0:1.26.5-9.el9_8 RHSA-2026:10)"']}' > "$T/rh/cve/CVE-2026-1009.json"
mkdir -p "$T/bin" "$T/root"
printf '* Wed Jun 03 2026 Packager - 1.26.5-8\n- Security fix for CVE-2026-1002 and CVE-2026-10020\n' > "$T/changelog"
printf '#!/bin/bash\n[ "$1 $2 $3 $4" = "--root %s -q --changelog" ] || exit 2\n[ "$5" = python3-urllib3 ] && cat "%s/changelog"\n' "$T/root" "$T" > "$T/bin/rpm"; chmod +x "$T/bin/rpm"
SITE=/usr/lib/python3.9/site-packages
p(){ echo '{"vulnerability": {"id": "'$1'", "namespace": "github:language:python", "severity": "High", "cvss": [], "fix": {"versions": ["9.9"], "state": "fixed"}}, "relatedVulnerabilities": [{"id": "'$2'"}], "artifact": {"name": "'$3'", "version": "'$4'", "type": "python", "locations": [{"path": "'$5'/'$3'-'$4'.dist-info/METADATA"}]}}'; }
rp(){ echo '{"vulnerability": {"id": "'$1'", "namespace": "redhat:distro:redhat:9", "severity": "High", "cvss": [], "fix": {"versions": ["'$4'"], "state": "fixed"}}, "artifact": {"name": "'$2'", "version": "'$3'", "type": "rpm", "locations": [{"path": "/var/lib/rpm/rpmdb.sqlite"}]}}'; }
pyreport(){ cat > "$T/vuln.json" <<EOF
{"descriptor": {"name": "grype"}, "matches": [
 $(p GHSA-0001 CVE-2026-1001 urllib3 1.26.5 $SITE), $(p GHSA-0002 CVE-2026-1002 urllib3 1.26.5 $SITE),
 $(p CVE-2026-1003 CVE-2026-1003 urllib3 1.26.5 $SITE), $(p GHSA-0004 CVE-2026-1004 Mako 1.1.4.dev0 $SITE),
 $(p GHSA-0005 CVE-2026-1005 setuptools 65.5.1 $SITE), $(p GHSA-0006 CVE-2026-1006 protobuf 3.14.0 /usr/lib64/python3.9/site-packages),
 $(p GHSA-0007 CVE-2026-1007 lxml 4.6.5 /usr/lib64/python3.9/site-packages), $(p GHSA-0008 CVE-2026-1008 Jinja2 3.0.1 $SITE), $(p GHSA-0009 CVE-2026-1009 urllib3 1.26.5 $SITE),
 $(p GHSA-0001 CVE-2026-1001 urllib3 1.26.5 /usr/local/lib/python3.9/site-packages),
 $(rp CVE-2026-1010 nginx-core 2:1.20.1-32.el9 2:1.26.3-9.module+el9.8.0+1), $(rp CVE-2026-1011 conmon 3:2.2.1-2.el9 ""),
 $(rp CVE-2026-1012 kernel-core 6.12.110-1.el9 ""), $(rp CVE-2026-1013 openvswitch3.3 3.3.4-1.el9s "")]}
EOF
}
pkept="' '.join(m['vulnerability']['id'] + '/' + m['artifact']['locations'][0]['path'].split('/')[2] for m in r['matches'] if m['artifact']['type'] == 'python')"
rkept="' '.join(m['vulnerability']['id'] + '/' + m['artifact']['name'] for m in r['matches'] if m['artifact']['type'] == 'rpm')"
pwhy(){ q "next(m['appliedIgnoreRules'][0]['reason'] for m in r['ignoredMatches'] if m['vulnerability']['id'] == '$1')"; }
pget(){ q "' '.join(map(str, next(($2,) for m in r['matches'] if m['vulnerability']['id'] == '$1' and '/local/' not in m['artifact']['locations'][0]['path'] for v in [m['vulnerability']])))"; }

pyreport; PATH="$T/bin:$PATH" KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout4" --sbom "$T/sbom.json" --root "$T/root"
chk "python pass exits 0 with no kernel" "$RC" "0"
chk "  no kernel fetch" "$([ -e "$T/checkout4" ] && echo yes || echo no)" "no"
chk "  kept" "$(q "$pkept")" "CVE-2026-1003/lib GHSA-0005/lib GHSA-0006/lib64 GHSA-0007/lib64 GHSA-0008/lib GHSA-0009/lib GHSA-0001/local"
chk "  a Red Hat fix this release sorts above" "$(pwhy GHSA-0001)" "Red Hat: fixed in python-urllib3-0:1.26.5-6.el9_7.1 (RHSA-2026:1), this build is 1.26.5-8.el9"
chk "  a z-stream rebuild above it, cleared by the changelog" "$(pwhy GHSA-0002)" "rpm changelog: python-urllib3-1.26.5-8.el9 backports CVE-2026-1002"
chk "  not affected, through a .dev0 dist-info" "$(pwhy GHSA-0004)" "Red Hat: python-mako is not affected in Red Hat Enterprise Linux 9"
chk "  no fix: rated by Red Hat" "$(pget CVE-2026-1003 "v['severity'], v['cvss'][0]['metrics']['baseScore']")" "High 7.5"
chk "  a SIG build keeps grype's verdict" "$(pget GHSA-0005 "v['severity'], len(v['cvss'])")" "High 0"
chk "  a fix in RHEL 9's newest z-stream above it is not cleared by an older one below" "$(pget GHSA-0009 "v['severity']")" "High"
chk "  so does one the SIG publishes as CentOS" "$(pget GHSA-0008 "v['severity'], len(v['cvss'])")" "High 0"
chk "  an EUS-only fix does not count" "$(pget GHSA-0006 "v['severity']")" "Medium"
chk "  fix deferred stays, rated" "$(pget GHSA-0007 "v['severity']")" "Medium"
chk "  rpm: kept" "$(q "$rkept")" "CVE-2026-1012/kernel-core CVE-2026-1013/openvswitch3.3"
chk "  rpm: a module stream's fix, cleared by RHEL's newest fix of its own line" "$(pwhy CVE-2026-1010)" "Red Hat: fixed in nginx-2:1.20.1-24.el9_7.2 (RHSA-2026:12), this build is 1.20.1-32.el9"
chk "  rpm: not affected" "$(pwhy CVE-2026-1011)" "Red Hat: conmon is not affected in Red Hat Enterprise Linux 9"
chk "  /usr/local untouched" "$(q "next(m['vulnerability']['severity'] for m in r['matches'] if '/local/' in m['artifact']['locations'][0]['path'])")" "High"
chk "  descriptor" "$(q "r['descriptor']['backportRecheck']['ignored'], r['descriptor']['backportRecheck']['changelogs']")" "{'Red Hat': 4, 'changelog': 1} True"
chk "  summary" "$OUT" "RHEL rpm matches (2 rpm, 7 python): 4 ignored by Red Hat, 1 by the rpm changelog, 4 kept; rated by Red Hat 4, by grype 0"

pyreport; PATH="$T/bin:$PATH" KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout4" --sbom "$T/sbom.json"
chk "without --root no changelog clears it" "$(q "'GHSA-0002' in [m['vulnerability']['id'] for m in r['matches']]")" "True"

pyreport; cp "$T/vuln.json" "$T/before.json"
REDHAT_SECDATA=http://127.0.0.1:9 KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout4" --sbom "$T/sbom.json"
chk "python pass: Red Hat unreachable fails" "$RC" "1"
chk "  report untouched" "$(cmp -s "$T/before.json" "$T/vuln.json" && echo same)" "same"

# OpenStack's python packages (--ossa): the advisories, in the shape ossa.git keeps them
mkdir -p "$T/ossa/ossa"
adv(){ printf 'date: 2099-01-01\n\nid: %s\n\ntitle: t\n\naffected-products:\n%b\nvulnerabilities:\n%b\n' "$1" "$2" "$3" > "$T/ossa/ossa/$1.yaml"; }
adv OSSA-2099-001 "  - product: keystone\n    version: '>=8.0.0 <25.0.1, >=26.0.0 <26.1.1, >=27.0.0 <27.0.1, >=28.0.0 <28.0.1'\n" "  - cve-id: CVE-2026-3001\n"
adv OSSA-2099-002 "  - product: Nova\n    version: '>=30.0.0 <30.2.1, >=31.0.0 <31.1.1'\n" "  - cve-id: 'CVE-2026-3002'\n"
adv OSSA-2099-003 "  - product: glance\n    version: All versions\n" "  - cve-id: CVE-2026-3003\n"
adv OSSA-2099-004 "  - product: oslo.messaging\n    version: '<16.1.3'\n  - product: python-keystoneclient\n    version: '<5.6.1'\n" "  - cve-id: CVE-2026-3004 (client to proxy)\n"
adv OSSA-2099-005 "  - product: nova\n    version: '<2014.2.3, >=2015.1.0 <2015.1.2'\n" "  - cve-id: CVE-2026-3010\n"
adv OSSA-2099-006 "  - product: glance\n    version: '<=2014.2.3, >=2015.1.0, <=2015.1.1'\n" "  - cve-id: CVE-2026-3011\n"
echo 'not an advisory' > "$T/ossa/README.rst"
git -C "$T/ossa" init -q -b master && git -C "$T/ossa" add -A &&
    git -C "$T/ossa" -c user.name=t -c user.email=t@t -c commit.gpgsign=false commit -q -m one
VENV=/opt/openstack-epoxy/lib/python3.12/site-packages
# A match whose fix grype states as given: <id> <CVE> <package> <version> <fix versions as JSON>
pd(){ p "$1" "$2" "$3" "$4" $VENV | sed "s/\"versions\": \[\"9.9\"\]/\"versions\": $5/"; }
osreport(){ cat > "$T/vuln.json" <<EOF
{"descriptor": {"name": "grype"}, "matches": [
 $(p GHSA-k001 CVE-2026-3001 keystone 27.1.0 $VENV), $(p GHSA-k002 CVE-2026-3001 keystone 27.0.0 $VENV),
 $(p GHSA-k003 CVE-2026-3001 keystone 29.0.0.0rc1 $VENV), $(p GHSA-n001 CVE-2026-3002 nova 31.0.2 $VENV),
 $(p GHSA-n002 CVE-2026-3002 nova 31.1.1 $VENV), $(p GHSA-g001 CVE-2026-3003 glance 30.0.0 $VENV),
 $(p GHSA-o001 CVE-2026-3004 oslo.messaging 16.1.2 $VENV), $(p GHSA-o002 CVE-2026-3004 python-keystoneclient 5.6.1 $VENV),
 $(p GHSA-x001 CVE-2026-3001 nova 31.0.0 $VENV), $(p GHSA-r001 CVE-2026-3005 keystone 27.1.0 $VENV),
 $(pd GHSA-d001 CVE-2026-3003 glance 30.2.0 '["2014.2.3"]'), $(pd GHSA-d002 CVE-2026-3009 nova 31.3.1 '["2014.1.4", "2013.2.4"]'),
 $(pd GHSA-d003 CVE-2026-3009 nova 31.3.1 '["2014.1.4", "28.0.1"]'), $(pd GHSA-d004 CVE-2026-3009 nova 2013.2.1 '["2014.1.4"]'),
 $(pd GHSA-d005 CVE-2026-3009 certifi 2024.2.2 '["2024.7.4"]'), $(pd GHSA-d006 CVE-2026-3001 keystone 27.0.0 '["2014.1.4"]'),
 $(pd GHSA-e001 CVE-2026-3010 nova 31.3.1 '["2014.2.3"]'), $(pd GHSA-e002 CVE-2026-3010 nova 2014.2.1 '["2014.2.3"]'),
 $(pd GHSA-e003 CVE-2026-3011 glance 30.2.0 '[]'), $(pd GHSA-e004 CVE-2026-3011 glance 2015.1.1 '[]'),
 $(rp CVE-2026-3001 keystone 27.1.0 "")]}
EOF
}
okept="' '.join(m['vulnerability']['id'] for m in r['matches'])"
ofix(){ q "next(m['vulnerability']['fix']['versions'] for m in r['matches'] if m['vulnerability']['id'] == '$1')"; }
owhy(){ q "next(m['appliedIgnoreRules'][0]['reason'].split(' (ossa.git')[0] for m in r['ignoredMatches'] if m['vulnerability']['id'] == '$1')"; }

osreport; KERNEL_VULNS_GIT="file://$T/nowhere" OSSA_GIT="file://$T/ossa" run "$T/vuln.json" "$T/checkout5" --ossa "$T/ossa-checkout"
chk "OSSA pass exits 0" "$RC" "0"
chk "  kept" "$(q "$okept")" "GHSA-k002 GHSA-n001 GHSA-g001 GHSA-o001 GHSA-x001 GHSA-r001 GHSA-d003 GHSA-d004 GHSA-d005 GHSA-d006 GHSA-e002 GHSA-e004 CVE-2026-3001"
chk "  outside every range: a later release of a fixed series" "$(owhy GHSA-k001)" "OSSA: OSSA-2099-001 affects keystone >=8.0.0 <25.0.1, >=26.0.0 <26.1.1, >=27.0.0 <27.0.1, >=28.0.0 <28.0.1, not 27.1.0"
chk "  a release candidate of a series it never affected" "$(owhy GHSA-k003)" "OSSA: OSSA-2099-001 affects keystone >=8.0.0 <25.0.1, >=26.0.0 <26.1.1, >=27.0.0 <27.0.1, >=28.0.0 <28.0.1, not 29.0.0.0rc1"
chk "  inside a range: its series' fix" "$(ofix GHSA-k002)" "['27.0.1']"
chk "  a capitalized product, a quoted CVE" "$(ofix GHSA-n001)" "['31.1.1']"
chk "  the series' fix release itself" "$(owhy GHSA-n002)" "OSSA: OSSA-2099-002 affects nova >=30.0.0 <30.2.1, >=31.0.0 <31.1.1, not 31.1.1"
chk "  prose versions judge nothing" "$(ofix GHSA-g001)" "['9.9']"
chk "  a dotted package name, a CVE with a note" "$(ofix GHSA-o001)" "['16.1.3']"
chk "  the advisory's second product" "$(owhy GHSA-o002)" "OSSA: OSSA-2099-004 affects python-keystoneclient <5.6.1, not 5.6.1"
chk "  a CVE named for another package: untouched" "$(ofix GHSA-x001)" "['9.9']"
chk "  a prose advisory, a fix from the YYYY.N releases" "$(owhy GHSA-d001)" "OSSA: glance is an OpenStack project, and its fix 2014.2.3 is a YYYY.N release, all of which came before its semantic versions such as 30.2.0"
chk "  no advisory names it, the project is OpenStack's" "$(owhy GHSA-d002)" "OSSA: nova is an OpenStack project, and its fix 2014.1.4, 2013.2.4 is a YYYY.N release, all of which came before its semantic versions such as 31.3.1"
chk "  a fix that is not all YYYY.N: untouched" "$(ofix GHSA-d003)" "['2014.1.4', '28.0.1']"
chk "  an installed YYYY.N release: untouched" "$(ofix GHSA-d004)" "['2014.1.4']"
chk "  a calendar-versioned package that is not OpenStack's: untouched" "$(ofix GHSA-d005)" "['2024.7.4']"
chk "  an advisory's range decides first" "$(ofix GHSA-d006)" "['27.0.1']"
chk "  ranges in YYYY.N releases do not take in a semantic version" "$(owhy GHSA-e001)" "OSSA: OSSA-2099-005 affects nova <2014.2.3, >=2015.1.0 <2015.1.2, not 31.3.1"
chk "  but do take in an older YYYY.N one" "$(ofix GHSA-e002)" "['2014.2.3']"
chk "  a range split at its comma is read whole" "$(owhy GHSA-e003)" "OSSA: OSSA-2099-006 affects glance <=2014.2.3, >=2015.1.0, <=2015.1.1, not 30.2.0"
chk "    and still takes in its own versions" "$(ofix GHSA-e004)" "['2015.1.1']"
chk "  descriptor" "$(q "r['descriptor']['ossaRecheck']['named'], r['descriptor']['ossaRecheck']['ignored'], r['descriptor']['ossaRecheck']['kept'], r['descriptor']['ossaRecheck']['advisories']['commit'] == '$(git -C "$T/ossa" rev-parse HEAD)'")" "14 {'advisory': 6, 'date-based fix': 2} 7 True"
chk "  summary" "$OUT" "OpenStack python matches: 14 named by an OSSA; 6 ignored by the advisory, 2 by a date-based fix; 7 kept"
chk "  checkout holds only the advisories" "$(cd "$T/ossa-checkout" && find . -path ./.git -prune -o -type f -print | grep -vc '^\./ossa/OSSA-.*\.yaml$')" "0"

# The next scan fetches what changed: the advisory now says keystone 27.0.0 is not affected
adv OSSA-2099-001 "  - product: keystone\n    version: '>=8.0.0 <25.0.1, >=26.0.0 <26.1.1, >=28.0.0 <28.0.1'\n" "  - cve-id: CVE-2026-3001\n"
git -C "$T/ossa" -c user.name=t -c user.email=t@t -c commit.gpgsign=false commit -q -am two
osreport; KERNEL_VULNS_GIT="file://$T/nowhere" OSSA_GIT="file://$T/ossa" run "$T/vuln.json" "$T/checkout5" --ossa "$T/ossa-checkout"
chk "  second run updates" "$(owhy GHSA-k002)" "OSSA: OSSA-2099-001 affects keystone >=8.0.0 <25.0.1, >=26.0.0 <26.1.1, >=28.0.0 <28.0.1, not 27.0.0"

osreport; cp "$T/vuln.json" "$T/before.json"
KERNEL_VULNS_GIT="file://$T/nowhere" OSSA_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout5" --ossa "$T/ossa-checkout2"
chk "ossa.git unreachable fails" "$RC" "1"
chk "  and says so" "$(grep -c "cannot fetch the OpenStack security advisories from file://$T/nowhere" <<<"$OUT")" "1"
chk "  report untouched" "$(cmp -s "$T/before.json" "$T/vuln.json" && echo same)" "same"
KERNEL_VULNS_GIT="file://$T/nowhere" OSSA_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout5"
chk "without --ossa: nothing fetched, report untouched" "$RC $(cmp -s "$T/before.json" "$T/vuln.json" && echo same)" "0 same"
echo '{"matches": []}' > "$T/vuln.json"
KERNEL_VULNS_GIT="file://$T/nowhere" OSSA_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout5" --ossa "$T/ossa-checkout3"
chk "no python match: no fetch" "$RC $([ -e "$T/ossa-checkout3" ] && echo yes || echo no)" "0 no"

# Go modules at a pseudo-version, judged by the module's own history. acme/widget's main line:
# v2.1.0 (2018), v1.6.27 (2019-03), the fix commit, v2.2.4 (2019-06), v2.6.3 (2021-12), then the
# commit a build links (2025-02). On a side branch from v2.1.0, a later fix (2026-01) tagged v2.6.5,
# and v0.305.2, a renumbered line's release, tagged the same year.
G="$T/gosrc"
gc(){ GIT_AUTHOR_DATE="$1" GIT_COMMITTER_DATE="$1" git -C "$G" -c user.name=t -c user.email=t@t \
    -c commit.gpgsign=false commit -q --allow-empty -m "$2"; }
gt(){ git -C "$G" -c tag.gpgsign=false tag "$@"; }
git init -q -b main "$G"
gc 2018-01-01T00:00:00 base; gt v2.1.0
gc 2019-03-01T00:00:00 one; gt v1.6.27; OLD=$(git -C "$G" rev-parse HEAD)
gc 2019-05-01T00:00:00 fix; FIX=$(git -C "$G" rev-parse HEAD)
gc 2019-06-01T00:00:00 two; gt v2.2.4
gc 2021-12-01T00:00:00 three; gt v2.6.3
gc 2025-02-27T17:41:06 linked; LINKED=$(git -C "$G" rev-parse HEAD)
git -C "$G" checkout -q -b side v2.1.0
gc 2026-01-29T09:22:49 sidefix; SIDE=$(git -C "$G" rev-parse HEAD); gt v2.6.5; gt v0.305.2
mkdir -p "$T/gomods/acme" && git clone -q --bare "$G" "$T/gomods/acme/widget"
pseudo(){ echo "v0.0.0-$(git -C "$G" log -1 --format=%cd --date=format-local:%Y%m%d%H%M%S "$1")-${1:0:12}"; }
PL=$(pseudo $LINKED) PO=$(pseudo $OLD)

# grype's database, as `grype db search --vuln <id> -o json` gives it: GitHub's go-module record, and
# a decoy from another provider
mkdir -p "$T/gadv"
gadv(){ cat > "$T/gadv/$1.json" <<EOF
[{"vulnerability": {"id": "$1", "provider": "chainguard"}, "packages": [{"package": {"name": "widget", "ecosystem": "apk"}, "detail": {"ranges": [{"version": {"constraint": "< 0"}, "fix": {"version": "0"}}]}}]},
 {"vulnerability": {"id": "$1", "provider": "github", "refs": [$3]}, "packages": [{"package": {"name": "github.com/acme/widget", "ecosystem": "go-module"}, "detail": {"ranges": [$2]}}]}]
EOF
}
rg(){ echo "{\"version\": {\"constraint\": \"$1\"}, \"fix\": {\"version\": ${2:-null}}}"; }
gadv GHSA-f001 "$(rg '>=2.0.0,<2.2.4' '"2.2.4"')" "{\"url\": \"https://github.com/acme/widget/commit/$FIX\"}"
gadv GHSA-f002 "$(rg "<0.0.0-20190501000000-${FIX:0:12}" "\"0.0.0-20190501000000-${FIX:0:12}\"")" ""
gadv GHSA-r001 "$(rg '<1.6.27' '"1.6.27"'), $(rg '>=2.0.0,<2.2.4' '"2.2.4"')" ""
gadv GHSA-k001 "$(rg "<0.0.0-20260129092249-${SIDE:0:12}" "\"0.0.0-20260129092249-${SIDE:0:12}\""), $(rg '>=2.0.0,<2.2.4' '"2.2.4"')" ""
gadv GHSA-k002 "$(rg '>=2.6.0,<2.6.5' '"2.6.5"')" ""
gadv GHSA-k003 "$(rg '<2.3.11' '"2.3.11"')" ""
gadv GHSA-k004 "$(rg '<=2.1.4')" ""
gadv GHSA-k005 "$(rg '<0.305.2' '"0.305.2"')" ""
mkdir -p "$T/bin"
cat > "$T/bin/grype" <<EOF
#!/bin/bash
[ "\$1 \$2 \$3 \$5 \$6" = "db search --vuln -o json" ] || exit 2
cat "$T/gadv/\$4.json" 2>/dev/null || echo '[]'
EOF
chmod +x "$T/bin/grype"
gm(){ echo '{"vulnerability": {"id": "'$1'", "namespace": "github:language:go", "severity": "High", "fix": {"versions": ["9.9"], "state": "fixed"}}, "artifact": {"name": "'$2'", "version": "'$3'", "type": "go-module"}}'; }
goreport(){ cat > "$T/vuln.json" <<EOF
{"descriptor": {"name": "grype"}, "matches": [
 $(gm GHSA-f001 github.com/acme/widget $PL), $(gm GHSA-f002 github.com/acme/widget $PL), $(gm GHSA-r001 github.com/acme/widget $PL),
 $(gm GHSA-k001 github.com/acme/widget $PL), $(gm GHSA-k002 github.com/acme/widget $PL), $(gm GHSA-k003 github.com/acme/widget $PL),
 $(gm GHSA-k004 github.com/acme/widget $PL), $(gm GHSA-k005 github.com/acme/widget $PO), $(gm GHSA-k006 github.com/acme/widget $PL),
 $(gm GHSA-r001 github.com/acme/widget v0.0.0-20250101000000-0123456789ab), $(gm GHSA-r001 github.com/acme/widget v2.6.3+incompatible),
 $(gm GHSA-r001 golang.org/x/widget $PL)]}
EOF
}
gkept="' '.join(m['vulnerability']['id'] + '@' + m['artifact']['version'][-12:] for m in r['matches'])"
gwhy(){ q "next(m['appliedIgnoreRules'][0]['reason'] for m in r['ignoredMatches'] if m['vulnerability']['id'] == '$1')"; }

goreport; KERNEL_VULNS_GIT="file://$T/nowhere" GOMODULE_GIT="file://$T/gomods" GRYPE="$T/bin/grype" run "$T/vuln.json" "$T/checkout6" --gomodules "$T/gomodules"
chk "Go pass exits 0" "$RC" "0"
chk "  kept" "$(q "$gkept")" "GHSA-k001@${LINKED:0:12} GHSA-k002@${LINKED:0:12} GHSA-k003@${LINKED:0:12} GHSA-k004@${LINKED:0:12} GHSA-k005@${OLD:0:12} GHSA-k006@${LINKED:0:12} GHSA-r001@0123456789ab GHSA-r001@incompatible GHSA-r001@${LINKED:0:12}"
chk "  a fix commit the advisory names is an ancestor" "$(gwhy GHSA-f001)" "go pseudo-version: github.com/acme/widget $PL is commit ${LINKED:0:12}, which contains the fix commit ${FIX:0:12}"
chk "  so is a fixed pseudo-version's commit" "$(gwhy GHSA-f002)" "go pseudo-version: github.com/acme/widget $PL is commit ${LINKED:0:12}, which contains the fix commit ${FIX:0:12}"
chk "  descends from a release after every fix" "$(gwhy GHSA-r001)" "go pseudo-version: github.com/acme/widget $PL is commit ${LINKED:0:12}, which descends from v2.6.3, released after every fixed release (1.6.27, 2.2.4) and outside every affected range"
chk "  descriptor" "$(q "r['descriptor']['goPseudoRecheck']['repositories'], r['descriptor']['goPseudoRecheck']['judged'], r['descriptor']['goPseudoRecheck']['ignored'], r['descriptor']['goPseudoRecheck']['kept']")" "['acme/widget'] 10 {'fix commit': 2, 'release': 1, 'project advisory': 0} 7"
chk "  summary" "$OUT" "Go pseudo-version matches: 10 judged; 2 ignored by a fix commit, 1 by a release; 7 kept. Go vulnerability database matches with no range: 0 judged; 0 ignored by the project's own advisory"
chk "  the clone holds no trees" "$(git -C "$T/gomodules/acme/widget.git" rev-parse --is-bare-repository)" "true"

# A build from a modified tree (+dirty); a pseudo-version that names a release, under the module path
# with a major-version suffix, whose fix commit is on another branch while the repository's other
# module path gives semantic ranges; the same advisory on a commit that is no release; an advisory
# naming only the other path. And the Go vulnerability database naming the old module path with no
# range, where the project's own advisory names only the new one.
PT=$(pseudo $(git -C "$G" rev-parse v2.6.3))
graw(){ echo "$2" > "$T/gadv/$1.json"; }
pk(){ echo "{\"package\": {\"name\": \"$1\", \"ecosystem\": \"go-module\"}, \"detail\": {\"ranges\": [$2]}}"; }
gadv GHSA-r002 "$(rg '<1.6.27' '"1.6.27"'), $(rg '>=2.0.0,<2.2.4' '"2.2.4"')" ""
graw GHSA-s001 "[{\"vulnerability\": {\"id\": \"GHSA-s001\", \"provider\": \"github\", \"refs\": []}, \"packages\": [$(pk github.com/acme/widget "$(rg '<1.6.27' '"1.6.27"'), $(rg '>=2.0.0,<2.2.4' '"2.2.4"')"), $(pk github.com/acme/widget/v7 "$(rg "<0.0.0-20260129092249-${SIDE:0:12}" "\"0.0.0-20260129092249-${SIDE:0:12}\"")")]}]"
graw GHSA-s002 "[{\"vulnerability\": {\"id\": \"GHSA-s002\", \"provider\": \"github\", \"refs\": []}, \"packages\": [$(pk github.com/acme/widget "$(rg '<2.2.4' '"2.2.4"')")]}]"
own='{"url": "https://github.com/acme/widget/security/advisories/GHSA-p00X"}'
govdb(){ graw $1 "[{\"vulnerability\": {\"id\": \"$1\", \"provider\": \"govulndb\"}, \"packages\": [$(pk github.com/acme/widget "$2")]}]"; }
ghsa(){ graw $1 "[{\"vulnerability\": {\"id\": \"$1\", \"provider\": \"github\", \"refs\": [$3]}, \"packages\": [$2]}]"; }
govdb GO-2099-0001 ""; ghsa GHSA-p001 "$(pk github.com/acme/widget/v2 "$(rg '>=2.1.0,<2.1.9' '"2.1.9"')")" "${own/p00X/p001}"
govdb GO-2099-0002 ""; ghsa GHSA-p002 "$(pk github.com/acme/widget/v2 "$(rg '>=2.1.0,<2.1.9' '"2.1.9"')")" ""
govdb GO-2099-0003 "$(rg '<1.7.40' '"1.7.40"')"; ghsa GHSA-p003 "$(pk github.com/acme/widget/v2 "$(rg '>=2.1.0,<2.1.9' '"2.1.9"')")" "${own/p00X/p003}"
govdb GO-2099-0004 ""; ghsa GHSA-p004 "$(pk github.com/acme/widget "$(rg '<1.7.40' '"1.7.40"')"), $(pk github.com/acme/widget/v2 "$(rg '>=2.1.0,<2.1.9' '"2.1.9"')")" "${own/p00X/p004}"
govdb GO-2099-0005 ""; ghsa GHSA-p005 "$(pk github.com/acme/widget/v2 "$(rg '>=2.1.0,<2.1.9' '"2.1.9"')")" "${own/p00X/p005}"
gv(){ echo '{"vulnerability": {"id": "'$1'", "namespace": "govulndb:language:go", "severity": "Critical", "fix": {"versions": [], "state": "not-fixed"}}, "relatedVulnerabilities": [{"id": "CVE-2099-'${1: -4}'"}, {"id": "'$2'"}], "matchDetails": [{"type": "exact-direct-match", "found": {"versionConstraint": "'$3'"}}], "artifact": {"name": "github.com/acme/widget", "version": "v1.7.36", "type": "go-module"}}'; }
goreport2(){ cat > "$T/vuln.json" <<EOF
{"descriptor": {"name": "grype"}, "matches": [
 $(gm GHSA-r002 github.com/acme/widget ${PL}+dirty), $(gm GHSA-s001 github.com/acme/widget/v7 $PT), $(gm GHSA-s001 github.com/acme/widget/v7 $PL),
 $(gm GHSA-s002 github.com/acme/widget/v7 $PT),
 $(gv GO-2099-0001 GHSA-p001 'none (unknown)'), $(gv GO-2099-0002 GHSA-p002 'none (unknown)'), $(gv GO-2099-0003 GHSA-p003 'none (unknown)'),
 $(gv GO-2099-0004 GHSA-p004 'none (unknown)'), $(gv GO-2099-0005 GHSA-p005 '<1.7.40 (go)')]}
EOF
}
goreport2; KERNEL_VULNS_GIT="file://$T/nowhere" GOMODULE_GIT="file://$T/gomods" GRYPE="$T/bin/grype" run "$T/vuln.json" "$T/checkout6" --gomodules "$T/gomodules"
g2kept="' '.join(m['vulnerability']['id'] + '@' + m['artifact']['name'].split('/')[-1] for m in r['matches'])"
gwhy2(){ q "next(m['appliedIgnoreRules'][0]['reason'] for m in r['ignoredMatches'] if m['vulnerability']['id'] == '$1' and m['artifact']['version'] == '$2')"; }
chk "Go pass, second report, exits 0" "$RC" "0"
chk "  kept" "$(q "$g2kept")" "GHSA-s001@v7 GHSA-s002@v7 GO-2099-0002@widget GO-2099-0003@widget GO-2099-0004@widget GO-2099-0005@widget"
chk "  a +dirty build is judged by its commit" "$(gwhy2 GHSA-r002 ${PL}+dirty)" "go pseudo-version: github.com/acme/widget ${PL}+dirty is commit ${LINKED:0:12}, which descends from v2.6.3, released after every fixed release (1.6.27, 2.2.4) and outside every affected range"
chk "  a release, by the other module path's ranges" "$(gwhy2 GHSA-s001 $PT)" "go pseudo-version: github.com/acme/widget/v7 $PT is commit $(git -C "$G" rev-parse v2.6.3 | cut -c1-12), which is release v2.6.3, released after every fixed release (1.6.27, 2.2.4) and outside every affected range"
chk "  the project's own advisory names only the new module path" "$(gwhy2 GO-2099-0001 v1.7.36)" "Go vulnerability database: GO-2099-0001 names github.com/acme/widget with no affected range; the project's own advisory GHSA-p001 names only github.com/acme/widget/v2"
chk "  summary" "$OUT" "Go pseudo-version matches: 4 judged; 0 ignored by a fix commit, 2 by a release; 2 kept. Go vulnerability database matches with no range: 4 judged; 1 ignored by the project's own advisory"

# The next scan fetches what changed: a new release
git -C "$G" checkout -q main && gc 2026-03-01T00:00:00 four && gt v2.7.0 && git -C "$T/gomods/acme/widget" fetch -q "$G" '+refs/heads/*:refs/heads/*' '+refs/tags/*:refs/tags/*'
goreport; KERNEL_VULNS_GIT="file://$T/nowhere" GOMODULE_GIT="file://$T/gomods" GRYPE="$T/bin/grype" run "$T/vuln.json" "$T/checkout6" --gomodules "$T/gomodules"
chk "  second run fetches the new tag" "$(git -C "$T/gomodules/acme/widget.git" tag --list v2.7.0)" "v2.7.0"

goreport; cp "$T/vuln.json" "$T/before.json"
KERNEL_VULNS_GIT="file://$T/nowhere" GOMODULE_GIT="file://$T/nowhere" GRYPE="$T/bin/grype" run "$T/vuln.json" "$T/checkout6" --gomodules "$T/gomodules2"
chk "a repository unreachable fails" "$RC" "1"
chk "  and says so" "$(grep -c "cannot fetch acme/widget from file://$T/nowhere" <<<"$OUT")" "1"
chk "  report untouched" "$(cmp -s "$T/before.json" "$T/vuln.json" && echo same)" "same"
KERNEL_VULNS_GIT="file://$T/nowhere" GOMODULE_GIT="file://$T/gomods" GRYPE=false run "$T/vuln.json" "$T/checkout6" --gomodules "$T/gomodules"
chk "grype's database unreadable fails" "$RC $(cmp -s "$T/before.json" "$T/vuln.json" && echo same)" "1 same"
echo '{"matches": []}' > "$T/vuln.json"
KERNEL_VULNS_GIT="file://$T/nowhere" GOMODULE_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout6" --gomodules "$T/gomodules3"
chk "no pseudo-version: no fetch" "$RC $([ -e "$T/gomodules3" ] && echo yes || echo no)" "0 no"
sk(){ python3 -B -c "import importlib.util as u; s = u.spec_from_file_location('r', '$SCRIPT'); m = u.module_from_spec(s); s.loader.exec_module(m); print(m.semkey('$1') < m.semkey('$2'))"; }
chk "semkey pseudo-version before its release" "$(sk 0.0.0-20250227174106-7829dbe62d7f 0.0.1) $(sk 1.5.1-0.20260427112133-525d1bab07e0 1.5.1)" "True True"
chk "semkey rc before release, numeric" "$(sk 1.0.0-rc.0 1.0.0) $(sk 2.9.9 2.10.0)" "True True"

pv(){ python3 -B -c "import importlib.util as u; s = u.spec_from_file_location('r', '$SCRIPT'); m = u.module_from_spec(s); s.loader.exec_module(m); print(m.pyver('$1') < m.pyver('$2'))"; }
chk "pyver rc before its release" "$(pv 29.0.0.0rc1 29.0.0)" "True"
chk "pyver dev before a pre-release" "$(pv 1.0.dev1 1.0a1)" "True"
chk "pyver release before post" "$(pv 1.0 1.0.post1)" "True"
chk "pyver numeric, not text" "$(pv 27.9.0 27.10.0)" "True"
chk "pyver trailing zeros equal" "$(pv 27.1 27.1.0) $(pv 27.1.0 27.1)" "False False"
or(){ python3 -B -c "import importlib.util as u; s = u.spec_from_file_location('r', '$SCRIPT'); m = u.module_from_spec(s); s.loader.exec_module(m); print(m.ossa_ranges('$1'))"; }
chk "ossa_ranges merges a split range" "$(or '<24.1.3, >=25.0.0 <26.1.1, >=27.0.0, <29.0.1')" "[[('<', '24.1.3')], [('>=', '25.0.0'), ('<', '26.1.1')], [('>=', '27.0.0'), ('<', '29.0.1')]]"
chk "ossa_ranges keeps a trailing lower bound open" "$(or '<24.1.3, >=27.0.0')" "[[('<', '24.1.3')], [('>=', '27.0.0')]]"
chk "ossa_ranges: prose is None" "$(or 'Folsom, Grizzly') $(or 'All versions')" "None None"
ov(){ python3 -B -c "import importlib.util as u; s = u.spec_from_file_location('r', '$SCRIPT'); m = u.module_from_spec(s); s.loader.exec_module(m); print(m.osver('$1') < m.osver('$2'))"; }
chk "osver YYYY.N before semantic" "$(ov 2015.1.2 12.0.0) $(ov 31.3.1 2014.2.3)" "True False"
chk "osver within an era, as pyver" "$(ov 2014.1.4 2014.2.3) $(ov 27.0.1 27.1.0)" "True True"

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

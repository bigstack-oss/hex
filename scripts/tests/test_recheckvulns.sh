#!/bin/bash
#
# Unit test for scripts/recheckvulns.py: which kernel matches the kernel CNA's records move to
# ignoredMatches, the fix of the ones that stay, and that nothing else is touched.
# Self-contained: vulns.git is a local repo.
#
# Needs bash, git and python3.
# Run: bash test_recheckvulns.sh
#
set -u
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT="$DIR/../recheckvulns.py"

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT

# The CNA's records, in the shapes vulns.git uses
mkdir -p "$T/vulns/cve/published/2026"
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
cna CVE-2026-0004 '"affected": ['"$(main 6.6 "" 6.18)"']'
# Fixed in mainline before 6.12 branched
cna CVE-2026-0005 '"affected": ['"$(main 5.15 "$(fix 6.6.50)" 6.11)"']'
# Only backports carry it: 6.12 from the start of history to 6.12.100, 6.18 to 6.18.54
cna CVE-2026-0006 '"affected": [{"defaultStatus": "unaffected", "versions": ['"$(win 0 6.12.100), $(win 0 6.18.54)"']}]'
# A backport brought it into 6.12.105, fixed in 6.12.115
cna CVE-2026-0008 '"affected": [{"defaultStatus": "unaffected", "versions": ['"$(win 6.12.105 6.12.115)"']}]'
git -C "$T/vulns" init -q -b master && git -C "$T/vulns" add -A &&
    git -C "$T/vulns" -c user.name=t -c user.email=t@t -c commit.gpgsign=false commit -q -m one
export KERNEL_VULNS_GIT="file://$T/vulns"

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
chk "RHEL's 5.14 rpm untouched" "$(get CVE-2026-0001 kernel-headers "v['severity']")" "Medium"
chk "not a kernel: untouched" "$(get CVE-2026-0003 openssl "v['severity'], v['fix']['versions']")" "Critical ['6.18.5']"
chk "commit recorded" "$(q "r['descriptor']['kernelRecheck']['cna']['commit']")" "$(git -C "$T/vulns" rev-parse HEAD)"
chk "summary" "$OUT" "kernel matches: 5 ignored by the CNA, 1 by NVD, 6 kept"

# The next scan fetches what changed: CVE-2026-0003 is now fixed in 6.12.105
cna CVE-2026-0003 '"affected": ['"$(main 5.10 "$(fix 6.12.105)" 6.17)"']'
git -C "$T/vulns" -c user.name=t -c user.email=t@t -c commit.gpgsign=false commit -q -am two
report; run "$T/vuln.json" "$T/checkout"; chk "second run updates and exits 0" "$RC" "0"
chk "  by the new record" "$(why CVE-2026-0003 linux-kernel)" "kernel CNA: fixed in 6.12.105"
chk "  checkout keeps only the records" "$(cd "$T/checkout" && git ls-files | grep -vc '^cve/published/.*\.json$')" "0"

report; cp "$T/vuln.json" "$T/before.json"
KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout2"; chk "vulns.git unreachable fails" "$RC" "1"
chk "  and says so" "$(grep -c "cannot fetch the kernel CNA records from file://$T/nowhere" <<<"$OUT")" "1"
chk "  report untouched" "$(cmp -s "$T/before.json" "$T/vuln.json" && echo same)" "same"

# No kernel image, nothing to judge: no fetch, and the report is left byte for byte
echo '{"matches":[]}' > "$T/vuln.json"
KERNEL_VULNS_GIT="file://$T/nowhere" run "$T/vuln.json" "$T/checkout3"; chk "no kernel image exits 0" "$RC" "0"
chk "  report untouched" "$(cat "$T/vuln.json")" '{"matches":[]}'
chk "  nothing fetched" "$([ -e "$T/checkout3" ] && echo yes || echo no)" "no"

run "$T/vuln.json"; chk "usage error with one arg" "$RC" "2"

echo "test_recheckvulns: $pass passed, $fail failed"
[ "$fail" -eq 0 ]

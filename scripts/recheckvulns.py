#!/usr/bin/env python3
# HEX SDK
#
# /// script
# requires-python = ">=3.9"
# dependencies = []
# ///
"""Re-judge grype's Linux kernel matches by the kernel CNA's records, and rate the ones that stay.

grype judges a kernel by data that does not describe it. The vmlinuz is matched by NVD's CPE
ranges, which often stop at the mainline fix and leave out the stable backports (CVE-2026-43197:
NVD says 6.6 to 6.18.16, the CNA says fixed in 6.12.103), so a fixed 6.12.y is reported
vulnerable. The kernel rpms are matched by Red Hat's data, which describes RHEL's 5.14 kernel and
names no fix for any other, so every one of its kernel CVEs is reported against a 6.12 rpm too.

The kernel CNA (cve@kernel.org) states for each CVE which stable releases carry the fix, per
branch, and which backports brought it in: the record Yocto's improve_kernel_cve_report.py judges
by too. A match the CNA says this version is fixed in, or never had, moves to "ignoredMatches"
with the reason; a match that stays gets the fix the CNA names for its own branch, or none. A
kernel rpm match for a CVE the CNA has no record of stays only if NVD matches the kernel image
too: Red Hat's verdict is about its 5.14 kernel, not this one.

Severity of what stays: Red Hat's rating where Red Hat has one, else the CNA's, else grype's own
(NVD). The CNA scores some of its records and nearly always High or Critical: CVE-2025-21673 is
9.8 to it, 5.5 Moderate to Red Hat. The rating source's CVSS goes first in the match's "cvss", so
the score shown agrees with the severity.

Both sources are read at scan time, as grype fetches its own database. The CNA's records are kept
in a git checkout that later scans bring up to date, and the report's descriptor records the
commit used; Red Hat's ratings are asked for per CVE on every scan. KERNEL_VULNS_GIT and
REDHAT_SECDATA in the environment point either at a mirror.
"""

import argparse
import concurrent.futures
import json
import os
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request

KERNEL_VULNS_GIT = os.environ.get(
    'KERNEL_VULNS_GIT', 'https://git.kernel.org/pub/scm/linux/security/vulns.git'
)
REDHAT_SECDATA = os.environ.get(
    'REDHAT_SECDATA', 'https://access.redhat.com/hydra/rest/securitydata'
)
RH_SEVERITY = {'Critical': 'Critical', 'Important': 'High', 'Moderate': 'Medium', 'Low': 'Low'}


def key(v):
    """Kernel version order: 0 < 6.12-rc3 < 6.12 < 6.12.1 < 6.13-rc1."""
    m = re.match(r'(\d+)\.(\d+)(?:\.(\d+))?(?:-rc(\d+))?', v)
    if not m:
        return 0, 0, 0, 0
    a, b, c, rc = m.groups()
    return int(a), int(b), int(c) if c else (-1 if rc else 0), int(rc or 0)


def judge(cna, ver):
    """('unaffected', why) or ('affected', fix on this version's branch or None)."""
    v = key(ver)
    # Ranges a stable branch is affected in, version to lessThan: a backport that brought the bug
    # in (6.12.63 to 6.12.70, or 6.1.141 to 6.2 if never fixed there), or "0" to the branch's fix.
    # Each speaks for one branch only: 0 to 6.18.54 says nothing about 6.12.112.
    for a in cna.get('affected', []):
        if a.get('defaultStatus') != 'unaffected':
            continue
        for x in a.get('versions', []):
            if x.get('versionType') == 'semver' and x.get('status') == 'affected':
                lo, hi = key(x['version']), key(x['lessThan'])
                if (hi if x['version'] == '0' else lo)[:2] == v[:2] and lo <= v < hi:
                    return 'affected', x['lessThan'] if hi[:2] == v[:2] else None
    main = next(
        (a['versions'] for a in cna.get('affected', []) if a.get('defaultStatus') == 'affected'),
        None,
    )
    if main is None:
        return 'unaffected', 'affected only in the stable releases it lists, not this one'
    # Unaffected ranges: before the bug went in ("0" to it), and any span it was out of mainline.
    for x in main:
        if x['status'] != 'unaffected' or 'lessThan' not in x:
            continue
        if key(x['version']) <= v < key(x['lessThan']):
            if x['version'] == '0':
                return 'unaffected', f'introduced in {x["lessThan"]}'
            return 'unaffected', f'absent from {x["version"]} to {x["lessThan"]}'
    line = f'{v[0]}.{v[1]}.*'
    branch = next((x['version'] for x in main if x.get('lessThanOrEqual') == line), None)
    if branch:
        return ('unaffected', f'fixed in {branch}') if key(branch) <= v else ('affected', branch)
    fix = next(
        (x['version'] for x in main if x.get('versionType') == 'original_commit_for_fix'), None
    )
    if fix and key(fix) <= v:
        return 'unaffected', f'fixed in {fix}'
    return 'affected', None


def fetch_records(vulns):
    """Clone or update the checkout of the CNA's records; return its commit and date."""
    # git.kernel.org serves no partial clones, so a first clone takes the whole tree (~1 minute);
    # the checkout keeps only the JSON records, and later scans fetch just what changed.
    git = ['git', '-C', vulns]
    if os.path.isdir(os.path.join(vulns, '.git')):
        subprocess.run(
            git + ['fetch', '-q', '--depth', '1', KERNEL_VULNS_GIT, 'master'], check=True
        )
        subprocess.run(git + ['reset', '-q', '--hard', 'FETCH_HEAD'], check=True)
    else:
        subprocess.run(
            ['git', 'clone', '-q', '--depth', '1', '--no-checkout', KERNEL_VULNS_GIT, vulns],
            check=True,
        )
        subprocess.run(
            git + ['sparse-checkout', 'set', '--no-cone', '/cve/published/*/*.json'], check=True
        )
        subprocess.run(git + ['checkout', '-q'], check=True)
    out = subprocess.run(
        git + ['log', '-1', '--format=%H %cI'],
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    )
    return out.stdout.split()


def record(vulns, cve):
    try:
        with open(os.path.join(vulns, 'cve', 'published', cve.split('-')[1], f'{cve}.json')) as f:
            return json.load(f)['containers']['cna']
    except FileNotFoundError:
        return None


def redhat(cve):
    """Red Hat's record of a CVE, or None if it has none."""
    # The default Python-urllib agent is refused (403); any named one is served.
    req = urllib.request.Request(
        f'{REDHAT_SECDATA}/cve/{cve}.json', headers={'User-Agent': 'hex-recheckvulns'}
    )
    for attempt in range(3):
        try:
            with urllib.request.urlopen(req, timeout=60) as r:
                return json.load(r)
        except urllib.error.HTTPError as e:
            if e.code == 404:
                return None
            err = e
        except urllib.error.URLError as e:
            if isinstance(e.reason, FileNotFoundError):
                return None
            err = e
        if attempt < 2:
            time.sleep(5 * (attempt + 1))
    raise RuntimeError(f'{cve}: {err}')


def rate(vul, cna, rh):
    """Rate a match Red Hat > kernel CNA > NVD (grype's own); return who rated it."""
    cvss = next((x['cvssV3_1'] for x in cna.get('metrics', []) if 'cvssV3_1' in x), None)
    if rh.get('threat_severity') in RH_SEVERITY:
        vul['severity'], by = RH_SEVERITY[rh['threat_severity']], 'Red Hat'
        score = rh.get('cvss3') and {
            'source': 'secalert@redhat.com',
            'vector': rh['cvss3']['cvss3_scoring_vector'],
            'score': float(rh['cvss3']['cvss3_base_score']),
        }
    elif cvss:
        vul['severity'], by = cvss['baseSeverity'].capitalize(), 'kernel CNA'
        score = {
            'source': cna['providerMetadata']['orgId'],
            'vector': cvss['vectorString'],
            'score': cvss['baseScore'],
        }
    else:
        return 'NVD'
    if score:
        vul['cvss'] = [
            {
                'source': score['source'],
                'type': 'Primary',
                'version': score['vector'][5:8],
                'vector': score['vector'],
                'metrics': {'baseScore': score['score']},
                'vendorMetadata': {},
            }
        ] + (vul.get('cvss') or [])
    return by


def upstream(v):
    return re.match(r'(?:\d+:)?([^-]+)', v).group(1)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('report', help="grype's JSON report, rewritten in place")
    ap.add_argument(
        'vulns',
        help="checkout of the kernel CNA's records (vulns.git): cloned when missing, "
        'brought up to date otherwise',
    )
    args = ap.parse_args()

    with open(args.report) as f:
        rep = json.load(f)

    # The kernel rpms judged are the ones that package a kernel image the SBOM found; any other
    # kernel-* rpm (RHEL's own, at 5.14) keeps grype's Red Hat verdict.
    images = {
        upstream(m['artifact']['version'])
        for m in rep['matches']
        if m['artifact']['type'] == 'linux-kernel'
    }
    if not images:
        print('no kernel image in the report: nothing to re-check')
        return
    nvd = {
        (upstream(m['artifact']['version']), m['vulnerability']['id'])
        for m in rep['matches']
        if m['artifact']['type'] == 'linux-kernel'
    }

    def is_kernel(a):
        return a['type'] == 'linux-kernel' or (
            a['type'] == 'rpm'
            and re.match(r'kernel(-|$)', a['name'])
            and upstream(a['version']) in images
        )

    try:
        commit, date = fetch_records(args.vulns)
    except subprocess.CalledProcessError:
        sys.exit(f'recheckvulns: cannot fetch the kernel CNA records from {KERNEL_VULNS_GIT}')

    keep, kernel, ignored = [], [], {'kernel CNA': [], 'NVD': []}
    for m in rep['matches']:
        vul, art = m['vulnerability'], m['artifact']
        if not is_kernel(art):
            keep.append(m)
            continue
        cna = record(args.vulns, vul['id'])
        if cna:
            state, what = judge(cna, art['version'])
            by, why = 'kernel CNA', f'kernel CNA: {what} (vulns.git {commit[:12]})'
        elif art['type'] == 'rpm' and (upstream(art['version']), vul['id']) not in nvd:
            state = 'unaffected'
            by = 'NVD'
            why = (
                f'NVD: no match on the kernel image, and no kernel CNA record (vulns.git '
                f"{commit[:12]}); Red Hat's verdict is for RHEL's 5.14 kernel"
            )
        else:
            state = 'affected'
        if state == 'unaffected':
            m['appliedIgnoreRules'] = [
                {
                    'vulnerability': vul['id'],
                    'namespace': vul['namespace'],
                    'reason': why,
                    'package': {
                        'name': art['name'],
                        'version': art['version'],
                        'type': art['type'],
                    },
                }
            ]
            ignored[by].append(m)
            continue
        if cna:
            vul['fix'] = (
                {'versions': [what], 'state': 'fixed'}
                if what
                else {'versions': [], 'state': 'not-fixed'}
            )
        keep.append(m)
        kernel.append(m)

    cves = sorted({m['vulnerability']['id'] for m in kernel})
    try:
        with concurrent.futures.ThreadPoolExecutor(8) as pool:
            rh = dict(zip(cves, pool.map(redhat, cves)))
    except RuntimeError as e:
        sys.exit(f'recheckvulns: Red Hat security data unreachable: {e}')

    rated = dict.fromkeys(['Red Hat', 'kernel CNA', 'NVD'], 0)
    for m in kernel:
        cve = m['vulnerability']['id']
        rated[rate(m['vulnerability'], record(args.vulns, cve) or {}, rh[cve] or {})] += 1

    rep['matches'] = keep
    rep['ignoredMatches'] = (
        (rep.get('ignoredMatches') or []) + ignored['kernel CNA'] + ignored['NVD']
    )
    rep['descriptor']['kernelRecheck'] = {
        'cna': {'repository': KERNEL_VULNS_GIT, 'commit': commit, 'date': date},
        'severity': {'order': ['Red Hat', 'kernel CNA', 'NVD'], 'redhat': REDHAT_SECDATA},
        'ignored': {by: len(ms) for by, ms in ignored.items()},
        'kept': len(kernel),
        'rated': rated,
    }
    with open(args.report + '.tmp', 'w') as f:
        json.dump(rep, f, indent=1)
    os.replace(args.report + '.tmp', args.report)
    print(
        f'kernel matches: {len(ignored["kernel CNA"])} ignored by the CNA, '
        f'{len(ignored["NVD"])} by NVD, {len(kernel)} kept; rated by Red Hat {rated["Red Hat"]}, '
        f'by the CNA {rated["kernel CNA"]}, by NVD {rated["NVD"]}'
    )


if __name__ == '__main__':
    main()

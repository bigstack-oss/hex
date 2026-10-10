#!/usr/bin/env python3
# HEX SDK
#
# /// script
# requires-python = ">=3.9"
# dependencies = []
# ///
"""Re-judge grype's Linux kernel matches by the kernel CNA's records.

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

The CNA's records are read at scan time, as grype fetches its own database: they are kept in a
git checkout that later scans bring up to date, and the report's descriptor records the commit
used. KERNEL_VULNS_GIT in the environment points at a mirror.
"""

import argparse
import json
import os
import re
import subprocess
import sys

KERNEL_VULNS_GIT = os.environ.get(
    'KERNEL_VULNS_GIT', 'https://git.kernel.org/pub/scm/linux/security/vulns.git'
)


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

    rep['matches'] = keep
    rep['ignoredMatches'] = (
        (rep.get('ignoredMatches') or []) + ignored['kernel CNA'] + ignored['NVD']
    )
    rep['descriptor']['kernelRecheck'] = {
        'cna': {'repository': KERNEL_VULNS_GIT, 'commit': commit, 'date': date},
        'ignored': {by: len(ms) for by, ms in ignored.items()},
        'kept': len(kernel),
    }
    with open(args.report + '.tmp', 'w') as f:
        json.dump(rep, f, indent=1)
    os.replace(args.report + '.tmp', args.report)
    print(
        f'kernel matches: {len(ignored["kernel CNA"])} ignored by the CNA, '
        f'{len(ignored["NVD"])} by NVD, {len(kernel)} kept'
    )


if __name__ == '__main__':
    main()

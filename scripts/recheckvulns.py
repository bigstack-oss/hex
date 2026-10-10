#!/usr/bin/env python3
# HEX SDK
#
# /// script
# requires-python = ">=3.9"
# dependencies = []
# ///
"""Re-judge grype's kernel matches by the kernel CNA, and RHEL python rpm matches by Red Hat.

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

Given the SBOM (--sbom), Python packages that an rpm built from RHEL's sources installed into the
system site-packages are re-judged too. grype matches them by the package's own version against
the GitHub advisories, and Red Hat backports fixes without moving that version: urllib3 1.26.5
from python3-urllib3-1.26.5-8.el9 carries fixes its version predates and is reported for each.
Red Hat's RHEL 9 record decides instead. A match moves to "ignoredMatches" when Red Hat says the
source package is not affected, when it names a fix of the same version whose release this build
sorts at or above, or when the rpm's own changelog names the CVE (--root, the scanned rootfs, lets
it read them): a z-stream rebuild such as 8.el9_8 sorts above the 8.el9 that fixed it in Stream.
The rpm is the SBOM's python3-<name> of the same version, published by CentOS or Red Hat with a
Stream or RHEL dist tag, so a SIG (el9s) or EPEL build of that name, which carries none of RHEL's
backports, keeps grype's verdict. A match that stays is rated by Red Hat where Red Hat rates it.
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
RHEL = 'Red Hat Enterprise Linux 9'
DISTRO_PUBLISHERS = {'CentOS', 'Red Hat, Inc.'}
SYSTEM_SITE = re.compile(r'/usr/lib(64)?/python3[.0-9]*/site-packages/')


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


def vercmp(a, b):
    """rpm's ordering of two version or release strings: -1, 0 or 1."""
    x, y = (re.findall(r'~|\^|\d+|[a-zA-Z]+', v) for v in (a, b))
    while x or y:
        p, q = x[0] if x else None, y[0] if y else None
        if p == '~' or q == '~':
            if p != q:
                return -1 if p == '~' else 1
        elif p == '^' or q == '^':
            if p is None or q is None:
                return -1 if p is None else 1
            if p != q:
                return 1 if q == '^' else -1
        elif p is None or q is None:
            return 1 if q is None else -1
        elif p.isdigit() != q.isdigit():
            return 1 if p.isdigit() else -1
        elif p.isdigit() and int(p) != int(q):
            return 1 if int(p) > int(q) else -1
        elif not p.isdigit() and p != q:
            return 1 if p > q else -1
        x, y = x[1:], y[1:]
    return 0


def fetch_redhat(cves):
    """Red Hat's records of these CVEs, or exit if Red Hat cannot be reached."""
    try:
        with concurrent.futures.ThreadPoolExecutor(8) as pool:
            return dict(zip(cves, pool.map(redhat, cves)))
    except RuntimeError as e:
        sys.exit(f'recheckvulns: Red Hat security data unreachable: {e}')


def ignore(m, why):
    vul, art = m['vulnerability'], m['artifact']
    m['appliedIgnoreRules'] = [
        {
            'vulnerability': vul['id'],
            'namespace': vul['namespace'],
            'reason': why,
            'package': {'name': art['name'], 'version': art['version'], 'type': art['type']},
        }
    ]


def recheck_kernel(rep, vulns):
    """Re-judge the kernel matches in place; a summary, or None if the report has no kernel."""
    # The kernel rpms judged are the ones that package a kernel image the SBOM found; any other
    # kernel-* rpm (RHEL's own, at 5.14) keeps grype's Red Hat verdict.
    images = {
        upstream(m['artifact']['version'])
        for m in rep['matches']
        if m['artifact']['type'] == 'linux-kernel'
    }
    if not images:
        return None
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
        commit, date = fetch_records(vulns)
    except subprocess.CalledProcessError:
        sys.exit(f'recheckvulns: cannot fetch the kernel CNA records from {KERNEL_VULNS_GIT}')

    keep, kernel, ignored = [], [], {'kernel CNA': [], 'NVD': []}
    for m in rep['matches']:
        vul, art = m['vulnerability'], m['artifact']
        if not is_kernel(art):
            keep.append(m)
            continue
        cna = record(vulns, vul['id'])
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
            ignore(m, why)
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

    rh = fetch_redhat(sorted({m['vulnerability']['id'] for m in kernel}))
    rated = dict.fromkeys(['Red Hat', 'kernel CNA', 'NVD'], 0)
    for m in kernel:
        cve = m['vulnerability']['id']
        rated[rate(m['vulnerability'], record(vulns, cve) or {}, rh[cve] or {})] += 1

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
    return (
        f'kernel matches: {len(ignored["kernel CNA"])} ignored by the CNA, '
        f'{len(ignored["NVD"])} by NVD, {len(kernel)} kept; rated by Red Hat {rated["Red Hat"]}, '
        f'by the CNA {rated["kernel CNA"]}, by NVD {rated["NVD"]}'
    )


def sbom_rpms(path):
    """The SBOM's rpms by name: (version, release, source package, publisher)."""
    with open(path) as f:
        sbom = json.load(f)
    rpms = {}
    for c in sbom.get('components') or []:
        if not (c.get('purl') or '').startswith('pkg:rpm/'):
            continue
        props = {p['name']: p['value'] for p in c.get('properties') or []}
        src = re.match(r'(.+)-[^-]+-[^-]+\.src\.rpm$', props.get('syft:metadata:sourceRpm', ''))
        evr = re.match(r'(?:\d+:)?(.+)-([^-]+)$', c.get('version', ''))
        if src and evr:
            rpms[c['name']] = (evr.group(1), evr.group(2), src.group(1), c.get('publisher'))
    return rpms


def distro_rpm(art, rpms):
    """(name, version, release, source package) of the RHEL-built rpm that installed this python
    package into the system site-packages, or None."""
    if art['type'] != 'python' or not any(
        SYSTEM_SITE.match(x.get('path', '')) for x in art.get('locations') or []
    ):
        return None
    name = 'python3-' + re.sub(r'[-_.]+', '-', art['name']).lower()
    r = rpms.get(name)
    # The publisher alone does not tell: the Cloud SIG ships python3-jinja2-3.0.1-2.el9s as
    # "CentOS". Its dist tag does -- el9s is a SIG build, el9 and el9_N are Stream's and RHEL's.
    if not r or r[3] not in DISTRO_PUBLISHERS or re.search(r'\.el\d+s(\.|$)', r[1]):
        return None
    # Red Hat's dist-info can carry a suffix its rpm version does not: Mako 1.1.4.dev0, rpm 1.1.4
    if art['version'] != r[0] and not art['version'].startswith(r[0] + '.'):
        return None
    return name, r[0], r[1], r[2]


def changelog(root, name):
    if not root:
        return ''
    out = subprocess.run(
        ['rpm', '--root', root, '-q', '--changelog', name],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    return out.stdout if out.returncode == 0 else ''


def backported(rh, cve, srpm, version, release, log):
    """(who, why) when Red Hat's record or the rpm's changelog clears this build, else None."""
    for x in rh.get('package_state') or []:
        if (x.get('product_name'), x.get('package_name'), x.get('fix_state')) == (
            RHEL,
            srpm,
            'Not affected',
        ):
            return 'Red Hat', f'Red Hat: {srpm} is not affected in {RHEL}'
    for x in rh.get('affected_release') or []:
        fix = re.match(r'(.+)-(?:\d+:)?([^-]+)-([^-]+)$', x.get('package') or '')
        if (
            x.get('product_name') == RHEL
            and fix
            and fix.group(1) == srpm
            and fix.group(2) == version
            and vercmp(release, fix.group(3)) >= 0
        ):
            return (
                'Red Hat',
                f'Red Hat: fixed in {x["package"]} ({x.get("advisory")}), '
                f'this build is {version}-{release}',
            )
    if re.search(rf'(?<![\w-]){re.escape(cve)}(?![\w-])', log):
        return 'changelog', f'rpm changelog: {srpm}-{version}-{release} backports {cve}'
    return None


def recheck_python(rep, sbom, root):
    """Re-judge the matches on RHEL-built python rpms in place; return a summary."""
    rpms = sbom_rpms(sbom)
    owned = []
    for m in rep['matches']:
        r = distro_rpm(m['artifact'], rpms)
        ids = [m['vulnerability']['id']] + [
            x.get('id', '') for x in m.get('relatedVulnerabilities') or []
        ]
        cve = next((i for i in ids if i.startswith('CVE-')), None)
        if r and cve:
            owned.append((m, r, cve))

    rh = fetch_redhat(sorted({cve for _, _, cve in owned}))
    logs, drop = {}, set()
    ignored = {'Red Hat': [], 'changelog': []}
    rated = {'Red Hat': 0, 'grype': 0}
    for m, (name, version, release, srpm), cve in owned:
        if name not in logs:
            logs[name] = changelog(root, name)
        verdict = backported(rh[cve] or {}, cve, srpm, version, release, logs[name])
        if verdict:
            by, why = verdict
            ignore(m, why)
            ignored[by].append(m)
            drop.add(id(m))
        else:
            by = rate(m['vulnerability'], {}, rh[cve] or {})
            rated['Red Hat' if by == 'Red Hat' else 'grype'] += 1

    rep['matches'] = [m for m in rep['matches'] if id(m) not in drop]
    rep['ignoredMatches'] = (
        (rep.get('ignoredMatches') or []) + ignored['Red Hat'] + ignored['changelog']
    )
    kept = len(owned) - len(drop)
    rep.setdefault('descriptor', {})['backportRecheck'] = {
        'product': RHEL,
        'redhat': REDHAT_SECDATA,
        'changelogs': bool(root),
        'ignored': {by: len(ms) for by, ms in ignored.items()},
        'kept': kept,
        'rated': rated,
    }
    return (
        f'RHEL python rpm matches: {len(ignored["Red Hat"])} ignored by Red Hat, '
        f'{len(ignored["changelog"])} by the rpm changelog, {kept} kept; '
        f'rated by Red Hat {rated["Red Hat"]}, by grype {rated["grype"]}'
    )


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('report', help="grype's JSON report, rewritten in place")
    ap.add_argument(
        'vulns',
        help="checkout of the kernel CNA's records (vulns.git): cloned when missing, "
        'brought up to date otherwise',
    )
    ap.add_argument('--sbom', help='the CycloneDX SBOM the report was scanned from')
    ap.add_argument('--root', help="the rootfs the SBOM was made from, for its rpms' changelogs")
    args = ap.parse_args()

    with open(args.report) as f:
        rep = json.load(f)

    done = [recheck_kernel(rep, args.vulns)]
    if args.sbom:
        done.append(recheck_python(rep, args.sbom, args.root))
    done = [x for x in done if x]
    if not done:
        print('no kernel image in the report: nothing to re-check')
        return

    with open(args.report + '.tmp', 'w') as f:
        json.dump(rep, f, indent=1)
    os.replace(args.report + '.tmp', args.report)
    print('\n'.join(done))


if __name__ == '__main__':
    main()

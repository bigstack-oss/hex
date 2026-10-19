#!/usr/bin/env python3
"""SBOM and vulnerability report for the container images a rootfs ships in a registry's storage.

A rootfs may carry container images inside a registry's filesystem storage (Docker Distribution's
layout: repositories/<name>/_manifests/tags/<tag>/current/link names a manifest, and
blobs/sha256/<2>/<64>/data holds every blob). syft scanning the rootfs sees none of their packages:
each layer is a gzipped tar named `data`. This scans every image on its own: an OCI image layout per
unique manifest, with the image's blobs copied in (go-containerregistry refuses a symlinked blob, and
a hardlink would change the rootfs), syft on that layout, grype on its SBOM. Each image is scanned
alone because grype matches distro packages against the image's own distro, which the SBOM names.

The rootfs SBOM and report are inputs, never changed. The outputs are a second pair:
  - the SBOM: the rootfs SBOM with each image added as a `container` component holding the image's
    components, every bom-ref of an image prefixed so the document stays unique;
  - the report: the rootfs report's matches, followed by each image's, with every artifact location
    rewritten to <registry>/<repository>:<tag>!<path>, so a match names the image it is in. The
    descriptor gains an `images` list: each manifest digest, its tags, distro and match counts.

grype's database is updated once, then held for every image, so all of them are judged by the same
data. syft and grype come from PATH.
"""
import argparse
import json
import os
import shutil
import subprocess
import sys

INDEX_TYPES = {
    'application/vnd.docker.distribution.manifest.list.v2+json',
    'application/vnd.oci.image.index.v1+json',
}


def blob_path(v2, digest):
    algo, hexd = digest.split(':', 1)
    return os.path.join(v2, 'blobs', algo, hexd[:2], hexd, 'data')


def find_tags(v2):
    """[(repository, tag, manifest digest)] from a registry's v2 storage, sorted."""
    repos = os.path.join(v2, 'repositories')
    out = []
    for root, _dirs, files in os.walk(repos):
        if 'link' not in files or not root.endswith('/current'):
            continue
        rel = os.path.relpath(root, repos)
        if '/_manifests/tags/' not in rel:
            continue
        name, tag = rel.split('/_manifests/tags/', 1)
        with open(os.path.join(root, 'link')) as f:
            out.append((name, tag[:-len('/current')], f.read().strip()))
    return sorted(out)


def resolve(v2, digest):
    """(manifest digest, media type, manifest) for linux/amd64, following one index level."""
    with open(blob_path(v2, digest)) as f:
        m = json.load(f)
    mt = m.get('mediaType', 'application/vnd.oci.image.manifest.v1+json')
    if mt in INDEX_TYPES or 'manifests' in m:
        amd64 = [e for e in m.get('manifests', [])
                 if e.get('platform', {}).get('os') == 'linux'
                 and e.get('platform', {}).get('architecture') == 'amd64']
        if not amd64:
            raise SystemExit('scanimages: %s is an index with no linux/amd64 manifest' % digest)
        return resolve(v2, amd64[0]['digest'])
    return digest, mt, m


def make_layout(v2, layout, ref, digest):
    """An OCI image layout for one image, its blobs copied as regular files."""
    mdigest, mtype, manifest = resolve(v2, digest)
    blobs = os.path.join(layout, 'blobs', 'sha256')
    os.makedirs(blobs)
    with open(os.path.join(layout, 'oci-layout'), 'w') as f:
        json.dump({'imageLayoutVersion': '1.0.0'}, f)
    with open(os.path.join(layout, 'index.json'), 'w') as f:
        json.dump({'schemaVersion': 2, 'mediaType': 'application/vnd.oci.image.index.v1+json',
                   'manifests': [{'mediaType': mtype, 'digest': mdigest,
                                  'size': os.path.getsize(blob_path(v2, mdigest)),
                                  'annotations': {'org.opencontainers.image.ref.name': ref}}]}, f)
    for d in [mdigest, manifest['config']['digest']] + [l['digest'] for l in manifest.get('layers', [])]:
        dst = os.path.join(blobs, d.split(':', 1)[1])
        if not os.path.exists(dst):
            if not os.path.isfile(blob_path(v2, d)):
                raise SystemExit('scanimages: %s: blob %s is missing' % (ref, d))
            shutil.copyfile(blob_path(v2, d), dst)
    return mdigest


def run(cmd, env, out=None):
    p = subprocess.run(cmd, env=env, stdout=subprocess.PIPE if out is None else out,
                       stderr=subprocess.PIPE, text=True)
    if p.returncode:
        raise SystemExit('scanimages: %s failed (%d): %s' % (cmd[0], p.returncode, p.stderr.strip()[-2000:]))
    return p.stdout


def prefix_refs(bom, pfx):
    """Prefix every bom-ref, and every reference to one, in an image's components and dependencies."""
    def comp(c):
        if 'bom-ref' in c:
            c['bom-ref'] = pfx + c['bom-ref']
        for sub in c.get('components', []):
            comp(sub)
    comps = bom.get('components', [])
    for c in comps:
        comp(c)
    deps = []
    for d in bom.get('dependencies', []):
        e = {'ref': pfx + d['ref']}
        if 'dependsOn' in d:
            e['dependsOn'] = [pfx + r for r in d['dependsOn']]
        deps.append(e)
    return comps, deps


def located(match, where):
    for loc in match.get('artifact', {}).get('locations', []):
        loc['path'] = '%s!%s' % (where, loc.get('path', ''))
    return match


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--root', required=True, help='the rootfs that holds the registries')
    ap.add_argument('--registry', action='append', required=True,
                    help='a registry storage root, relative to --root (repeatable)')
    ap.add_argument('--sbom', required=True, help="the rootfs SBOM (CycloneDX JSON), read only")
    ap.add_argument('--vuln', required=True, help="the rootfs report (grype JSON), read only")
    ap.add_argument('--out-sbom', required=True)
    ap.add_argument('--out-vuln', required=True)
    ap.add_argument('--work', required=True, help='scratch dir, emptied before and removed after')
    ap.add_argument('--vex', action='append', default=[], help='OpenVEX document for grype (repeatable)')
    a = ap.parse_args()

    shutil.rmtree(a.work, ignore_errors=True)
    os.makedirs(os.path.join(a.work, 'tmp'))
    os.makedirs(os.path.join(a.work, 'cache'))
    env = dict(os.environ, TMPDIR=os.path.join(a.work, 'tmp'), SYFT_CACHE_DIR=os.path.join(a.work, 'cache'),
               SYFT_CHECK_FOR_APP_UPDATE='false', GRYPE_CHECK_FOR_APP_UPDATE='false')
    run(['grype', 'db', 'update'], env)
    env['GRYPE_DB_AUTO_UPDATE'] = 'false'

    with open(a.sbom) as f:
        sbom = json.load(f)
    with open(a.vuln) as f:
        vuln = json.load(f)

    images = []
    for reg in a.registry:
        rel = '/' + reg.strip('/')
        v2 = os.path.join(a.root, reg.strip('/'), 'docker', 'registry', 'v2')
        if not os.path.isdir(v2):
            raise SystemExit('scanimages: no registry storage at %s' % v2)
        by_digest = {}
        for name, tag, digest in find_tags(v2):
            by_digest.setdefault(digest, []).append('%s:%s' % (name, tag))
        for n, (digest, refs) in enumerate(sorted(by_digest.items(), key=lambda kv: kv[1][0])):
            ref = refs[0]
            layout = os.path.join(a.work, 'layout')
            shutil.rmtree(layout, ignore_errors=True)
            mdigest = make_layout(v2, layout, ref, digest)
            cdx = os.path.join(a.work, 'image.cdx.json')
            name, tag = ref.rsplit(':', 1)
            run(['syft', 'scan', 'oci-dir:' + layout, '-q', '--source-name', name, '--source-version', tag,
                 '-o', 'cyclonedx-json=' + cdx], env)
            rep = json.loads(run(['grype', 'sbom:' + cdx, '-o', 'json'] + sum((['--vex', v] for v in a.vex), []), env))
            shutil.rmtree(layout)
            with open(cdx) as f:
                ibom = json.load(f)
            os.remove(cdx)

            where = '%s/%s' % (rel, ref)
            matches = [located(m, where) for m in rep.get('matches', [])]
            ignored = [located(m, where) for m in rep.get('ignoredMatches', [])]
            vuln.setdefault('matches', []).extend(matches)
            vuln.setdefault('ignoredMatches', []).extend(ignored)

            pfx = 'image:%s/' % digest.split(':', 1)[1][:12]
            comps, deps = prefix_refs(ibom, pfx)
            sbom.setdefault('components', []).append({
                'type': 'container', 'bom-ref': pfx.rstrip('/'), 'name': name, 'version': tag,
                'properties': [{'name': 'hex:image:location', 'value': rel},
                               {'name': 'hex:image:manifest-digest', 'value': mdigest}]
                              + [{'name': 'hex:image:tag', 'value': r} for r in refs],
                'components': comps})
            sbom.setdefault('dependencies', []).extend(deps)

            sev = {}
            for m in matches:
                s = m['vulnerability'].get('severity', 'Unknown')
                sev[s] = sev.get(s, 0) + 1
            distro = rep.get('distro', {})
            images.append({'registry': rel, 'tags': refs, 'digest': mdigest,
                           'distro': ('%s %s' % (distro.get('name', ''), distro.get('version', ''))).strip(),
                           'matches': len(matches), 'ignoredMatches': len(ignored), 'severities': sev})
            print('  IMAGE   %s (%d matches)' % (where, len(matches)), flush=True)

    vuln.setdefault('descriptor', {})['images'] = images
    for path, doc in ((a.out_sbom, sbom), (a.out_vuln, vuln)):
        tmp = path + '.part'
        with open(tmp, 'w') as f:
            json.dump(doc, f)
        os.replace(tmp, path)
    shutil.rmtree(a.work)


if __name__ == '__main__':
    sys.exit(main())

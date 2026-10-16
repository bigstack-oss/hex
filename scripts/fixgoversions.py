#!/usr/bin/env python3
# HEX SDK
#
# /// script
# requires-python = ">=3.9"
# dependencies = []
# ///
"""Give a Go binary's main module the version of the rpm that ships it, where the SBOM can only guess.

A Go binary records its main module's version in its build info, but only a `go install` of a
tagged module fills it in. A binary built from a checkout says "(devel)", which is what grafana's,
docker-compose's, docker-buildx's and containerd's release builds carry. syft then guesses: from
-X ldflags when the build stamped a version there, else from version-like strings in the binary
(golang.main-module-version.from-contents). That guess is how grafana-enterprise 12.4.10 came to be
github.com/grafana/grafana v3.53.2 -- an unrelated "3.53.2" in its data -- so grype matched it
against advisories for Grafana 3.x through 8.x and missed every one for 12.4. Where nothing in
the binary looks like a version, the module is UNKNOWN and nothing is matched at all.

When the binary came from an rpm, the rpm database of the scanned rootfs says what it is. For
each Go main module syft found, whose binary's build info says "(devel)" and whose owning rpm is
that module's own package -- the rpm's name carries the module's last path element, major-version
suffix dropped (grafana-enterprise for github.com/grafana/grafana, docker-compose-plugin for
github.com/docker/compose/v5) -- the component's version becomes v<rpm version>, and its purl and
CPEs follow. A binary its rpm only bundles keeps syft's version: containerd.io ships runc, which
is github.com/opencontainers/runc at its own version. Binaries no rpm owns are left alone too.

The SBOM is rewritten in place, so the one grype scans is the one that ships.
"""

import argparse
import json
import mmap
import re
import subprocess

GO_CATALOGER = 'go-module-binary-cataloger'
BUILDINFO = b'\xff Go buildinf:'
# runtime/debug's modinfo sentinel: the module info starts with these 16 bytes
MODINFO_START = bytes.fromhex('3077af0c9274080241e1c107e6d618e6')


def uvarint(buf, i):
    """encoding/binary.Uvarint: (value, next offset)."""
    v, shift = 0, 0
    while True:
        b = buf[i]
        i += 1
        v |= (b & 0x7F) << shift
        if b < 0x80:
            return v, i
        shift += 7


def main_module_version(path):
    """The main module's version in a Go binary's build info ('(devel)' for a checkout build), or
    None when there is no Go 1.18+ build info to read."""
    info = None
    try:
        with open(path, 'rb') as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as m:
            # The magic can also appear as a plain string (a binary that imports debug/buildinfo),
            # so take the first occurrence that parses as the build info header.
            i = m.find(BUILDINFO)
            while i >= 0 and info is None:
                # pointer size, then flag 2: inline varint strings (Go 1.18 and later)
                if m[i + 14] in (4, 8) and m[i + 15] & 2:
                    n, j = uvarint(m, i + 32)  # the Go version
                    n, j = uvarint(m, j + n)  # the module info, inside 16-byte sentinels
                    if n > 32 and m[j : j + 16] == MODINFO_START:
                        info = m[j + 16 : j + n - 16].decode('utf-8', 'replace')
                i = m.find(BUILDINFO, i + 1)
    except (OSError, ValueError, IndexError):
        return None
    for line in (info or '').splitlines():
        f = line.split('\t')
        if f[0] == 'mod' and len(f) > 2:
            return f[2]
    return None


def owning_rpm(root, path):
    """(name, version) of the rpm that owns path in the rootfs, or None."""
    out = subprocess.run(
        ['rpm', '--root', root, '-qf', '--qf', r'%{NAME} %{VERSION}\n', path],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    lines = out.stdout.split('\n') if out.returncode == 0 else []
    return tuple(lines[0].split(' ', 1)) if lines and ' ' in lines[0] else None


def own_package(module, rpm):
    """Whether rpm is module's own package: its name carries the module's last path element."""
    base = re.sub(r'/v\d+$', '', module).rsplit('/', 1)[-1].lower()
    return bool(re.search(rf'(^|[-_.]){re.escape(base)}($|[-_.])', rpm.lower()))


def set_cpe_version(cpe, version):
    f = cpe.split(':')
    if len(f) > 5 and f[:2] == ['cpe', '2.3']:
        f[5] = version
    return ':'.join(f)


def fix(sbom, root):
    """Rewrite the guessed main-module versions in sbom (a CycloneDX dict); list what changed."""
    changed = []
    for c in sbom.get('components') or []:
        props = {p['name']: p['value'] for p in c.get('properties') or []}
        if props.get('syft:package:foundBy') != GO_CATALOGER:
            continue
        if props.get('syft:metadata:mainModule') != c.get('name'):
            continue
        path = props.get('syft:location:0:path', '')
        if not path or main_module_version(root + path) != '(devel)':
            continue
        rpm = owning_rpm(root, path)
        if not rpm or not own_package(c['name'], rpm[0]) or not re.match(r'\d+(\.\d+)*$', rpm[1]):
            continue
        version = 'v' + rpm[1]
        if c.get('version') == version:
            continue
        changed.append((path, c['name'], c.get('version'), version, rpm[0]))
        c['version'] = version
        c['purl'] = f'pkg:golang/{c["name"]}@{version}'
        if c.get('cpe'):
            c['cpe'] = set_cpe_version(c['cpe'], version)
        for p in c.get('properties') or []:
            if p['name'].startswith('syft:cpe23'):
                p['value'] = set_cpe_version(p['value'], version)
    return changed


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('sbom', help="syft's CycloneDX SBOM, rewritten in place")
    ap.add_argument('--root', required=True, help='the rootfs the SBOM was made from')
    a = ap.parse_args()
    with open(a.sbom) as f:
        sbom = json.load(f)
    changed = fix(sbom, a.root.rstrip('/'))
    if changed:
        with open(a.sbom, 'w') as f:
            json.dump(sbom, f, indent=2)
    for path, name, old, new, rpm in changed:
        print(f'{path}: {name} {old} -> {new} (from {rpm})')
    print(f'{len(changed)} Go main module version(s) taken from their rpm')


if __name__ == '__main__':
    main()

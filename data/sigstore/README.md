# Sigstore trusted root

`trusted_root.json` is Sigstore's public-good trust root: the Fulcio certificate authorities, the
Rekor transparency-log keys, the CT log keys and the timestamp authority. It is installed into the
rootfs as `/usr/share/hex/sigstore/trusted_root.json`, where `hex_verify_update` (the release check
`hex_install` runs before an update) hands it to `cosign verify-blob --trusted-root`. With it, a
keyless release signature verifies **offline**; without it, cosign tries to fetch the root from
`tuf-repo-cdn.sigstore.dev` and fails on a system with no Internet access.

It is the system being updated that checks the new release, against the copy it shipped with. A
release signed after a Sigstore key rotation that this copy predates will not verify there, and
`hex_verify_update` says so. Refresh the file in every release, so no system falls far behind.

## Refreshing it

Let cosign fetch the current root through Sigstore's TUF repository, which checks it against the
TUF root of trust, then copy the result:

```bash
cosign initialize        # refreshes ~/.sigstore/root/tuf-repo-cdn.sigstore.dev/ via TUF
cp ~/.sigstore/root/tuf-repo-cdn.sigstore.dev/targets/trusted_root.json data/sigstore/
git diff data/sigstore/trusted_root.json   # expect additions; a removed key needs explaining
```

Then run `scripts/tests/test_hex_verify_update.sh`: it verifies a real keyless bundle offline
against this file.

Current copy: fetched 2026-10-07; sha256 `6494e21ea73fa7ee769f85f57d5a3e6a08725eae1e38c755fc3517c9e6bc0b66`.

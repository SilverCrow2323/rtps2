# NEXUS — storage, package and boot safety

1. Treat every catalog entry, archive filename and `title.cfg` as untrusted metadata.
2. A package is installable only when a versioned recipe defines its archive format, exact destination mapping, dependencies, expected files and verification policy.
3. PSU is a savedata archive format, not a generic ZIP. SAS-compliant packages and application-specific recipes require dedicated handling.
4. Reject absolute paths, traversal, duplicate destinations, symlink escapes, unexpected file types and writes outside an explicitly approved target root.
5. Never infer a destination from a package title or `title.cfg` alone.
6. Unknown free space is `unknown`, never `pass`. Updates should conservatively account for old and new payloads coexisting until replacement is verified.
7. Never automatically overwrite boot-critical or system paths, including BOOT, SYS-CONF, SYSTEM, OSDSYS, FMCB, FHDB, PSBBN and PS2BBL.
8. Do not automate boot-chain updates. Require manual review and an independently recoverable backup.
9. Keep planning, downloading, verification and installation as distinct states. A failed verification must block installation.
10. Host-side tests prove only host-side behavior. They do not prove PS2 I/O safety or compatibility.

The current transaction engine is a reference implementation for sandboxed host tests, not an approved PS2 device writer. Its symlink checks do not eliminate all TOCTOU races; native filesystem behavior needs a separately designed and reviewed implementation.

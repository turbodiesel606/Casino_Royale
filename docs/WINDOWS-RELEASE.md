# Preparing A Windows Release

**Publication HOLD:** the current candidates must not be published. Read the
[R1-R7 redistribution findings](../packaging/windows/REDISTRIBUTION.md) and
[status index](../For-Agent/Task-Report/windows-release/STATUS.md) first.
Local build/install test success does not establish legal, privacy or clean-machine
acceptance. Scripts can still create unapproved candidates; no automatic gate or
upload clears the hold.

The maintained [workflow reference](../For-Agent/Docs/windows-release.md) describes
toolchain, version identity, deployment, archive trust and limitations.
[build.md](../For-Agent/Docs/build.md) is the only executable command catalog.
Use [BUILDING](BUILDING.md) for SDK/preset setup and
[WINDOWS-INSTALLER](WINDOWS-INSTALLER.md) for installation behavior.

1. Build the production Windows x64 Release target with exactly Qt 6.10.3; use
   the separate test tree for backend validation.
2. Install into a new absolute staging prefix. Check Qt/QML/platform/SQLite
   dependencies and the reviewed payload inventory. The common tree excludes CRTs.
3. Create and verify the archive, `.sha256` sidecar and per-file manifest. Preserve
   all old outputs. Canonical examples name retained successful stage-13 paths;
   choose new paths consistently for another run.
4. Compile Online and Local Setup from that exact manifest and matching generated
   Release identity. Validate the locked Visual Studio runtime inputs.
5. Apply the [acceptance checklist](../tests/deployment/Windows-Release-Checklist.md)
   to these exact bytes and record PASS/FAIL/NOT_RUN by evidence level. Audit the
   actual archive, installer CRTs and embedded installer engine.

Version comes from root `project(JobTracker VERSION ...)`, currently `0.1.0`.
Windows metadata, generated installer identity and asset names follow it; QML
module version is independent. The temporary icon is
`packaging/windows/JobTracker.ico`. Generated files are private build inputs.

The archive assets use `JobTracker-<version>-windows-x64.7z`, with appended
`.sha256` and `.manifest.json`. Online Setup embeds the verified digest and fixed
GitHub `v<version>` asset URL documented in the workflow reference; it never uses
`latest` or trusts a newly downloaded checksum. A new release needs a new Setup.
The common archive alone is not a clean-machine distribution.

Preserve private source, logs, `installer-build.json`, generated includes,
`work/`/`verify-*`, resource inspections and audit metadata. The current executable
contains recoverable QML despite having no loose project QML in the archive;
the private-source requirement is unresolved. Only separately approved assets
may be handed off for manual publication to the configured Releases repository.
This guide authorizes no Git/GitHub writes.

## Manual Publication After Clearance

The sequence below is a future owner-controlled handoff, not permission to
publish the current candidates. Resolve R1-R7, confirm the private-source and
third-party redistribution policy, and complete the required local/clean-machine
acceptance first. Any payload, notice, resource, runtime or signature change
requires fresh artifacts, hashes and applicable verification.

1. Use the canonical commands to build the production Release and separate test
   tree, run tests, stage into a fresh absolute prefix, then create and verify
   the matching archive, SHA-256 sidecar and public manifest. Audit those exact
   bytes and prepare reviewed customer legal/source materials.
2. Obtain the owner's explicit approval of the exact asset names and hashes.
   Manually publish the approved archive and accompanying approved materials to
   [turbodiesel606/Releases](https://github.com/turbodiesel606/Releases) under
   `v<version>`. Preserve the fixed asset basename; never substitute `latest`.
   The common archive requires the installer's runtime policy and is not a
   standalone clean-machine package.
3. Retrieve the public archive and verify it against the locally approved
   SHA-256. A missing asset, HTTP failure, or hash mismatch blocks the public
   download/install check; do not replace the trusted digest with a remote one.
4. Build the matching version-specific Online Setup from the same approved
   local manifest and generated identity. It embeds that archive's expected
   hash and fixed version URL. Earlier pre-publication Setup builds are local
   candidates and do not establish public end-to-end success.
5. Validate the actual Online Setup through the public URL in disposable Windows
   acceptance environments, including required scope/runtime/UI/data cases.
   Record its final hash and obtain owner approval for that exact installer;
   then manually attach only the approved installer to the same release.

Do not overwrite an approved versioned archive with different bytes after Setup
has been validated. Keep build logs, generated includes, private metadata and
audit evidence out of the release. Final staged evidence and remaining blockers
are recorded in [stage 16](../For-Agent/Task-Report/windows-release/16.md).

Code signing, automatic updates, CI/CD and Linux packaging remain deferred.
Future signing changes bytes and must precede the corresponding final hashes;
see the workflow reference for ordering. Stage completion is not release approval.

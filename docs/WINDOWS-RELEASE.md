# Preparing A Windows Release

**Publication HOLD:** current candidates must not be published. The current
[license issues](../For-Agent/Task-Report/offline-installer/LICENSE_ISSUES.md) and
[redistribution hold](../packaging/windows/REDISTRIBUTION.md) remain separate
from technical completion. A successful build or DONE stage is not approval.

Use [build.md](../For-Agent/Docs/build.md) as the sole executable command guide,
[BUILDING](BUILDING.md) for toolchain/presets, and
[WINDOWS-INSTALLER](WINDOWS-INSTALLER.md) for lifecycle/data behavior.
The [workflow reference](../For-Agent/Docs/windows-release.md) explains ownership
and evidence boundaries.

1. Build Windows x64 production Release with Qt 6.10.3; run applicable backend
   tests in their separate test trees.
2. Install into a fresh absolute staging prefix. Validate the reviewed Qt/QML,
   platform and SQLite payload. The common stage excludes MSVC CRTs.
3. Build `Installer/JobTracker.iss` through `Installer/Build-Installer.ps1` with
   Inno Setup 7+ and authorized local Microsoft runtime inputs matching the lock.
   The wrapper embeds the direct stage, App-Local CRTs, official redistributable
   and MSVC notice. Missing or mismatched inputs fail; no dependency is downloaded.
4. Record the single `Installer/output/JobTracker-Setup-<version>.exe` and its
   matching private build JSON. Preserve an existing candidate before rebuilding;
   the wrapper will not overwrite it.
5. Apply the [offline checklist](../tests/deployment/Windows-Release-Checklist.md)
   to those exact bytes. Keep static validation, native harnesses, actual scoped
   installation, application/data checks and disconnected clean-host evidence
   separate. Record PASS, FAIL and NOT RUN honestly.

Root CMake `project(VERSION)` is the only application version source. Generated
identity is private under `<build>/Installer/<configuration>/`; the temporary
icon is `Installer/JobTracker.ico`. The internal binary remains
`JobTrackerApp.exe`. Embedded QML resources are allowed by the approved
requirements; loose C++/build/test/internal-document/PDB files must not ship.

The required distribution is one offline Setup EXE. It needs no GitHub release,
application archive, sidecar checksum, remote payload or installer-time network.
There is no Online/Local release-mode choice, updater or CI/CD pipeline.
Linux Debug/Release builds and tests remain supported; Linux packaging is out
of scope.

Private payload/runtime snapshots, generated includes, logs, audit JSON and
`installer-build.json` stay outside customer output. Stage reports retain exact
paths and hashes; their commands describe dated runs and must not be replayed
into old evidence directories. No whole build/dist/work directory is a release
asset.

Unsigned local builds remain valid. Optional owner-authorized
[SignTool steps](../For-Agent/Docs/build.md#optional-installer-signing) sign the
finished Setup and record its digest afterward. Signing application bytes must
precede staging/input hashes. Signing does not establish redistribution rights.

The numbered workflow starts at [00_RUN_NEXT](../Tasks/Offline-Installer/00_RUN_NEXT.md)
and [STATUS](../Tasks/Offline-Installer/STATUS.md). Resolve the separate legal
findings and applicable acceptance gaps before any future, explicitly authorized
publication. This guide does not authorize Git writes, uploads or publication.

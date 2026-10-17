---
name: windows-release-packaging
description: Prepare or diagnose JobTracker Windows Release staging, the offline Inno Setup installer, local runtime inputs, and release handoffs. Use for the numbered Offline-Installer workflow or bounded packaging work; use cmake-build-debug for ordinary development builds.
---

# Windows Release Packaging

Work in the existing JobTracker checkout. For a numbered task, enter through
`Tasks/Offline-Installer/00_RUN_NEXT.md` and select the lowest non-DONE stage
from `Tasks/Offline-Installer/STATUS.md`. Resume BLOCKED or IN_PROGRESS work;
read only that stage, binding requirements and directly relevant handoff evidence.
The 11-stage offline plan supersedes the historical Windows-Release workflow.

## Sources Of Truth

- `For-Agent/Docs/build.md` owns executable commands and verification provenance.
  Do not maintain another command catalog in this skill.
- `Tasks/Offline-Installer/00_REQUIREMENTS.md` owns approved requirements.
  `For-Agent/Docs/windows-release.md` describes implemented behavior.
- `Installer/README.md` maps implementation contracts.
- `packaging/windows/REDISTRIBUTION.md` preserves publication HOLD;
  `For-Agent/Task-Report/offline-installer/LICENSE_ISSUES.md` carries current
  license/provenance/owner issues.
- `For-Agent/Docs/testing.md` and `tests/deployment/Windows-Release-Checklist.md`
  separate local suites from actual UI/data/clean-host acceptance.
- Stage reports are dated handoffs, not proof that current inputs match.

## Preparation And Verification

Inspect the scoped diff, existing helpers, generated identity, reviewed inventory
and runtime lock before edits. Keep normal builds, explicit Qt staging, offline
Setup compilation and standalone verification separate. Preserve Windows/Linux
Debug/Release production/test isolation. Use `cmake-build-debug` for CMake
execution and its mandatory elevated-tool-access and process-wait rules.

Use a fresh absolute stage and fresh ignored build/dist fixtures, propagating
paths consistently. Preserve old outputs, failed attempts and user files.
Never package a development/test runtime or change production to support tests.

Root CMake owns the version. Build `Installer/JobTracker.iss` through
`Installer/Build-Installer.ps1` with Inno Setup 7+ and verified local runtime
inputs. The native x64 `Installer/output/JobTracker-Setup-<version>.exe` is the
only required end-user artifact. No application archive, GitHub payload, remote
hash, Online/Local release mode or installation-time network is required.
The wrapper refuses an existing same-name EXE; preserve it before rebuilding.

CurrentUser uses App-Local CRTs. AllUsers embeds the authentic Microsoft x64
redistributable and runs it only when needed. Missing/mismatched local inputs
must fail clearly; never download silently, refresh the lock, bypass inventory
or relax integrity checks. Private snapshots/logs/includes/`installer-build.json`
remain under the selected build's `Installer/work/<id>/`, outside customer output.

CurrentUser integration requires a non-elevated Windows token and real
registry/shell-folder access; elevated build-tool access does not change that.
Use only guarded owned fixtures. Actual app/data acceptance requires a disposable
account/VM. Preserve same-scope repair/upgrade, opposite-scope rejection/manual
uninstall-reinstall, default data preservation and manual closure. Never force
close processes or weaken a guard to obtain a passing test.

Choose checks for the changed contract and record PASS/FAIL/NOT RUN at the right
evidence level. Reuse standalone suites. Do not rebuild unchanged C++ or run
installers merely for documentation changes. A probe is not app startup,
synthetic versions/data are not real compatibility, and static network-code
inspection is not disconnected clean-Windows acceptance. Preserve unresolved
sharing error 32 and earlier failures.

## Completion And Boundaries

Write `For-Agent/Task-Report/offline-installer/NN.md`, update only established
acceptance evidence, then update `Tasks/Offline-Installer/STATUS.md` last and stop.
DONE means the selected stage's obligations are complete, not release approval.

Preserve historical implementation/reports until the assigned cleanup stage.
Only directly relevant build/test/installer documentation and skills may change;
`src/`, application QML, unrelated skills/workflows and immutable task inputs
remain protected. Embedded QML resources are allowed by approved requirements.

No Git writes, publication, CI/CD, updater, Linux packaging or licensing
remediation is authorized by this skill. Unsigned builds remain supported.
Optional signing follows the canonical procedure with an authorized certificate;
never store secrets or infer rights/clearance from a signature.

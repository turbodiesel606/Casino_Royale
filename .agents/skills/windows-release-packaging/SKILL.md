---
name: windows-release-packaging
description: Prepare or diagnose JobTracker Windows Release staging, archives, Inno Setup installers, runtime inputs, and release handoffs. Use for the numbered Windows Release workflow or bounded packaging work; use cmake-build-debug for ordinary development builds.
---

# Windows Release Packaging

Work in the existing JobTracker checkout. For a numbered task, enter through
`Tasks/Windows-Release/00_RUN_NEXT.md` and select exactly the lowest non-DONE
stage from `For-Agent/Task-Report/windows-release/STATUS.md`. Resume BLOCKED or
IN_PROGRESS work; read only that task and directly relevant handoff evidence.

## Sources Of Truth

- `For-Agent/Docs/build.md` owns all executable commands and their verification
  provenance. Do not maintain another command catalog in this skill.
- `For-Agent/Docs/windows-release.md` describes implemented behavior;
  `windows-release-requirements.md` beside it records accepted requirements.
- `packaging/windows/README.md` documents implementation contracts;
  `packaging/windows/REDISTRIBUTION.md` owns the actual-payload publication hold.
- `For-Agent/Docs/testing.md` and `tests/deployment/Windows-Release-Checklist.md`
  distinguish local suites from real UI/clean-machine acceptance.
- Stage reports are dated handoffs, not proof that current source/artifacts match.

## Preparation And Verification

Inspect the scoped diff, existing helpers, generated identity and relevant
manifest/lock before changing anything. Keep normal builds, explicit Qt staging,
archive generation and Setup compilation separate. Preserve Debug/Linux workflows
and the one-way test-to-production dependency. Use `cmake-build-debug` for CMake
execution and its mandatory build-access/wait rules.

Use a fresh absolute install prefix and fresh ignored `dist/` output/fixture
directories, propagating them through dependent commands. Canonical stage paths
are retained evidence, not reusable empty directories. Do not remove old outputs,
failed attempts or user files. Never package a development/test runtime.

Root CMake owns the version; the generated Release identity, archive filenames,
manifest and compiled fixed GitHub URL/digest must agree. Do not use `latest`,
accept a downloaded replacement trust anchor, bypass the reviewed payload
inventory or silently refresh the MSVC lock. Verify exact artifact hashes.
Keep private logs, includes, extracted copies and audit evidence out of publication.

CurrentUser integration needs a non-elevated Windows token and its real
registry/shell-folder access; elevated build-tool access does not change that.
Use only owned test fixtures. Application startup and actual data acceptance need
a disposable account/VM. Keep shared SQLite/CV data, equal-version coexistence,
manual closure and default-preservation safeguards intact. Do not force-close
processes or weaken a guard to obtain a passing test.

Choose checks for the changed contract. Reuse existing standalone suites and
record PASS/FAIL/NOT_RUN at the right evidence level. Do not rebuild unchanged
C++ or run real installers merely for documentation changes. A test probe is not
application startup, a synthetic version is not historical compatibility, and
safe rejection of a missing asset is not successful public installation.

## Completion And Boundaries

Preserve unresolved acceptance findings and the redistribution hold. DONE means
the selected stage's obligations are complete; it never means permission to
publish or legal clearance. Owner/license/source-policy decisions are not
silently resolved by packaging changes.

For a numbered stage, write `For-Agent/Task-Report/windows-release/NN.md` with
actual changes, commands, evidence and limits, then update STATUS and stop.
Keep historical reports and task inputs unchanged. No Git writes, publication,
CI/CD, Linux packaging, automatic updates or signing are authorized by using this
skill; follow the user's explicit scope and existing project policies.

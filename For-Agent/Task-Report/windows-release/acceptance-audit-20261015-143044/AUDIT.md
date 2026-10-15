# JobTracker Windows Release — Acceptance Audit

Created: 2026-10-15 15:09 local time (Asia/Baku, UTC+04:00).

**Not all expected results are established. Implementation completeness: NO-GO for unconditional acceptance. Publication readiness: NO-GO.** The build/packaging/Online mechanisms are substantial and execute locally, but a concrete directory-relocation mismatch, an unmet private-source requirement, unresolved redistribution materials/decisions, unavailable original requirements and mandatory unperformed behavioral acceptance prevent a complete acceptance claim. Task 14's audit and task 15's documentation are real delivered work; their existence does not resolve their reported release blockers.

This was an audit only. No findings were fixed; no next numbered stage was executed; no Git mutation/publication, production installation/uninstallation, system runtime installation, real personal-data cleanup or JobTracker application launch occurred. Only new audit outputs and isolated generated verification directories were created. The native Online test harness copies fixture files under its own directory, has no JobTracker uninstall registration/shortcuts and never launches the app.

## Scope, identity and initial state

- Root: `D:/Project_CV/Root`; all command entry points ran there in PowerShell.
- HEAD: `e3a0ac480c845f7a1d4d4c25b320e9a88d00f4fd` (unchanged).
- Primary checklist: `Tasks/Windows-Release/JobTracker_Windows_Release_16_Tasks_Acceptance.md`; SHA-256 `cd900db98d8f7fb90dc45219f59652cd2d05728cd2c97d5827022b892e3a1d3c`.
- Audit instructions SHA-256: `91c60ee694008fc01dcbfc372fed9b56fe975482e564dc3f5dec6f6d111a9f82`.
- Initial existing state: **12 modified tracked files and 98 untracked project files**, excluding this audit directory. The 12 tracked files are the two existing build/review skills, .gitignore, AGENTS.md, BuildCommands.txt, CMakeLists.txt, both preset files, build.md, testing.md, README.md and qt_bins.txt. The release scripts/tasks/reports/docs already existed as untracked work. No pre-existing change is attributed to this audit.
- Complete initial/final state and diffs: `snapshots/git-status-before.txt`, `git-status-after.txt`, `working-diff-before.patch`, `working-diff-after.patch`, HEAD snapshots; neither status has staged entries. Empty cached-diff output did not produce a patch file. The before Git snapshot was captured after creating the audit helper; entries inside this new audit directory are excluded from initial-state counts.
- Coverage: **166/166 checkbox items**, all 16 acceptance paragraphs and 23 general/artifact requirements: **205 separately assessed rows** in [COVERAGE.md](COVERAGE.md), with source-line references and separate implementation/verification states.
- Tools actually observed: PowerShell 7.6.5; CMake/CTest 4.0.2; Qt 6.10.3.0; VS2022 x64, MSVC 19.44.35229.0 (toolset directory14.44.35207); Windows SDK10.0.26100.0; host Windows 10 build19045; **Inno compiler engine6.7.1 from actual compilation output**. ISCC file ProductVersion reports0.0.0.0 and is not used as its engine version. MSVC runtime lock14.44.35211.0 remains unchanged.

## Summary of all 16 tasks

| Task | Implementation / acceptance result | What was verified | What remains |
|---|---|---|---|
| 01 | IMPLEMENTED | Current instructions, ignore rules, requirements and report handoff inspected. | Historical initialization accepted as progression. |
| 02 | IMPLEMENTED; verification bounded | Fresh Release configure/build; copied Debug 18/18; lifecycle inspected. | Linux and actual developer GUI/debugger checks NOT_RUN. |
| 03 | IMPLEMENTED with owner housekeeping | Exact Qt 6.10.3, VS2022 x64, portable presets; current configure and copied tests. | Linux NOT_RUN; tracked user preset needs authorized index cleanup (RC3). |
| 04 | IMPLEMENTED; app acceptance pending | Fresh148-file external staging; isolated Qt/QML/QSQLITE/image/App-Local probe. | Actual clean-Windows JobTracker/graphics NOT_RUN; DXC/DXIL warning. |
| 05 | IMPLEMENTED; visual checks pending | Both current EXEs: seven exact icon frames, version 0.1.0/0.1.0.0, neutral metadata. | Explorer/taskbar/shortcut/native UI appearance NOT_RUN. |
| 06 | PARTIAL | Fresh7z, exact checksum,148 extracted hashes; 19 package checks. | Private raw QML remains embedded in payload EXE. |
| 07 | IMPLEMENTED foundation; acceptance PARTIAL | Both retained Setup identities/hashes; fresh Online compile; scope/UI source inspected. | Real AllUsers/UAC/coexistence and interactive UI acceptance NOT_RUN. |
| 08 | PARTIAL overall; Online mechanism implemented | Fresh version/hash-locked Setup; 24 native local pipeline checks; public HEAD404. | Private-source criterion fails; successful public Online installation NOT_RUN. |
| 09 | Technical implementation present; acceptance PARTIAL | Locked 10 CRT inputs/vendor hashes; actual App-Local probe; system branch inspected. | Real missing-runtime install/failure/reboot/UAC and legal entitlement incomplete. |
| 10 | PARTIAL | Version/metadata/equal-version/coexistence guards inspected; prior fixtures verified. | Explicit upgrade relocation rejected (RC1); real history/dual-scope data checks missing. |
| 11 | PARTIAL against strict acceptance | No forced-close design; exact-file/retry/cancel gates; retained native evidence. | Late original-state rollback unproved (RC2); actual app/other-session checks missing. |
| 12 | Default-safe implementation; acceptance PARTIAL | Actual storage identity; default-No/ownership/other-copy/path/handle safeguards inspected. | Post-consent cleanup nontransactional (RC4); real consent/UAC/late-failure checks missing. |
| 13 | PARTIAL acceptance evidence | Current build/stage/package/compile/probe, copied backend 18+18, package19, Online 24. | Critical VM/UI/AllUsers/runtime/history cases NOT_RUN; error32 unresolved. |
| 14 | Audit IMPLEMENTED; strict notice acceptance PARTIAL | 148-file/10 CRT inventory and current SPDX/provenance checks; privacy failure reproduced. | R1–R7 publication hold; no complete delivered notices/source arrangement (RC5). |
| 15 | IMPLEMENTED | 43 command blocks parse; 6 presets/12 scripts resolve; 43 historical/agent inputs preserved. | Linux execution and listed external acceptance remain explicitly NOT_RUN. |
| 16 | PARTIAL | Report,36 retained-map identifiers, exact artifacts and manual release order inspected. | Original prompt unavailable; DONE is not full strict-checklist acceptance. |

## Current executed checks and their limits

Exact invocations, output locations and exit codes are in [COMMANDS.md](COMMANDS.md); machine-readable records are in `commands.json`. `CURRENT_AUDIT` means performed in this audit. `PREVIOUS_EVIDENCE` means retained prior results whose applicability is explicitly limited.

| Check | Current result | Evidence and practical limit |
|---|---|---|
| Fresh Release configure | PASS after environmental retry | C01 failed because sandbox SDK discovery was denied access; C02 used permitted host-tool access and returned0. Both logs retained. No Windows administrator/UAC elevation was requested. |
| Fresh production Release compile/link | PASS, exit 0 | C03; new external temporary tree; testsOFF; JobTrackerApp target. Every yielded process was awaited to final exit. |
| Qt staging | PASS, exit 0 | C05; fresh absolute external prefix; 148 files. Missing DXC/DXIL warning retained. |
| Archive and trust manifest | PASS, exit 0 | C06; 148 extracted-file checks and exact final digest; C11 package regression19/19. |
| Current Online Setup compilation | PASS, exit 0 | C07; exact fresh archive trust anchor; Inno 6.7.1; locked runtime inputs. Compilation did not install or publish. |
| SDK/PATH-isolated runtime probe | PASS, exit 0 | C08; QML imports, windows platform, in-memory QSQLITE SELECT, SVG/JPEG, and actual private MSVCP140/VCRUNTIME140/VCRUNTIME140_1 module paths. Probe source enables test mode and does not construct App/AppBootstrap. |
| Copied Release backend suites | PASS18/18,20.23s | C09; retained binaries copied byte-for-byte to a fresh isolated tree. Only copied CTest output/executable paths changed. This is a current run of retained binaries, not a fresh test-target build. |
| Copied Debug backend suites | PASS18/18,23.35s | C10; same isolation; process-local TEMP/TMP and disabled QML disk cache. Temporary fixtures supply explicit StoragePaths. Fresh Debug compile was not repeated. |
| Native Online local harness | PASS24/24, exit 0 | C12; valid/redirect, offline/404/503, corrupt/truncated/oversized/hash, cancel/extract-cancel/retry, malformed/traversal/reparse/tampered payload cases. Test-only loopback endpoint; no production registration or real app startup. |
| PE icon/version identity | PASS, exit 0 | C17; both retained and fresh EXE have seven ICO frames exactly matching source and correct neutral version/packaging identity. Visual shell acceptance NOT_RUN. |
| Current private-source artifact proof | **FAIL criterion**; inspection exit 0 | C13 recovered17835 Main.qml bytes in memory, matching current source SHA; 47180-byte resource array exists in archived EXE; 28 raw QML entries. No recovered source was saved or printed. |
| Current dependency/docs static collector | PASS bounded comparisons, exit 0 | C19; 148 paths, 10 CRT inputs, 6 SBOM inputs, 3 Lottie binaries; 43 canonical blocks/6 presets/12 scripts; 43 historical/agent hashes. This confirms the actual unresolved hold, not legal clearance. |
| Current public payload URL | HTTP404; successful public install **NOT_RUN** | C16 at 2026-10-15 14:44:25Asia/Baku. Earlier web cache miss and denied sandbox HEAD are retained but not treated as evidence of absence. |
| Integrity | PASS | Final comparison:414 protected entries/345 files, including 124 src files, unchanged; 171 monitored existing artifacts unchanged. See below. |

No actual clean Windows 10/11 application acceptance, real AllUsers/CurrentUser registration coexistence, UAC/different account, real missing-runtime execution/reboot, historical application/data upgrade, interactive EN/RU closure/consent, late-uninstall/data-cleanup or public successful Online installation was performed. No prepared disposable environment was supplied/established; the primary host was not substituted for one. Linux configure/build/test/startup remain NOT_RUN. These gaps have specific work items rather than being counted as PASS.

### Historical evidence binding

`snapshots/stage13-input-comparison.json` compared209 current implementation-related paths with stage 13's retained input hashes:208match; the only differing path is packaging/windows/README.md, subsequently updated by documentation stages. This is useful source continuity evidence, not an absolute binary-to-source attestation. All six installer source hashes match retained stage 13 Online metadata; both retained Setup hashes and common archive were recalculated. Prior native cases (Online 25, runtime15, manual-close21, data19, policy54 twice) were inspected as PREVIOUS_EVIDENCE. Current copied tests are explicitly distinguished from current compilation. Old failed logs were not erased or relabeled.

## Actual artifacts and exact-byte relationships

All rows below exist and were rehashed. Candidate version is0.1.0, Windows x64. They are unsigned, unapproved audit/release candidates.

| Artifact path (repository-relative unless absolute) | Bytes | SHA-256 |
|---|---:|---|
| `dist/stage13-package/JobTracker-0.1.0-windows-x64.7z` | 25222031 | `7bd2ab5a1cf35ee7c1748b92d1c3227b9fe46cbb0e0c228ecd07cb139aa60864` |
| `dist/stage13-online/JobTracker-0.1.0-windows-x64-online-setup.exe` | 2645020 | `05dba8ffdee5f93510aab1c9febbad176c9fa795ad3ed7bbf3404d85639c5d40` |
| `dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe` | 26778722 | `1b5ba6e1a4d6b05f576b9048397395417f48bf490cb4ee2f7ecc80772e7136df` |
| `dist/acceptance-audit-20261015-143044/package/JobTracker-0.1.0-windows-x64.7z` | 25223755 | `da5785c57bd946af9e39a38625128343218e21303c3f918b67e180a6a62efa60` |
| `dist/acceptance-audit-20261015-143044/online/JobTracker-0.1.0-windows-x64-online-setup.exe` | 2645047 | `445adcc022b923330afc4ad3b559e4cc0bac2a4d72ae68199f1649b8d45d10fb` |
| `C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production/Release/JobTrackerApp.exe` | 2014720 | `53697cacccaf74d71e9ea79d5861d4a5239573bc4cfad4593c4c30c0dbe2a0dc` |

The fresh build changes only `bin/JobTrackerApp.exe` among the 148 manifest members; all 147 other payload hashes match the retained stage 13 package. The fresh archive hash therefore differs from the retained package. Reproducible C++ binaries across build directories were never required/proved. **Do not mix the fresh Setup with the retained archive, even though both use version 0.1.0.** Each pair embeds its own exact digest; publish neither under the current hold. Fresh per-file manifest and Setup metadata are copied into `snapshots/fresh-package-manifest.json` and `fresh-installer-build.json`.

Retained source/binary binding is inspectable in `snapshots/artifact-verification-before.json` and `after.json`; source/ICO identity in `logs/17-identity-results.json`. The fixed URL remains `https://github.com/turbodiesel606/Releases/releases/download/v0.1.0/JobTracker-0.1.0-windows-x64.7z`; no latest endpoint, runtime replacement checksum or fake production URL was introduced.

## Significant findings and requirements that need reconciliation

**P1 implementation/acceptance gaps:** upgrade-directory changes are rejected; private source remains in the archived EXE; complete delivered notices/source/replacement fulfillment and the exact redistribution route are unresolved. Mandatory real-system behavior and original-state rollback guarantees remain unverified. No new proven P0 defect is asserted merely because a risky scenario was not run.

1. **RC1 — REQUIREMENT_CONFLICT, relocation.** Checklist line188 and task 10 work1 say “unless the user explicitly changes it.” `packaging/windows/InstallationPolicy.iss:229–230` refuses a different directory for an existing scope; `packaging/windows/README.md:336–341` describes uninstall/reinstall relocation. The conservative implementation does not satisfy that literal acceptance wording. R07 requires an explicit decision or a safe installer fix.
2. **RC2 — REQUIREMENT_CONFLICT / verification shortfall, late cancellation.** Checklist lines 205/210 require cancellation without partial damage and preservation of the original state. `packaging/windows/README.md:442` states “generic rollback does not prove the old version was restored.” `ManualClose.iss:84–115` requests cancellation after a late copy block. A retained rollback log is not byte-for-byte restoration evidence after earlier overwrites. R08 tests the contract before any claimed correction.
3. **RC3 — REQUIREMENT_CONFLICT, local preset tracking.** Checklist artifact line319 says “local only” and “not committed”; task 03 work4 instructs “If already tracked, report the safe untracking operation; do not run forbidden Git mutations.” Both ignore rule and safe instructions exist, but the file remains tracked. Task 03 followed its Git boundary; the final literal repository condition awaits owner-authorized index-only cleanup (R20). This audit did not run it.
4. **RC4 — REQUIREMENT_CONFLICT / guarantee boundary, cleanup.** Checklist line227 says “every ambiguous or unsafe situation results in data preservation.” `packaging/windows/README.md:492–498` explicitly states cleanup is not transactional and completed deletions cannot be restored. `PersonalData.iss:201–221` performs strong preflight; `:223–227` deletes validated handles sequentially only after consent. A late failure is not evidence of accidental/unconsented deletion, but an all-or-nothing reading is not proved. R09 requires explicit interpretation and disposable late-failure evidence.
5. **RC5 — REQUIREMENT_CONFLICT, notice deliverable.** Checklist line260 permits notices to be “included in the release, or explicitly listed as release blockers”; task 14 similarly allows notices or clear blockers. Checklist acceptance line266 asks for “prepared notices.” The audit/blocker alternative is delivered; a full customer notice set is not. R05 resolves delivery, without falsely saying no licensing audit exists.

**Current redistribution/source hold:** R1–R7 in `packaging/windows/REDISTRIBUTION.md:22–28` remain applicable to the actual files. C19 reconfirmed the three Lottie commercial/GPLv3-only records, empty common-payload legal-file set, matching System32 ICU, and current runtime/SBOM inputs. C13 reconfirmed raw private QML. Owner/legal decisions, corresponding sources and compatible replacement, Mesa/LLVM/D3D/Inno exact components and notices remain open. Official links and 2026-10-15 verification date are in `logs/static-review.md`. Inno's compiler banner and permissive current license/product-page wording are recorded without inventing a categorical commercial-use prohibition or clearance.

**Known error32 remains unresolved:** actual old failed gates are at `dist/stage12-policy-tests/currentuser/version-0.0.9.log:258`, `stage12-policy-tests-repeat/currentuser/version-0.0.9.log:314` and `stage12-policy-stage11-control/currentuser/repair.log:388`. No current evidence establishes the lock owner or a fix. Later identical-installer passes do not fulfill an unqualified “all discovered errors fixed” claim (T13-12). R10 calls for causal tracing in a disposable environment, not assumed antivirus attribution, sleeps or weakened guards.

**Original prompt unavailable:** the retained 36 identifier map covers role/objective, sections 1–22 and 22.1–22.13, but `Promt_deploy.docx` is absent. Report 16 itself admits this at lines 55–60. The primary checklist was present and fully audited; comparison to absent original wording was not possible. R19 requires the original or an explicit owner source-of-truth decision.

### Discrepancies with previous completion labels

STATUS contains 16DONE rows and also candidly retains licensing/runtime/VM/public-download limitations. Task 14 legitimately completed an audit that found blockers; task 15 completed maintained documentation. This audit does not erase those accomplishments. However, DONE is not proof that the stricter acceptance wording is fulfilled: T06/T08 privacy, T10 relocation, T11 cancellation preservation, T12 absolute cleanup interpretation, T13 unresolved errors/critical tests, T14 complete prepared notices and T16 original-source comparison require qualifications. Neither STATUS nor any prior report/checklist was edited. All new discrepancies remain here and in COVERAGE/REMAINING_WORK.

## Integrity and audit-created files

**src/ remained unchanged:** before/after snapshots contain the same124 regular files and 12 directories, with identical paths, SHA-256s, lengths, file write timestamps and attributes. No src links were present; the collector records links without following them. all 345 protected project files (414 entries including directories) are unchanged, including CMake/presets/QML/tests/instructions/STATUS/checklist/previous reports. all 171 monitored existing artifact files are unchanged. Git HEAD and working-diff snapshots, plus index columns of both status snapshots, are retained for comparison; new audit outputs are the only new nonignored working-tree entries.

Snapshot scope explicitly excludes `.git`, `.vs`, old `build/` and `dist/` trees from whole-tree hashing; selected171 release-artifact files in those generated trees are separately hashed. No claim of exhaustive hashing of every old generated intermediate or IDE metadata is made. No command targeted old build trees for configure/build/install or old test logs for output.

An initial audit-helper comparison falsely reported timestamp differences because PowerShell automatically deserialized ISO date strings and then removed fractional trailing zeros when serializing them. No mutating project checks continued while this was investigated. A raw string-preserving, property-by-property comparison found zero changes. Only the new audit helper was corrected with `ConvertFrom-Json -DateKind String`; original baseline snapshots were preserved. Initial results remain in `logs/integrity-initial-serialization-result.json` and `artifact-initial-serialization-result.json`; corrected final results are `snapshots/integrity-result.json` and `artifact-verification-after.json`.

New outputs only:
- This audit directory: AUDIT.md, COVERAGE.md, REMAINING_WORK.md, COMMANDS.md, exact requirements/judgments/command data, bounded read-only-input verification helpers, logs and integrity snapshots.
- `C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/`: isolated production build/staging, smoke copy and copied Debug/Release test trees/data.
- `dist/acceptance-audit-20261015-143044/`: fresh package/Online Setup and native Online fixtures.
- `dist/package-tests-3080de2ed9a944138bb5c674a17695ad/`: existing archive-test runner's uniquely named fixture output.

No existing output was deleted; no cleanup is implied by completion. Full logs/fixtures remain local and are not public release assets. The focused documentation consistency check identified the requirements conflicts above; user instructions prohibited editing their existing source documents, so only these new reports record them.

## Decision and next actions

**Implementation completeness — NO-GO for unconditional acceptance.** The release mechanisms largely exist, but strict mandatory criteria are unmet or cannot be established. This is not an assertion that all missing verification means missing code. Task-level and per-item distinctions are explicit above.

**Publication readiness — NO-GO.** Exact-payload redistribution/source obligations, requirement/safety decisions, unexplained maintenance failure, critical unperformed clean-system/runtime/data/UI tests and absent public-payload success remain. NOT_RUN never counts as PASS. Unsigned status must be disclosed; actual signing itself remains explicitly outside scope.

[REMAINING_WORK.md](REMAINING_WORK.md) contains 20 concrete dependency-ordered items with priority, checklist links, required action, expected files, acceptance, dependencies, responsible party and **src/ required: NO/YES/UNKNOWN**. R02 and R10 are UNKNOWN because runtime/startup or the unexplained lock may eventually expose a C++ dependency; any src/ change needs a separate owner decision. All current required verification/material/packaging actions otherwise start outside src/. Optional backend warning investigation is separately marked YES and is not an authorized fix.

First resolve owner licensing/source/requirement choices and provide disposable environments; then correct/finalize packaging and exact legal/source materials; rebuild/freeze and test the final bytes. Only after explicit owner authorization should the approved archive be published, followed by matching Setup creation, actual public clean-system tests, and separately authorized Setup publication. CI/CD, Linux installers, automatic updates and actual signing are deferred future work, not invented failures of this assignment.

The audit stops at these reports. No remaining work has been implemented.





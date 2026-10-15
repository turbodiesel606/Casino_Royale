# Windows Release - persistent task status

Updated: 2026-10-15 13:25 local time (Asia/Baku, UTC+04:00).

Execute exactly one stage through
[`00_RUN_NEXT.md`](../../../Tasks/Windows-Release/00_RUN_NEXT.md).
Update this index after saving the selected stage's report.

States: `PENDING`, `IN_PROGRESS`, `BLOCKED`, `DONE`. Select the lowest-numbered
stage not marked `DONE`; resume blocked or in-progress stages without skipping.

| # | Stage | State | Report | Verification / blocker |
|---:|---|---|---|---|
| 01 | Audit and bootstrap agent instructions | DONE | [01.md](01.md) | PASS: static audit, instruction/command preservation, narrow ignore exception, unchanged inputs, diff check. Build/CTest/GUI/VM: NOT_RUN (documentation-only stage). No stage blocker. |
| 02 | Clean CMake lifecycle without breaking Debug or Linux | DONE | [02.md](02.md) | PASS: Windows Debug production/test builds, CTest 18/18, fresh build without runtime deployment, explicit development runtime and single-file install, static isolation/review. GUI/Linux/clean VM: NOT_RUN; existing warnings documented. No stage blocker. |
| 03 | Normalize CMake presets and Qt 6.10.3 | DONE | [03.md](03.md) | PASS: fresh Debug/Release production builds, final Debug and Release CTest 18/18 each, exact SDK/preset/schema and isolation checks. Initial Release SQLite-driver failure fixed in test-only runtime helper. Linux/GUI/VM: NOT_RUN. Local preset remains tracked; safe untracking documented, not executed. No stage blocker. |
| 04 | Deploy Qt/QML/SQLite into Windows Release staging | DONE | [04.md](04.md) | PASS: Release/Debug builds, three matching 148-file installs (including lowercase Release), isolated Qt/QML/platform/SQLite smoke 1/1, Release CTest 18/18, static import closure and independent review. Initial relative-prefix error corrected. MSVC policy stage 09; GUI/Linux/clean VM NOT_RUN; DXC warning recorded. No stage blocker. |
| 05 | Temporary icon, Windows metadata and unified version | DONE | [05.md](05.md) | PASS: Release/Debug builds and staging, exact seven-frame icon and version-resource checks, generated packaging/installer identity, Release CTest 18/18, isolated runtime smoke 1/1, 208 preserved-input hashes. GUI/Linux/Inno/VM NOT_RUN. No stage blocker. |
| 06 | Versioned application archive and SHA-256 | DONE | [06.md](06.md) | PASS: two byte-identical 148-file 7z archives/manifests, exact SHA-256/extraction verification, 19 packaging checks, isolated extracted Qt/QML/SQLite smoke, independent review and 213 preserved-input hashes. Workflow-only; fresh C++ build/backend CTest/Inno/GUI/Linux/VM NOT_RUN. No stage blocker. |
| 07 | Inno Setup foundation: two installation modes and UI | DONE | [07.md](07.md) | PASS: Inno 6.7.1 compile, distinct scope IDs, 148-file English/Russian current-user install/reinstall/conflict/uninstall checks, both desktop choices, 19 archive regressions and independent review. Initial padded-metadata check corrected; sandbox install denial resolved by non-elevated host tests. AllUsers/UAC/coexistence/GUI/VM NOT_RUN. No stage blocker. |
| 08 | GitHub version-locked Online Installer and secure downloads | DONE | [08.md](08.md) | PASS: Online/Local Inno builds, fixed version/hash inputs, 25 native download/extraction/failure checks, 148-file hashes, cancellation/cleanup, traversal/junction rejection and independent review. Public asset 404 fails safely; successful public download/GUI/AllUsers/VM NOT_RUN. No stage blocker. |
| 09 | Implement hybrid MSVC Runtime deployment | DONE | [09.md](09.md) | PASS: Online/Local builds, signed and pinned x64 runtime inputs, 15 runtime checks (96 PEs/994 imports/30 policy assertions), native Microsoft download/cancellation/hash rejection, real non-elevated current-user install/reinstall/uninstall, App-Local loaded-module smoke, staging CTest 1/1, 24 online regressions and independent review. Real AllUsers/missing-runtime/UAC/reboot/GUI/VM NOT_RUN. No stage blocker. |
| 10 | Upgrades, reinstalls, downgrades and coexistence safety | DONE | [10.md](10.md) | PASS: Online/Local builds, 54 policy/current-user integration checks, 24 online and 12 runtime regressions, independent review and preserved inputs. Strict equal-version coexistence, validated registry/PE/path identity and safe relocation/shortcut rules. Real AllUsers/dual-scope/multi-session/GUI/VM/historical releases NOT_RUN. No stage blocker. |
| 11 | Manual closure of running JobTracker before changes | DONE | [11.md](11.md) | PASS: final Online/Local builds, 21 manual-close checks, 54 policy/current-user, 24 online and 12 runtime regressions, independent review and input preservation. Exact-file EN/RU gates, native late-copy cancellation, unsafe override rejection; unattended Setup uses /SUPPRESSMSGBOXES. Interactive Retry/native delayed-uninstall/other-session/AllUsers/VM NOT_RUN. No stage blocker. |
| 12 | Safe uninstall and optional deletion of personal data | DONE | [12.md](12.md) | PASS: Online/Local builds, 19 data-safety/current-user checks, 21 manual-close/24 online/12 runtime regressions, independent review and preserved inputs. Default-No cleanup only for verified non-elevated CurrentUser; all unsafe/shared/elevated cases preserve data. Policy integration FAIL on sharing error 32, reproduced with unchanged stage-11 installer; GUI/UAC/AllUsers/VM NOT_RUN. No stage-12 implementation blocker. |
| 13 | Installer integration tests and clean-machine verification | DONE | [13.md](13.md) | PASS: Release production/test builds, CTest 18/18, fresh 148-file staging/archive and both Setup builds, staging CTest 1/1 plus App-Local smoke, 19 archive / 54 policy twice / 21 manual-close / 19 data / 25 online / 15 runtime checks, RU Unicode-path lifecycle, five real fixtures removed, independent evidence review. Public asset 404; GUI/startup/AllUsers/VM/historical upgrades NOT_RUN. Earlier error 32 did not recur; root cause unknown. |
| 14 | Redistribution/license audit and release safeguards | DONE | [14.md](14.md) | PASS: exact 148-file archive, ten CRT DLLs, 144 dependency/attribution records, installer-engine inventory, 24 native security checks and independent review. Publication HOLD: three GPL-only/commercial Lottie files, missing legal/source/replacement material, external-binary provenance, recoverable private QML and owner entitlement. GUI/VM/legal acceptance NOT_RUN. Audit deliverables complete. |
| 15 | Final synchronization of Codex instructions and developer docs | DONE | [15.md](15.md) | PASS: 218 static documentation/input checks, 76 local links, 43 command blocks, three validated skills, preset/ignore/diff checks and unchanged candidate hashes. Canonical workflow, developer guides and release hold synchronized. Fresh build/CTest/native suites/GUI/Linux/VM/public download NOT_RUN (documentation-only). No stage blocker. |
| 16 | Independent final review and manual GitHub release handoff | DONE | [16.md](16.md) | PASS: two independent bounded reviews with no new actionable defect, all 36 requirement-map rows, 218 static documentation/retained-evidence checks, 44 candidate/source/CRT checks, 20 final-artifact links and input preservation. Exact public asset HEAD remains 404; public install/GUI/AllUsers/VM/historical upgrades NOT_RUN. Manual owner-approved handoff documented; publication HOLD and error-32 risk remain. |

Next task: none - all 16 numbered stages are DONE. Stop; no publication is authorized.

Active blockers: no essential implementation blocker for completed stage
deliverables 01-16. **Publication HOLD** from stage 14: R1-R7 in
[REDISTRIBUTION.md](../../../packaging/windows/REDISTRIBUTION.md).
The audit found three Qt Lottie binaries offering GPLv3/commercial terms without
LGPL, missing delivered Qt/third-party notices and corresponding-source/replacement
materials, unresolved Windows ICU/Mesa/D3D/Inno provenance and rights, recoverable
private QML in the actual EXE, and unconfirmed owner/Visual Studio entitlement.
The raw SDK SBOM checksum correspondence is also unresolved; kit hashes and
signatures are separate evidence. Stage 14 is DONE because its explicit scope
is inventory/checklist/blockers, not legal clearance. No candidate is approved.
Exact file/hash/license evidence, corrected audit-script failures, 24 passing
native checks and remaining limits are in [14.md](14.md) and `14-evidence/`.
Stage-13 artifact bytes remain unchanged. Stage 15 completed documentation and
skill synchronization while preserving this hold; see [15.md](15.md),
[the implemented workflow](../../Docs/windows-release.md), and
[the canonical command catalog](../../Docs/build.md). Its checks are static and
read retained evidence; no runtime acceptance was repeated. Stage 16 completed
the independent source review, full retained-section requirement matrix, exact
artifact/source correspondence and manual release handoff; see [16.md](16.md)
and [the independent review](../../Review/cpp-review-2026-10-15-1318-stage16-release-handoff.md).
It found no new actionable code defect and made no production/installer change.
The final static checker passed 218 checks; the candidate/source collector passed
44 checks. A fresh host-access HEAD at 2026-10-15 13:14:45 +04:00 returned HTTP 404
for the exact versioned archive; successful public download/install is NOT_RUN.
The initial sandbox socket denial and corrected evidence-parser failure are
retained. Stage 16 added the explicit archive-publication/public-verification/
matching-Online-Setup sequence to the developer guide. These completed review
deliverables do not clear the hold or external acceptance gates.

Active verification concern: stage-12 policy/current-user integration failed twice
on Windows sharing error 32 during upgrade; the unchanged stage-11 installer
reproduced the same safe cancellation during repair. Lock owner is unknown.
Stage 13 passed two fresh 54-check runs with byte-identical stage-12 installers;
the error did not recur. No root-cause fix or elimination is claimed; retain this
intermittent reliability risk and the manual-close safeguard. Real historical
upgrade/repair, AllUsers/coexistence, interactive UI and clean-machine acceptance
remain unresolved. The user confirmed no prepared disposable account/VM.
Stage 13 artifacts are under `dist/stage13-package/`, `dist/stage13-online/` and
`dist/stage13-local/`; exact hashes, commands, complete acceptance matrix,
retained evidence and limitations are in [13.md](13.md). The new common archive
and installers match the prior stage-06/stage-12 bytes. All five real stage-13
CurrentUser fixtures were uninstalled, scoped registrations/shortcuts were
absent and synthetic sentinels remained. No actual personal data was accessed.
Public archive URL was rechecked in stage 13 and returned 404; safe rejection
passed, successful public installation remains NOT_RUN. Stage 14 audited these
unchanged candidates and recorded the publication hold above.
Stage 12 final installers are under `dist/stage12-online-final/` and
`dist/stage12-local-final/`; exact commands/hashes, 19 passing data-safety checks,
21/24/12 passing regressions, failed policy/control evidence and manual limits
are in [12.md](12.md). All seven real test installations were uninstalled.
Personal data is preserved by default; explicit EN/RU post-uninstall cleanup
is limited to a verified, non-elevated CurrentUser account. Elevated/AllUsers,
silent, shared, incomplete and unverifiable cases preserve data. Real UI consent,
cross-account UAC/dual scope, ACL denial, concurrent mutation and clean VM remain
unverified. Cleanup is irreversible and not transactional; unregistered copies
cannot be exhaustively discovered. No actual personal data was read/deleted.
Stage 11 final installers are under `dist/stage11-online-final/` and
`dist/stage11-local-final/`; exact commands/hashes, final 21-check manual-close
suite, 54 policy/24 online/12 runtime regressions and corrected native-dialog
evidence are recorded in [11.md](11.md). Both final real CurrentUser fixtures
were uninstalled. No JobTracker/user application process was force-closed.
Unattended Setup requires /VERYSILENT and /SUPPRESSMSGBOXES. Per-file failure
uses acknowledged native cancellation; interactive Retry, native delayed
uninstall timing, AllUsers/other sessions, concurrent-start races and full
old-version restoration after a partial upgrade remain unverified.
Stage 10 reviewed installers are under `dist/stage10-online-reviewed/` and
`dist/stage10-local-reviewed/`; exact commands/hashes, 54-check non-elevated
current-user evidence, 24 online/12 runtime regressions and shared-data/profile
restrictions are recorded in [10.md](10.md). All stage-10 real test installations
were uninstalled. Equal versions are required across scopes; unloaded/unreadable
profiles block all-users changes. Directory relocation requires uninstall with
data preserved, then reinstall. Real AllUsers/dual-scope/VM acceptance is pending.
Stage 09 final installers are under `dist/stage09-online-final/` and
`dist/stage09-local-final/`; exact commands, runtime provenance/hashes, 15-check
evidence, current-user cleanup, App-Local loading and untested system-install
outcomes are recorded in [09.md](09.md). The common stage-06 archive is unchanged.
Stage 08 Online Installer is under `dist/stage08-online-verified/`; exact commands,
hashes, 25-check evidence, public-asset 404 and remaining acceptance limits are
recorded in [08.md](08.md). Local mode remains available with `-Mode Local`.
Stage 07 local installer is under `dist/stage07-verified/`; exact commands,
artifact hashes, current-user test/cleanup results, separate AppIds and
manual AllUsers/UAC/coexistence checks are recorded in [07.md](07.md).
Stage 06 archive/checksum/manifest assets are under `dist/stage06/`; exact commands,
hashes, negative tests and later installer prerequisites are recorded in [06.md](06.md).
Stage 05 executable resources, generated identity hooks, stage-05 staging and
manual shell/installer icon checks are recorded in [05.md](05.md).
Stage 04 staging, isolated runtime checks, corrected install-prefix failure,
MSVC-runtime deferral and missing DXC warning are recorded in [04.md](04.md).
Stage 03 evidence, the corrected initial Release test failure, manual Linux/GUI
checks and tracked-user-preset cleanup are recorded in [03.md](03.md).
The Linux SDK selection now names Qt 6.10.3 but requires validation on Linux.
The shared-data constraint accepted in [01.md](01.md) is now explained with the
implemented coexistence/cleanup policies in the stage-15 documentation.

Release readiness remains blocked/unverified; stages 01-16 DONE record bootstrap,
CMake lifecycle, preset/build/test, common Qt staging, executable identity and
verified versioned archives, the shared installer foundation and a version-locked
Online Installer with local native download/extraction/failure evidence, plus
hybrid MSVC deployment with verified current-user files and prerequisite policy,
plus validated upgrade/reinstall/downgrade and strict equal-version coexistence
rules with local installer-policy and current-user integration evidence,
and exact-file manual-close gates with verified cancellation and process survival,
plus default-safe uninstall and guarded, explicit personal-data cleanup,
and the stage-13 Release build/artifact/native/CurrentUser verification pass with
an explicit matrix and user-confirmed unavailable VM checks, followed by the
stage-14 actual-payload redistribution/security audit and explicit release hold,
then stage-15 synchronization of canonical commands, agent skills and developer
guides with static documentation and retained-input verification, followed by
stage-16 independent final review, requirements reconciliation and manual handoff.
The full policy regression failed in stage 12 and passed twice unchanged in
stage 13; the intermittent sharing issue has no identified cause or proven fix.
The public archive URL returned 404 again in stage 16; successful public download
remains unverified and no artifact is an approved release package.
AllUsers/UAC/coexistence, real missing-runtime installation/reboot behavior,
GUI/graphics/clean-machine checks remain pending. Licensing/source/privacy
acceptance is blocked by the stage-14 findings. Development/test runtime output and executable-only Debug
installs must not be used as release payloads.
No next numbered stage remains. Hold remediation and acceptance need separately
scoped work and fresh evidence; manual publication still requires explicit owner
approval of the exact assets after the blockers are resolved.

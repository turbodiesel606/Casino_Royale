# Stage 5 leftovers review on Windows
Created: 2026-11-04 04:34 local time (Asia/Baku)

## Scope and identity

Windows 10 x64, Qt 6.10.3/MSVC 19.44, CMake/CTest 4.0.2. Reviewed the cumulative R0-to-stage-4 source change and the stage-5 verification materials. Input/output production identity is `5e54d637d8e60bca6a4970791110cf6d8ae693e706ec7ae71104f08c8c1963be`; stage 5 changes only the ShellBaseline test's layout/focus synchronization and refactor reports/evidence. No production entity was added. [Final source manifest](../evidence/05-final-source-2026-11-04-0426.tsv) · [R0 artifact inventory](../evidence/baseline/r0-artifacts.tsv)

Three independent read-only reviewers examined R001–R007/R009 and their direct contracts. The lead checked generated artifacts, build references, dormant resources, baseline membership and the stage-5 helpers. This is a scoped evidence-based inventory, not a claim that every externally used repository artifact can be deleted.

Nothing in this report authorizes automatic deletion. No existing build directory, cache, artifact, historical document or source candidate was removed.

## Proven redundant in the current source

| Exact path / symbol | Origin and original purpose | Why redundant and consumer evidence | Removal risk / disposition |
| --- | --- | --- | --- |
| `src/common/BulkIdSelectionTracker.cpp:48–60`, disabled second `BulkIdSelectionTracker::selectedIds()` | Present in R0; the adjacent comment identifies an older slower alternative retained beside the active implementation. | Entire alternative is under `#if 0`. The active definition is at lines 34–46. The excluded definition cannot have compiled, QML or meta-object consumers. R001 preserved these lines. | Low behavior risk, but removes a historical example and its existing comment. Preserved; any cleanup is a separate request. |
| `src/maintenance/StorageMutationGate.hpp:11–12`, `<chrono>` and `<thread>` | Both were present in R0. Their original reason for inclusion is not established. | Neither the gate declarations nor implementation uses their facilities. Independent review traced direct consumers; actual thread/chrono users provide their own includes. No new helper requires them. | Low source-maintenance risk, with a residual transitive-include/build check if removed. No removal/build-without-includes was performed. Preserved. |

No newly introduced unused production helper, abandoned replacement implementation or duplicated icon definition was established. `CvImportService::removeCompletedFile` was fully replaced; `compensateCompletedFile` has three production callers. The old gate template/disabled alternatives and both inline `PdfIcon` definitions were removed during the implementation stage.

## Candidates requiring further verification

| Exact path / symbol | Origin and purpose | Present evidence / why it may be redundant | Remaining consumer question and removal risk |
| --- | --- | --- | --- |
| `src/common/Header.hpp`, `src/common/FixedStringQt.hpp` | Present in R0; an older local fixed-string helper pair. Historical intent beyond the source is unknown. These are separate from protected `src/fixed_str/`. | Neither is registered in application source lists. The only current include of `Header.hpp` is in this pair, and no application source includes the pair. Actual CV SQL uses `fixed_str/FixedStringQt.hpp`. | Repository-local absence does not prove no external or instructional consumer. Deletion could remove retained examples; verify purpose separately. The protected library is never a candidate. |
| `qml/pages/MemoryPage.qml`, `ProfilePage.qml`, `SettingsPage.qml`; `qml/components/SidebarButton.qml` | Present in R0; registered prototype resources. | No current main-shell route/instantiation was found; the Settings sidebar entry does not navigate to these pages. All remain in `JOBTRACKER_QML_FILES`. | QML registration and potential dynamic or external module loading are consumers that plain text call counts cannot dismiss. Removing resources changes module availability. Preserve. |
| `tests/deployment/__pycache__/Verify-Handoff.cpython-312.pyc` | Tracked before R0; generated CPython 3.12 bytecode beside `tests/deployment/Verify-Handoff.py`. | Generated/reproducible-looking output; the corresponding maintained Python source exists. No production target requires this cache. | The installer/deployment consumers were intentionally not executed or globally audited. Its tracked status and historical purpose need a separate decision. Preserve; do not expand this task into deployment cleanup. |
| `src/cvs/CvImportService.cpp:116–121`, disabled cancellation branch | Present in R0; retains an earlier cancellation-policy alternative near the persistence boundary. | Excluded from compilation; current preparation/import behavior deliberately retains the existing cancellation boundary. | Its historical rationale is relevant to behavior-preservation review. It is unrelated to R009's compensation extraction. Do not remove it under an assumption that the disabled policy should become active. |

## Required retained materials, not garbage

| Exact path / family | Origin and purpose | Consumer / preservation reason |
| --- | --- | --- |
| `src/fixed_str/` and `src/cvs/CvRepository.cpp` | Pre-existing production library and SQL consumer. | Seven library files, paths and raw bytes match R0; the live SQL authoring uses remain. Explicitly protected. |
| `qml/components/CvDocumentIcon.qml` | R007. | Two direct consumers in the CV browser/preview and a QML resource registration. Removing it breaks loading. |
| `tests/maintenance/ShellBaseline.cpp`, `StorageMutationFeedbackTest.hpp` and strengthened existing suites | Stages 2–4. | Registered only in tests-ON; exercise real bootstrap, storage/workers and QML through existing contracts. Required characterization coverage, not product hooks. |
| `CvLibraryController::categorySummary`, `CompanyDirectoryController::clearFilters` | Existing public contracts, internally simplified in R005/R004. | `Q_PROPERTY`/`Q_INVOKABLE` declarations and C++ consumers remain. Lack of a direct current QML text reference does not authorize removing an API. |
| `For-Agent/Refactor/BASELINE.md`, `STATE.md`, `PLAN.md`, `FINDINGS.md`, `reports/`, `evidence/baseline/`, prior numbered evidence | Refactoring audit and verification stages. | R0, lineage, decisions, failures and successful runs support stages 6–7 and prevent rewriting history. Similar reports refer to different executions, not duplicated authoritative architecture. |
| `For-Agent/Refactor/evidence/verify-r0.py` | Stage 1. | Strict immutable-R0 verifier; an intentional later source delta is expected to fail its direct checkout comparison. Still required for baseline reproduction. |
| `For-Agent/Refactor/evidence/05-verify-2026-11-04-0426.py` | Stage 5. | Verifies R0 checksums/archive, stage-4 overlay, membership, protected bytes, source identity and Git read-only state. Used for stage-5 input and final checks. |
| `For-Agent/Refactor/evidence/05-production-isolation-2026-11-04-0426.py` | Stage 5. | Reproducible Windows production prerequisite/runtime/PE-import inspection; writes only its evidence receipt. No production/test target references it. Its purpose is verification, not application execution. |
| `build/r0ui-0351/`, `build/r0-integration-2026-11-04-0412/` | Stages 3 and 4 control workspaces. | Retain exact R0 control and diagnostic fixture history referenced by prior reports, including T001 investigation. Normal application targets do not use them, but they remain useful for reproduction. No cleanup. |
| `build/qt-6.10.3/windows-debug/`, `windows-debug-tests/`, `windows-release/` | Pre-existing caches/runtimes, incrementally updated in stage 5. | Existing canonical build paths; preserving them is explicitly required. Stale files in ordinary development output are not treated as proof of current dependencies. |
| `build/qt-6.10.3/windows-release-tests/` | Created by the existing Release-tests preset in stage 5. | Required separate tests-ON tree for the explicit Release matrix. Not an unsolicited alternative build design. |
| `build/qt-6.10.3/windows-{debug,release}/dev-runtime-fresh/{Debug,Release}/` stage-5 outputs | Stage 5, using the existing opt-in target. | Fresh destinations prove production deployment inventory independently of stale files or test-runtime trees. Exact paths and executable hashes are recorded in the production-isolation receipts. |
| `For-Agent/Refactor/evidence/05-*` logs, Qt archives, frames, manifests and receipts | Stage 5. | Preserve the initial capture-setup failure separately from successful reruns, actual build/CTest commands, visual comparisons and final identity. These are requested evidence, not disposable logs to remove silently. |
| Existing `For-Agent/Docs/architecture/{jobs,cvs,removal,threading,qml-contracts}.md` | Maintained docs updated in stage 3. | Canonical domain responsibilities and contracts. Existing overview routes cover the moved templates, compensation owner and shared icon; a second new domain document would duplicate them. |
| Existing installer outputs, `.vs/`, historical review/human notes and task inputs | Present before R0. | Ownership/history retained. This task neither generated installer packages nor established that these materials are safe to delete. |

## Outcome

Two old source-level redundancies are established above. Other candidates retain explicit uncertainty; none is upgraded to safe deletion merely because no ordinary target currently uses it. No newly introduced unnecessary source entity was found. Stage-5 outputs serve the requested Debug/Release evidence and Linux handoff.


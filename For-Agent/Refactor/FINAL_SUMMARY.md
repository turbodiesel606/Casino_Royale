# Final refactoring summary
Created: 2026-11-04 14:05 local time (Asia/Baku)

## Outcome and current source

**Stage 7 is complete in the executed scope. The full plan is not complete: R008 remains deferred, and native/manual limits remain explicit.** Windows Debug and Release production, separate full tests-ON builds, 20/20 CTest each, and ordinary development runtimes passed. No new confirmed production regression or stage-7 source change was found. Independent review accepted the exact Linux qualifier and directly affected contracts.

Stage-7 input/output production digest: **`92bc1300d0bf16df245a3b908c313975b61e24a33d6f1c774328edfe6c36fbd1`**, covering 169 sorted production paths. All 350 final-Linux manifest entries and three transferred installer/harness extras match current bytes. Full manifest SHA-256 remains `a45472c0599d006029d1551750dd52bc202b8e50789446ab9f88c93dbf659377`. R0 is preserved, not replaced.

**Linux recheck required: no.** Stage-6 Linux and stage-7 Windows results apply to the same final source and test inputs. The earlier stage-5 Windows results used the prior header and remain historical evidence. [Final identity](evidence/07-windows-2026-11-04-1332/final-validation.json) · [Full manifest](evidence/07-windows-2026-11-04-1332/final-source.tsv) · [Immutable R0](BASELINE.md)

## Actual four-platform matrix

Windows environment: Windows 10 x64, MSVC 19.44/v143, SDK 10.0.26100.0, Qt 6.10.3, CMake/CTest 4.0.2. Reused Linux environment: Ubuntu 24.04.4 amd64, GCC 13.3, Qt 6.10.3, CMake/CTest 3.28.3, Ninja 1.11.1. Linux was not rerun on the Windows host.

| Platform / evidence | Production and separate full tests build | Full application CTest | Runtime and visual evidence | Preserved failure / not-run scope |
| --- | --- | --- | --- | --- |
| Windows Debug, [stage-7 log](evidence/07-windows-2026-11-04-1332/debug-ctest.log), current | passed, exit 0 | **20/20**, exit 0; 249 Qt PASS incl. lifecycle, 2 SKIP | Ordinary runtime/test isolation passed; 25/25 R0 frames equal; native bootstrap and pending-import Wait/Interrupt passed | Supplementary native Dashboard wheel **failed**; standalone production EXE, picker/editor hardware input and native removal-close not run |
| Windows Release, [stage-7 log](evidence/07-windows-2026-11-04-1332/release-ctest.log), current | passed, exit 0 | **20/20**, exit 0; 249 Qt PASS incl. lifecycle, 2 SKIP | Same; 25/25 R0 frames equal | Same native wheel **failure** and manual limits |
| Linux Debug, [stage-6 log](evidence/06-linux-2026-11-04-0552/debug-ctest-access.log), current by full manifest match | passed, exit 0 | **20/20**, exit 0; 251 Qt PASS incl. lifecycle, no SKIP | Ordinary runtime and isolated standalone XCB start/normal close passed; 25 frames equal to Linux Release; native minimum assertions passed | Editor-wheel zero-offset observation remains inadequate; picker/other DPI/Wayland/native removal-close not run |
| Linux Release, [stage-6 log](evidence/06-linux-2026-11-04-0552/release-ctest.log), current by full manifest match | passed; preserved interrupted production attempt followed by successful identical retry | **20/20**, exit 0; 251 Qt PASS incl. lifecycle, no SKIP | Ordinary runtime and isolated standalone XCB start/normal close passed; 25 frames equal to Linux Debug | Default native resize **failed**; separate explicit-geometry row passed, exact cause unresolved; editor/hardware limits remain |

The two Windows skips are open-log-directory replacement and filesystem link creation. Both rows ran on Linux. Each Windows native invocation has **6 PASS including lifecycle, 1 FAIL, exit 1**; these failures are not absorbed into the successful 20-suite CTest result. Both Windows windows reached 1180×720 at DPR 1.25 before failing the already R0-reproduced synthetic Dashboard wheel check. Real pending-import Wait, automatic dialog dismissal and Interrupt/drain passed separately within those invocations.

Both Windows configurations retain all 86 prior normalized QML warning signatures. Release matches 24/25 stage-5 frames but 25/25 R0 frames: the historical confirmation-button state variation now matches R0 exactly. No code was changed to obtain this. Native and offscreen renderers and different OS font rendering are not treated as pixel-equivalent.

[Windows report and full matrix](reports/07-windows-recheck-2026-11-04-1352.md) · [Windows command/exit ledger](evidence/07-windows-2026-11-04-1332/execution-ledger.md) · [Linux report](reports/06-linux-final-2026-11-04-0904.md)

## Plan and structural result

| ID | Final result | Preserved responsibility / contract |
| --- | --- | --- |
| R001 | Implemented: bulk-selection include cleanup | Stable IDs, proxy order and selection notifications |
| R002 | Implemented: private update template moved to .cpp | Validation modes, missing-row precedence, errors and worker contract |
| R003 | Implemented: private typed CV removal templates moved to .cpp | Typed outcomes, correlation, model/completion/admission order |
| R004 | Implemented: company clear uses existing search setter | No-op reset, selection reconciliation and sort |
| R005 | Implemented: one category-counting pass | Same map/list values, ordering and existing notification edge case |
| R006 | Implemented: direct private gate dispatch | Blocker priority, reservations, diagnostic facts, quiet shutdown and signals |
| R007 | Implemented: shared CV document icon | Two callers, original drawing/sizes/margins and PDF label |
| R008 | **Deferred; unchanged R0 scroll code** | Native minimum-editor wheel/focus/draft reference required before extraction |
| R009 | Implemented: one compensation/message policy in existing import service | Three distinct workflows, rollback-before-cleanup, held mutex, exact error suffix |
| F007 / L006-C1 | Authorized old GCC compilation fix retained | One standard `template` disambiguator; now verified with GCC and MSVC |

All nine requested directions were considered: source architecture, classes/components, functions, modern C++, interfaces/dependencies, threading, Qt/QML, data/resources and error handling. The [detailed direction matrix](reports/07-windows-recheck-2026-11-04-1352.md#structural-result-and-plan) records boundaries instead of claiming every possible refactor was performed.

Cumulative R0 differences are 18 production/build entries, five architecture documents, eleven test entries and one separately owned task typo. CMake changed only through three registrations. No build/preset/installer architecture was refactored. All seven `src/fixed_str/` files and existing SQL uses remain unchanged. Production graphs/runtime imports remain independent of Qt Test; no production API or hook was added for testing.

Schema v4, persisted formats, transaction/managed-file guarantees, errors, cancellation, significant Qt/model signals, logging semantics and existing async ordering remain the comparison contracts. Full suites cover Jobs/CV/storage/models, all four typed removal kinds, admission, workers, shutdown, real QML navigation/save and logging. The five affected architecture documents describe actual existing owners; stage 7 introduces no responsibility needing another document.

## Stage-7 files and remaining issues

Modified continuation indexes: `STATE.md`, `PLAN.md`, `FINDINGS.md`, `WINDOWS_RECHECK.md`. Created this summary, the [Windows report](reports/07-windows-recheck-2026-11-04-1352.md), [final leftovers report](reports/07-leftovers-final-2026-11-04-1339.md), and the `evidence/07-windows-2026-11-04-1332/` bundle (receipts, logs, images, manifests and reusable verification helpers). Ordinary generated Windows build/runtime outputs were updated. No production/test/QML/CMake/preset/installer source or immutable baseline was edited in this stage.

No confirmed open refactor regression is recorded. Old F001 partial-read recovery, F002 checked-CV counts, F003 log identity, F004 failed-deletion log totals, F005 CV error classification and F006 category-summary notification remain. S001-S004, T001's intermittent synthetic input and the historical T002 fixture fix retain their documented uncertainty/history. Existing style and confirmation binding-loop diagnostics remain. [Findings](FINDINGS.md)

Initial sandbox build regeneration and helper setup failures are preserved separately from product assertions. Physical disk exhaustion/short writes, partial-cursor recovery, worker creation/post failure, clean-host execution and installer acceptance were not established; installer/package/acceptance work was excluded.

## Exact continuation

1. Use a controlled disposable Windows account/profile for standalone Debug/Release production-EXE startup and normal shutdown; personal records must remain untouched.
2. Capture R0/current native input at 1180×720 and 1660×948: wheel over editor labels/fields/blank space, track/drag, focus and draft retention; check native Unicode file picker, relevant DPI/themes and close during active removal. R008 remains gated until its own reference is adequate.
3. Resume only the accepted remaining scope. If shared source, tests or registration change later, rerun existing prompt 6 on Linux, then prompt 7 on Windows. No eighth prompt is required.

## What remains unnecessary

Two old source redundancies, three empty Linux setup directories and four newly generated disposable SQLite fixture databases remain. Uncertain prototype/helper/deployment candidates are separated from proven redundancies; required baseline/history, protected library, current components and test evidence are retained. **Nothing was automatically cleaned up.** Exact paths, provenance, consumers and removal risks are in the [final leftovers report](reports/07-leftovers-final-2026-11-04-1339.md).


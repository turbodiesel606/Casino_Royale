# Stage 7 Windows recheck status and retained handoff
Created: 2026-11-04 07:29 local time (Asia/Baku)

## Current status

Stage 7 has been executed on real Windows against the exact stage-6 final source. Debug/Release production, separate full test builds, CTest 20/20 each, ordinary runtimes and test isolation passed. Each Windows run has 249 Qt PASS including lifecycle and two original skips; each configuration's 25 offscreen frames match R0. Native bootstrap and pending-import Wait/Interrupt passed, but both supplementary minimum-shell runs failed the prior R0-equivalent Dashboard wheel assertion. R008 and manual limits remain. No source/test/registration change was needed; Linux evidence remains current. **Linux recheck required: no.** [Final summary](FINAL_SUMMARY.md) · [Stage-7 report](reports/07-windows-recheck-2026-11-04-1352.md) · [Final Windows manifest](evidence/07-windows-2026-11-04-1332/final-source.tsv)

The preserved stage-6 evidence has 20/20 Linux CTest in each configuration, 251 Qt PASS and no skips, plus isolated standalone startup/normal close. Its default native Release resize failure and separate explicit-geometry pass remain distinct. Full 350-entry manifest SHA-256 is still `a45472c0599d006029d1551750dd52bc202b8e50789446ab9f88c93dbf659377`. The original handoff below remains the checklist/source contract for any future recheck; it is not a new pending stage-7 instruction.

## Change after stage 5

| Path / symbol | Exact edit and origin | Windows contract |
| --- | --- | --- |
| `src/maintenance/DataRemovalExecutor.hpp:54`, `DataRemovalExecutor::process<Kind>` | Add the standard `template` disambiguator: `ensureService(...).template process<Kind>(...)`. GCC 13.3 rejected the original expression as `operator<`; the same error was reproduced in the immutable R0 source copy. The user explicitly authorized this minimal old compilation fix and any further necessary portable compilation fixes. | Preserve all four `DataRemovalKind` specializations, request/outcome types, arguments, operation/token correlation, worker-thread SQL ownership, exception mapping, lifetime and business outcomes. The qualifier changes parsing, not the intended call. |

No production/QML API, runtime behavior, source registration, build flags, preset, installer or protected fixed-string library changed. All other bytes in the edited header, including CRLF, are preserved. Architecture responsibilities are unchanged; no architecture-document edit is required. [Exact patch](evidence/06-linux-2026-11-04-0552/source-fix.patch) / [authorization and hashes](evidence/06-linux-2026-11-04-0552/authorized-source-delta.json)

Stage-5 header SHA-256: `812bc363fe142e94af0f32fd8da857ae0d64ac1d55ef91169f3136f0c986de43`.
Stage-6 header SHA-256: `4155362dcfd684d3e9c1768fc442bd54d1aea2f3af3d48517316336b8c41c80f`.
Current production digest: `92bc1300d0bf16df245a3b908c313975b61e24a33d6f1c774328edfe6c36fbd1` over the same 169 sorted production paths.

The Linux checkout's Git HEAD differs from the Windows baseline. All 350 stage-5 manifest entries matched at entry; three extra old tracked Linux-checkout installer/harness files are documented separately and preserved. Use the final manifest and authorized source delta, not the broad local Git diff, to validate the transferred source. Preserve R0 and previous evidence.

## Checklist used for real Windows verification

1. Verify the actual current Windows Qt 6.10.3/MSVC kit and source identity. Use existing supported Windows Debug/Release presets with separate production tests-OFF and tests-ON trees; preserve local settings and caches.
2. Build production and the full registered application/unit/integration/QML test targets, then run CTest separately for both configurations. In particular retain Removal, Jobs, Cv, Integration and MutationFeedback coverage of all typed deletion/archive/restore outcomes, stale completions, reservations, rollback, cancellation, publication and shutdown. Do not add hooks or alter expected outcomes for the qualifier.
3. Prepare the ordinary production development runtimes and check startup/QML loading and normal close only with disposable data. Observe the native picker, relevant DPI/theme and hardware input. Exercise Wait/Interrupt/close with synthetic import/removal work, preserving worker cleanup and existing outcome rules.
4. Retain R008 as deferred until its actual native editor wheel/focus reference is adequate. The Linux minimum XCB fixture passed its assertions, but its focused editor-wheel observation had zero offset; that observation is not asserted as success by the existing fixture. Passing Linux input checks do not supply Windows evidence.

## Remaining boundaries

Native MSVC verification was **NOT_RUN_ON_UBUNTU** at the stage-6 handoff and is now covered by current Windows Debug/Release builds and full tests. Prior stage-5 Windows passes still refer to the pre-edit header. Stage-7 execution, not a syntax opinion, closes the native compiler requirement.

F001-F006 and S001-S004 retain their old defect/uncertainty classifications. Existing confirmation binding-loop warnings remain. The Linux sandbox initially blocked UUID test-profile writes and display access; supported access-enabled executions passed without source workarounds. The native startup helper checks an isolated empty profile and idle shutdown, not a native close during active removal. Hardware editor wheel/focus, native picker, other DPI/themes and unexercised fault-injection cases remain explicit limits. No package, installer or deployment acceptance harness is part of this handoff.

The original Linux native Release resize failure remains a verification limit: actual 1854x1011 versus requested 1180x720. Its separate `-qwindowgeometry 1180x720` pass did not isolate exact causation. Stage-7 Windows default minimum sizing passed in both configurations at DPR 1.25; each then failed Dashboard synthetic wheel movement before reaching the editor. Hardware editor reference, native picker, standalone Windows EXE under a disposable profile and native active-removal close remain manual. If shared source/tests/registration change later, execute existing prompt 6 on Linux, then prompt 7 on Windows; no eighth prompt is needed.

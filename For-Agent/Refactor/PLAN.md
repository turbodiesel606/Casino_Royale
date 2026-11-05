# Behavior-preserving refactoring plan

## Execution order and gates

Stage 7 Windows recheck is complete in the executed scope with retained native/manual limits. Windows Debug/Release production, separate full test builds, 20/20 CTest each and ordinary runtimes passed on the exact final Linux source. No stage-7 source change was required; Linux evidence remains current. R001-R007 and R009 are implemented; R008 retains its native-editor gate and is not complete. No confirmed open production regression is recorded. [Final summary](FINAL_SUMMARY.md) · [Windows report](reports/07-windows-recheck-2026-11-04-1352.md) · [Linux report](reports/06-linux-final-2026-11-04-0904.md)

Stage 3 status and executed comparisons are recorded below and in the [implementation journal](reports/03-implementation-2026-11-04-0330.md). Stage 2 recorded all 20 original Windows Debug application suites, then strengthened the tests on the same production bytes. Each implemented ID was compared with R0 and recorded in STATE. A failed or unavailable gate holds its dependent scope. See the [stage-1 audit](reports/01-audit-2026-11-04-0237.md), [stage-2 execution](reports/02-baseline-2026-11-04-0256.md) and [R0](BASELINE.md).

| ID | Order / status | Why here | CMake allowance during implementation |
| --- | --- | --- | --- |
| R001 | Complete | Remove a dependency from a common model helper on two product domains. | None. |
| R002 | Complete | Reduce the implementation exposed to every job-worker consumer. | None. |
| R003 | Complete | Hide CV removal implementation while preserving its typed contract. | None. |
| R004 | Complete | Keep company search reset in its existing owner. | None. |
| R005 | Complete | Aggregate the CV category projection in one pass. | None. |
| R006 | Complete | Simplify the GUI admission policy without changing reservations or logging. | None. |
| R007 | Complete | Unify two identical drawings of a CV document. | One QML entry in `JOBTRACKER_QML_FILES`. |
| R008 | Deferred · native editor reference unavailable | Give one owner to a three-screen scroll algorithm. | One QML entry in `JOBTRACKER_QML_FILES`. |
| R009 | Complete | Three production paths maintain the same completed-file failure policy. | None. |

The approaches use the existing C++20/Qt 6.10.3 project. New test support, if a case is actually missing, lives inside `tests/`. Prefer an existing suite and registration. Choose additional source registration only when a new test translation unit is necessary. No production API, compile definition or access change is justified by a test. The original controls, diagnostics, persisted bytes, transactions, cancellation, worker affinity and guaranteed notifications remain comparison criteria.

## R001 — Decouple bulk selection from CV and removal types

**Scope and benefit.** `src/common/BulkIdSelectionTracker.cpp:1–16,34–47,137–172` includes `CvListModel`, `DataRemovalTypes`, a concrete filter and its own header twice. The public header also includes itself at `src/common/BulkIdSelectionTracker.hpp:4–9`. The implementation only needs a `QAbstractProxyModel`, an integer role, common role access and Qt/standard containers. Remove the unused domain/concrete includes and redundant self-includes. Supply direct Qt includes needed for the actual symbols. Reused by jobs and CVs. This covers source architecture, interfaces/dependencies. Applicable guidance includes BP03 explicit dependencies and minimal interfaces.

**Contract and risk.** Low. Keep declarations, role, proxy-order enumeration, selection after filtering/sorting, notification counts and previously disabled code outside this include edit. No dependence on any other plan ID.

**Verify and finish.** Compile the app and the separately registered selection target(s). Run `JobTrackerSelectionTrackerTests`, especially `bulkSelectionUsesVisibleStableIdsAndSurvivesSorting`, plus the checked-selection cases in `JobTrackerJobsTests` and `JobTrackerCvTests`. Complete when this source needs neither product-domain header, all active logic is unchanged and both domains still expose the same selected IDs/signals.

## R002 — Move the private job-update preparation template to its implementation

**Scope and benefit.** `UpdateJobService::prepareImpl` in `src/jobs/UpdateJobService.hpp:86–176` exports SQL lookup, exception/log handling and CV preparation through a service header also included by `src/jobs/JobSaveWorkerTypes.hpp:6`. Both instantiations occur in `src/jobs/UpdateJobService.cpp:37–61`. Keep the constrained-by-mode design and `if constexpr`. Define the body and its associated rationale in the existing .cpp, retain its private declaration and include only required value types in the header. Fix direct includes in consumers if needed. This covers source organization, functions, modern C++, interface dependencies and error boundaries. Applicable guidance includes project coding style, Header Conventions, BP02/03/04.

**Contract and risk.** Low behavioral risk, moderate include fan-out. The two public entry points, mode selection, thread/token guards, operation IDs, missing-record precedence, checked versus prevalidated behavior, exact errors and throwing/logging paths stay intact. Preserve replacement-byte preparation. This is independent of R001.

**Verify and finish.** Build Debug production and test consumers. Run `JobTrackerUpdateJobTests`, `JobTrackerJobsTests` and `JobTrackerIntegrationTests`. Pin the missing row plus invalid input case, no-replacement/replacement cases, defensive validation and the controller's prevalidated route. Complete when the template has one private definition visible to both instantiations, and failures, records, signals and logs match R0.

## R003 — Localize the CV controller's typed removal handlers

**Scope and benefit.** `submitCvMutation<Kind>` and `handleRemovalCompleted<Kind>` are private but their full bodies occupy `src/cvs/CvLibraryController.hpp:219–336`, exposing logger, gate and worker implementation to all consumers. Only the constructor's typed signal handlers and the three commands in `src/cvs/CvLibraryController.cpp:140–154,650–703` use them. Move their unchanged definitions into that .cpp, preserving the constraints on their declarations. Remove the self-include and move only implementation-only includes. Keep complete types required for member storage or moc. This covers classes, source organization, modern C++, interfaces, Qt/QML. Applicable guidance includes project private-template rule, BP03 encapsulation, BP08 typed Qt boundaries.

**Contract and risk.** Moderate. Preserve concrete outcome types/signals, acceptance before dispatch, operation ID/token correlation, rejection of stale completions, exact counts/text, model and selection publication before `cvMutationCompleted`. `releaseCvMutation` publishes mutating/pending/availability in sequence before ending removal (`src/cvs/CvLibraryController.cpp:862–871`). Keep shutdown behavior and old F002 separate. No prerequisite beyond a typed R0 reference.

**Verify and finish.** `JobTrackerCvTests` cases `permanentDeletionRetainsSkippedLinkedSelection` and `typedRemovalActionsIgnoreStaleCompletions`, plus `JobTrackerRemovalTests`, `JobTrackerIntegrationTests` and `JobTrackerMutationFeedbackTests`. Observe committed, skipped, failed and cancelled items using disposable fixtures. Complete when header consumers compile, signatures/metaobject expose the same types and the observer sees the R0 sequence.

## R004 — Route company search reset through the existing setter

**Scope and benefit.** `CompanyDirectoryController::clearFilters` duplicates `setSearchText`'s guard, proxy update bracket and notification in `src/directory/CompanyDirectoryController.cpp:143–154,176–186`. Delegate the company-only reset to `setSearchText(QString{})`. This covers functions and Qt model interfaces. Applicable guidance includes BP03 DRY, same owner and same reason to change.

**Contract and risk.** Low, independent. Empty and already-cleared searches remain a no-op. On change the tracker finishes reconciliation before `searchTextChanged`. Sort mode, linked jobs/contacts and selected ID follow the same rules. The contact reset batches three edits and the jobs reset also touches bulk state. Leave both alone.

**Verify and finish.** Run `JobTrackerDirectoryTests`, including `scalarNotificationsAreSemantic` and `companySelectionPublishesLinkedViewContracts`. Capture nonempty → clear → clear, selected ID and linked rows at the notify signal. Complete when there is one implementation of company search resetting and no new signal or sort change. QML continues to call the existing command.

## R005 — Compute the category summary in one model pass

**Scope and benefit.** `CvLibraryController::categorySummary` in `src/cvs/CvLibraryController.cpp:227–263` walks the same concrete model twice with the same active/archive predicate. Count total rows and increment the existing `QMap` buckets in one pass and then emit the same list of maps. Keep this a computed property without caching. This covers functions, C++ container use, Qt data contract. Applicable guidance includes BP03 one rule per owner and BP06 direct algorithms. This removes a repeated traversal. No timing improvement has been measured. The exposed property has no direct reader in the current QML tree.

**Contract and risk.** Low and independent of the removal path, though R003 edits the same translation unit. Keep the first `All CVs` row even for an empty view, the same `title/count/selected` keys and types, `QMap` iteration order, duplicate/empty category aggregation and selected flags. Count from the source model, independent of language/search filters. Do not alter any emitting site.

**Verify and finish.** `JobTrackerCvTests` checks filter and summary notifications. Compare the complete `QVariantList` before/after with empty, repeated/empty categories and mixed active/archived rows. Cover the empty and `All` filter and semantic no-op updates. Complete when values and notifications match with a single model scan.

## R006 — Simplify the gate's private category dispatch

**Scope and benefit.** `StorageMutationGate` converts known template categories into a runtime category and back into private template specializations. Its private header carries policy and logging-sensitive signal publication. Replace this round trip with one ordinary private `blockingReason(category)`, `checkAllowed(category, action)` and category-indexed retry reset in the existing .cpp. Migrate the active implementation, including event recording. The disabled alternative in the .cpp does not contain the same active behavior. `src/maintenance/StorageMutationGate.hpp:77–142`, `src/maintenance/StorageMutationGate.cpp:14–75,92–145,152–204`. This covers classes/functions, modern C++, concurrency/admission, Qt and error reporting. Applicable guidance includes BP03 KISS, BP05 protect the whole invariant, BP06 use features for clarity.

**Contract and risk.** Moderate. Keep GUI-thread ownership, public enums/invokables/properties, two categories, zero-count reservation semantics, counter overflow/underflow diagnostics and all texts. Priority is closing, then active removal, then pending import/update for a removal request. Each ordinary conflicting attempt records the appropriate action and signals `feedbackChanged` before `operationBlocked`. State transition publication emits `stateChanged` before `feedbackChanged`. Shutdown declines attempts quietly. Retry interest never queues work. Do not change the lock or reservations at callers.

**Verify and finish.** `JobTrackerMutationGateTests`, `JobTrackerMutationFeedbackTests`, `JobTrackerIntegrationTests`. Record signal order as observed by listeners, repeated dialog reopening, dismissal, quiet shutdown and exclusivity through model publication. Complete when every supported category/reason has the R0 result, signals/log facts and visible message. Resolve R003 header changes separately so failures are attributable to one ID.

## R007 — Share the CV document icon between the browser and preview

**Scope and benefit.** The two inline `PdfIcon` definitions in `qml/pages/CvLibraryBrowserPane.qml:321–356` and `qml/pages/CvLibraryPreviewPanel.qml:241–276` have the same rectangles, geometry, colors and label. Introduce one presentation-only `qml/components/CvDocumentIcon.qml` and replace those two inline definitions/uses. No new backend contract. This covers UI structure, reusable components and QML resources. Applicable guidance includes qml-style Components and BP03 reuse. Independent of backend changes.

**Contract and risk.** Low structural risk with a visual gate. Keep the browser's 64×89 and preview's 72×91 preferred sizes, layout margins, the CV page’s 0.80 scale and literal PDF label even for supported non-PDF input. The dashboard has a different drawing. Register only the one resource in the existing list.

**Verify and finish.** Baseline and compare the two surfaces for a selected item in a populated disposable profile, filtered/empty state and Active/Archived. Use the default 1660×948 and minimum 1180×720 sizes, same DPI/theme. Run `JobTrackerMutationFeedbackTests` as a module/startup check. It does not assert icon pixels. Complete when both consumers render the same geometry and colors and module loading emits no new warnings.

## R008 — Share the vertical track and drag behavior

**Scope and benefit.** `qml/pages/DashboardPage.qml:105–162`, `qml/pages/JobFormPage.qml:412–476` and `qml/pages/JobDescriptionPane.qml:730–815` implement the same click-to-center, clamping and thumb drag in track coordinates. Extract the track/thumb into one QML component with an explicit Flickable input. Keep each Flickable, its external anchors, local width, separator, margin/reserved area and page state with the caller. This covers functions, UI component boundaries and Qt input/geometry. Applicable guidance includes qml-style Components, BP03 DRY and BP08 UI ownership.

**Contract and risk.** Moderate visual/input risk. Keep overflow-only visibility, the 42 px minimum thumb bound, width minus 3, colors, cursor, `visibleArea` height ratio, top/bottom endpoints, coordinate mapping and wheel behavior. Do not change focus, editor scroll retention, separators or the page's hidden width reservation. Independent of R007 but apply and compare one new resource at a time.

**Verify and finish.** Use `JobTrackerMutationFeedbackTests` for startup, then compare overflowing and short content on all three views at both window sizes. Exercise track clicks above/below the thumb, drag to each end, wheel scroll, focus and draft preservation. Complete when reference recordings/screenshots and event results agree. If the GUI reference cannot be taken safely, leave R008 waiting.

## R009 — Centralize completed-file compensation after a failed CV write

**Scope and benefit.** Add, update and standalone import call the same file-store operation and append the same quarantine suffix independently (`src/jobs/AddJobService.cpp:155–166`, `src/jobs/UpdateJobService.cpp:155–166`, `src/cvs/CvImportExecutor.cpp:157–166`). Give that single cleanup/message operation to the existing `CvImportService` which currently just forwards to `CvManagedFileStore::removeCompletedFile` (`src/cvs/CvImportService.cpp:167–170`, `src/cvs/CvManagedFileStore.cpp:335–342`). The policy takes the completed path, updates the cleanup fact and appends to the caller's existing failure message only if the file store reports failure. Recheck all callers before changing an internal signature. Keep the existing file store responsible for filesystem I/O. This covers functions/classes, interface ownership, threads, data/resources and error handling. Applicable guidance includes BP01 lifetime, BP03 shared rule, BP04 preserve error translation, BP05 lock invariant.

**Contract and risk.** High. R002 touches the same service consumer file and should finish first. Leave base exception messages, operation IDs, log guards, null-preparation guards and cancellation boundaries at their original owners. Preserve the exact leading-space suffix and behavior for empty/missing paths, for which the file store leaves the prior cleanup fact alone. Transactions have unwound before compensation. Keep the shared lock through that operation. The helper must not start a transaction, acquire/release a lock, aggregate errors or alter recovery.

**Required gate and finish.** Run `JobTrackerAddJobTests`, `JobTrackerUpdateJobTests`, `JobTrackerCvTests`, `JobTrackerCvImportTests` and `JobTrackerIntegrationTests`. In disposable fixtures compare SQL rows after rollback, final/staged filenames/bytes, visible error and sanitized facts for no file, successful removal and failed removal. Existing cases include `removesFinalizedCvWhenCvInsertFails`, `rollsBackDatabaseAndFileOnUpdateFailure` and `failedCvInsertRollsBackAndContinuesFifo`. A nonempty directory used as the completed path in a temporary file-store fixture is one way to test a failed removal and its cleanup fact. Separately check that a participating caller appends the exact suffix after rollback. If the combined failure cannot be observed through a current contract, keep this item blocked. Complete only when all three routes retain their distinct results and logs while sharing exactly one cleanup-message policy. Never provoke partial database reads in a real profile.

## Decision history

2026-11-04, stage 1. IDs assigned against immutable R0. No candidates were implemented, dropped for a failing test, or granted dynamic readiness. Keep subsequent status changes with their date, evidence and reason. The report records investigated alternatives, including the worker executor pointer, database/recovery consolidation, unreachable QML pages and existing diagnostics. [Findings and deferred boundaries](FINDINGS.md) · [State](STATE.md)

2026-11-04, stage 2. R001–R007 and R009 have sufficient Windows Debug protection for their scoped changes. R009 follows R002 and must retain the distinction between the three callers and the existing failure/log paths. R007 requires comparison with both icon surfaces and the filtered state using the same renderer. R008 has track/drag/short-content references; capture native wheel/focus behavior at 1180×720 before changing it. Start stage 3 with R001. [Executed contracts](reports/02-baseline-2026-11-04-0256.md#contracts-and-per-id-evidence) · [Visual gate](evidence/02-visual-2026-11-04-0323.md#native-comparison-gate)

R001 · Both bulk-selection files differ from R0 only in include lines and whitespace. The tests-OFF app and all three selected targets built. SelectionTracker, Jobs and Cv passed 3/3, including proxy ordering, notifications and checked IDs. [Executed commands](reports/03-implementation-2026-11-04-0330.md)

R002 · Moved prepareImpl and its rationale into UpdateJobService.cpp. Comparison to the snapshot matched the full definition after dedenting and class qualification. The private header no longer includes the SQL/query, exception, validator, thread and repository implementations. UpdateJob, Jobs and Integration passed 3/3. An independent static reviewer found no regression in guards, exception/logging paths, private visibility, mode or ownership. [Executed commands](reports/03-implementation-2026-11-04-0330.md)

R008 · Deferred. A second native Windows attempt with the stage-2 Feedback executable, software renderer and synthetic profile failed to expose the minimum-size window before input. All three scroll implementations remain at R0. Reopen only after recording wheel position/offset and retained focus/draft on an interactive desktop. [Native Qt log](evidence/03-native-2026-11-04-0338/qt-test.txt)

R003 · Moved both constrained CV removal handlers to CvLibraryController.cpp. Their text matches R0 after class qualification and dedenting. The typed connections, public metaobject declarations, token/ID checks, GUI model publication, completion and gate release order are unchanged. Cv, Removal, Integration and MutationFeedback passed 4/4. Independent static review found no regression, including F002 and shutdown handling. [Executed commands](reports/03-implementation-2026-11-04-0330.md)

R004 · Company clearFilters delegates to setSearchText(QString{}). The existing setter still reconciles the selection before searchTextChanged, skips a second empty reset and leaves the sort untouched. Directory passed 1/1, including semantic notifications, linked views and the observed S004 ordering. [Executed commands](reports/03-implementation-2026-11-04-0330.md)

R005 · The source-model loop now increments the total and the QMap bucket together, retaining the All CVs row and ordered category rows. No cache or notifying site changed. Cv passed 1/1, including mixed views, empty/repeated category keys and the F006 empty/All transition. [Executed commands](reports/03-implementation-2026-11-04-0330.md)

R006 · Collapsed the two private template specializations and runtime switch to one .cpp policy for the two actual categories. Retained blocked-attempt logging, counter guards, retry interest, zero reservations, silent close and feedback ordering. MutationGate, MutationFeedback and Integration passed 3/3. An independent static reviewer verified the active R0 logging and gate contracts. T001 reproduced on byte-verified R0 and the unchanged R006 Feedback executable passed three consecutive trials. The 3/3 acceptance run used the same image-capture mode as the stage-2 comparison. [Executed commands](reports/03-implementation-2026-11-04-0330.md)

R007 · The browser cards and preview now share CvDocumentIcon, keeping the two layout sizes, margins, scale and literal PDF label. One QML registration was added. MutationFeedback passed 1/1. All 25 PNGs matched R0 byte for byte, including eight CV frames at both sizes. The 25 normalized warning signatures matched the baseline; visual inspection confirmed the populated card/preview and empty preview. [Executed commands](reports/03-implementation-2026-11-04-0330.md)

R009 · CvImportService now owns the completed-file failure message and cleanup-fact policy for Add Job, Update Job and standalone import. The file store still performs the removal. Each workflow retains its own base error, log guard, transaction, lock, cancellation and publication. The independent reviewer found the same outcomes for empty, missing, removable and obstructed paths. Full CTest passed 20/20 with all five required suites, including rollback, exact quarantine suffix, preserved files, and sanitized logging. [Executed commands](reports/03-implementation-2026-11-04-0330.md)

Stage 3 closed with eight completed IDs and R008 deferred. See the [output manifest](evidence/03-source-2026-11-04-0402.tsv) and the [stage-4 handoff](STATE.md). R008 may resume after a native editor reference is available.

## Stage 4 integration closure

R002/R009 now also have public-controller coverage of a failed new-file CV replacement. The SQLite failure must preserve the original model, relationship, managed filename and bytes while suppressing company/CV publication. The prior metadata-only case remains a separate data row. A real QML editor Save is followed through the visible Dashboard title and an independent SQLite read. Both strengthened suites passed on R0 and the current tree. The other IDs passed their cross-domain matrix checks and independent boundary review. [Command ledger](evidence/04-execution-2026-11-04-0419.md)

The full current suite passed 20/20 with 249 Qt PASS entries including lifecycle and two known filesystem skips. Final production development runtime preparation passed. Source ownership distinguishes the two test edits from a concurrent one-letter edit in the stage-3 task prompt. The [output manifest](evidence/04-output-source-2026-11-04-0416.tsv) preserves the current source identity for stage 5. Native R008 input, the native picker/active-removal process close, Linux and Release remain in their recorded verification scopes.

## Stage 5 Windows closure

R001–R007 and R009 passed cumulative independent static review and both full Windows Debug/Release application CTest selections (20/20 each). Separate tests-OFF production builds and fresh/ordinary development runtimes passed; generated prerequisite graphs and fresh runtime/PE inventories remain independent of Qt Test. Production retains the stage-3/4 digest. [Final Windows report](reports/05-windows-final-2026-11-04-0437.md)

The only new source edit is the ShellBaseline fixture's panel-render/focus synchronization. Initial Release failed all three editor-focus rows on both current and R0 production. The fixed test passed R0 Release, then final Debug and Release suites without changing expected outcomes, subsequent assertions or production. The [fixture overlay](evidence/05-test-overlay-2026-11-04-0448.zip) is part of the exact handoff. This is verification support, not a new product refactor ID.

R008 remains deferred. Native startup/exit passed in both configurations, but minimum-shell synthetic Dashboard wheel input failed identically on R0 before editor interaction. Debug frames match 25/25 R0 Debug references; Release matches 24/25 R0 Release references with the remaining pixels confined to an unchanged button region also varying on R0. No production cleanup or artifact deletion was performed. Continue with stage 6 on Linux and preserve the remaining native, filesystem/fault-injection and old-finding boundaries.


## Stage 6 Linux closure

| ID | Status / contract | Evidence and remaining risk |
| --- | --- | --- |
| L006-C1 (F007) | Complete. Add only the standard dependent-template disambiguator in DataRemovalExecutor.hpp:54; preserve all four typed removal kinds, arguments, exceptions, worker affinity, lifetime and business outcomes. | Original failure reproduced on byte-verified R0; one-line independent review accepted. Both production configurations and all 20 application suites passed. Native MSVC remains stage 7. |
| L006-V1 | Complete with recorded limits. Separate Debug/Release production/tests/runtime matrix; exact source identity, private runtime and production/test isolation. | 20/20 CTest each; 251 Qt PASS including lifecycle, no FAIL/SKIP. Both standalone XCB runtimes passed. Original native Release resize failure retained; explicit-geometry minimum-shell pass recorded without a single-factor causal claim. |
| R008 | Still deferred; not reopened by stage 6. | Native editor wheel logged zero offset; hardware Windows reference remains inadequate. No scroll source changed. |

R001-R007 and R009 retain their implemented scope. No additional compilation correction was required after the user's broader minimal-fix authorization. No production/test API or expected outcome was changed for testing. Existing architecture responsibilities/contracts remain accurate and need no documentation edit for the qualifier. All original failures, baselines, caches and unrelated inputs are preserved. [Final report](reports/06-linux-final-2026-11-04-0904.md) · [Final source](evidence/06-linux-2026-11-04-0552/final-source.tsv) · [Next: real Windows recheck](WINDOWS_RECHECK.md)

## Stage 7 Windows recheck closure

| ID | Final status / contract | Executed evidence and limitation |
| --- | --- | --- |
| W007-V1 | Complete in executed scope: validate final Linux source with native MSVC/C++20, preserve Qt/ownership/removal contracts and production/test isolation. | Debug/Release tests-OFF app, separate full tests-ON build, CTest 20/20 each and ordinary runtimes passed. All 350 source/boundary entries and three extras match stage 6; no stage-7 source change. |
| W007-V2 | Complete with native/manual limits: cumulative Jobs/CV/storage/model/QML/async scenarios and visual reference checks. | Both configurations: 249 Qt PASS, two old skips; 25/25 R0 frames equal; native bootstrap and pending-import Wait/Interrupt passed. Supplementary native minimum shell failed Dashboard synthetic wheel in both configurations, matching the prior R0 symptom. Standalone Windows EXE, picker/hardware editor input and native removal-close not run. |
| W007-R1 | Complete: independent critical review, nine-direction scope check, architecture impact, current four-platform matrix and leftovers inventory. | No actionable source regression; no architectural edit needed. CMake still has only three mechanical registrations, fixed_str and its uses retained. Reports/indexes updated, no cleanup. |
| R008 | Deferred, not reopened. | Actual minimum native editor wheel/focus/draft baseline is still inadequate. All three scroll sources remain R0-identical. |

Linux recheck required: **no**, because stage 7 changes only reports/evidence. Both OS execution records apply to production digest `92bc1300d0bf16df245a3b908c313975b61e24a33d6f1c774328edfe6c36fbd1`. The next point is the controlled native reference/manual scope, not a new architectural pass. If shared source/tests/registration are later changed, repeat prompt 6 on Linux and prompt 7 on Windows. [Final report](reports/07-windows-recheck-2026-11-04-1352.md) · [Final leftovers](reports/07-leftovers-final-2026-11-04-1339.md) · [Command ledger](evidence/07-windows-2026-11-04-1332/execution-ledger.md)

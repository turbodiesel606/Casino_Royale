# Remaining Windows Release Acceptance Work

Created: 2026-10-15 15:02 local time (Asia/Baku, UTC+04:00).

No work below was implemented by this audit. `src/` remains read-only. IDs are referenced by COVERAGE.md. R01–R20 are this audit's work IDs; the historical R1–R7 hold in REDISTRIBUTION.md is a separate identifier set. P1 means mandatory functionality or a release blocker; P2 means another mandatory gap. No new proven P0 data-loss/security defect is asserted from the available evidence. A check that has not run is not a demonstrated implementation defect.

Recommended dependency order: owner decisions on requirements/licensing and disposable environments → repair/resolve packaging and privacy/provenance obligations → freeze a new candidate and its exact manifests → native and clean-system acceptance → separately authorized archive publication → matching Setup and actual public clean-system acceptance → separately authorized Setup publication. R18–R20 can proceed independently when their environments/authorization exist.

## Implementation / redistribution prerequisites

### R01 — P1

- **Checklist:** T14-02/04; GLOBAL-20.
- **Finding:** Three actually bundled Qt 6.10.3 Lottie binaries have commercial/GPLv3-only alternatives; the intended proprietary/LGPL route is not established.
- **Required action:** Owner chooses a valid licensing route. For an exclusion route, remove the unnecessary deployment closure, adjust the reviewed allowlist, rebuild and check every QML style/import used by the app.
- **Expected files:** CMakeLists.txt; packaging/windows/payload-files.txt; deployment tests; notices/provenance; regenerated payload/Setup.
- **Acceptance check:** No unapproved Lottie binary remains in the exact shipped payload, or documented appropriate entitlement covers it; clean QML/graphics checks pass.
- **Dependencies:** Owner route decision; complete before final notices/source inventory and final VM acceptance.
- **Responsible party:** Owner/legal and packaging maintainer.
- **src/ required: NO.** Deployment selection/licensing work; no direct application Lottie use was found.

### R02 — P1

- **Checklist:** T06-03; T08-02; T14-06; GLOBAL-20.
- **Finding:** Current archive EXE embeds 28 raw QML resources; Main.qml was recovered in memory and hash-matched.
- **Required action:** Resolve the strict private-source policy explicitly. If it remains required, implement a reviewed resource policy and rebuild; verify absence of recoverable raw QML, startup, imports and compatible Qt replacement. Do not equate one build flag with acceptance.
- **Expected files:** CMakeLists.txt QML resource options; packaging audit checks; potentially src/app/AppBootstrap.cpp:151 startup path; new artifacts/docs.
- **Acceptance check:** Approved privacy policy is met by actual artifact inspection and runtime tests; Qt replacement/source obligations remain satisfied.
- **Dependencies:** Owner policy decision; coordinate R04; precedes final artifacts and R16.
- **Responsible party:** Owner and release engineer.
- **src/ required: UNKNOWN.** A CMake/resource solution may suffice, but cache/startup behavior may require work near src/app/AppBootstrap.cpp. Any src/ change requires a separate explicit owner decision; this audit authorizes none.

### R03 — P1

- **Checklist:** T14-01/02/03; GLOBAL-20.
- **Finding:** ICU is a byte-identical Windows System32 copy; exact Mesa/LLVM, D3Dcompiler and embedded Inno/reduced7z provenance/terms remain incomplete.
- **Required action:** Resolve the exact ICU redistribution grant or adopt an approved dependency strategy. Complete a version-specific source/license/component dossier for the other shipped binaries and embedded installer engine; distinguish build tools from shipped components.
- **Expected files:** Deployment configuration; packaging/windows/REDISTRIBUTION.md and inventory; corresponding legal/provenance files; package allowlist if payload changes.
- **Acceptance check:** Every final shipped binary/component has a supported provenance/license route; no unexplained system DLL copy; required materials feed R05 and clean dependency checks pass.
- **Dependencies:** Owner/legal evidence; R01/R02 final dependency choices; before R05 and final candidate testing.
- **Responsible party:** Release engineer and owner/legal.
- **src/ required: NO.** Expected work concerns packaging/vendor inputs and provenance. No source change is authorized.

### R04 — P1

- **Checklist:** T14-02/03/05; GLOBAL-20.
- **Finding:** No established distributor-controlled corresponding-source fulfillment and no modified-compatible-Qt replacement result; SBOM/source correspondence requires resolution.
- **Required action:** Preserve exact Qt sources/changes/build configuration and fulfillment arrangements; reconcile binary/SBOM and translation differences. Document and demonstrate compatible library replacement in disposable systems for both installed scopes.
- **Expected files:** Legal/source fulfillment materials; build-provenance records; deployment tests and new evidence; release guides.
- **Acceptance check:** Exact final Qt binaries correspond to the fulfilled sources and documented rights; modified compatible libraries can be used as required by the selected route.
- **Dependencies:** R01/R02/R03 final payload choices; owner/legal source-policy review; disposable environment R11.
- **Responsible party:** Owner/legal and release engineer.
- **src/ required: NO.** Materials and verification can be prepared outside src/. Any application defect found by replacement testing is a new separately authorized task.

### R05 — P1

- **Checklist:** T14-03/05/AC; GLOBAL-20/42.
- **Finding:** Common payload contains no Qt legal texts/notices/source/replacement instructions; Setup adds only MSVC-NOTICE.txt. RC5 distinguishes audit completion from delivery.
- **Required action:** Create reviewed version-specific notices/full texts and source/replacement instructions; deliberately extend allowed package paths and make the materials reach the archive and both installation modes.
- **Expected files:** New legal materials; packaging/windows/Package-Release.ps1; payload-files.txt; JobTracker.iss; tests; manifest/docs.
- **Acceptance check:** Extracted final archive and both installed copies contain the approved complete materials for actual delivered dependencies; strict allowlist checks still pass.
- **Dependencies:** R01/R03/R04/R06 before finalization.
- **Responsible party:** Release engineer with owner/legal approval.
- **src/ required: NO.** Packaging/documentation delivery only.

## Implementation / requirement reconciliation

### R07 — P1

- **Checklist:** T10-04/AC; T07-04.
- **Finding:** RC1: checklist/task10 permit an explicit upgrade-directory change, but policy rejects all relocation of an existing scope.
- **Required action:** Obtain an explicit acceptance decision for uninstall/reinstall relocation, or implement safe same-scope relocation. Check old/new files, rollback, shortcuts, registration, other-scope isolation and personal-data preservation.
- **Expected files:** packaging/windows/InstallationPolicy.iss; deployment tests; requirements and release/installer docs if an accepted constraint changes.
- **Acceptance check:** Owner-approved relocation contract is met by a disposable-system custom-directory upgrade and cancellation matrix.
- **Dependencies:** Owner decision first; R11; before final R12/R14 acceptance.
- **Responsible party:** Owner and packaging maintainer.
- **src/ required: NO.** Installer directory/state management.

## Verification first / possible packaging fix

### R08 — P1

- **Checklist:** T11-04/AC; T10-AC; GLOBAL-15.
- **Finding:** RC2: native rollback messages do not prove original-state preservation after an earlier file was overwritten and a later manual-close gate fails.
- **Required action:** In a disposable VM, snapshot every installed binary, registry entry and shortcut; trigger a late lock/cancel after an overwrite, then compare all bytes/state. Cover late uninstall failure. If restoration fails, fix the packaging transaction/rollback or obtain an explicit change to the absolute acceptance criterion.
- **Expected files:** New VM evidence; tests/deployment; potentially packaging/windows/ManualClose.iss/transaction strategy and docs.
- **Acceptance check:** Original-state/no-partial-loss promise is demonstrated for the approved contract; no weakening of manual-close gates or forced termination.
- **Dependencies:** R11; candidate otherwise stable; coordinate R10.
- **Responsible party:** Packaging maintainer and test operator.
- **src/ required: NO.** Verification and expected packaging correction. Application cooperation would require a separate owner-approved scope decision.

### R10 — P1

- **Checklist:** T13-12; T10-AC; GLOBAL-13.
- **Finding:** Retained policy/repair runs failed with Windows error32; later identical-installer passes do not explain or fix it.
- **Required action:** Reproduce upgrade/repair in disposable systems. Capture exact-path I/O with process identity, desired access/share modes and timestamps when it recurs; identify the lock owner and fix the demonstrated cause. Preserve all old failures; do not add sleeps or disable safeguards as an assumed fix.
- **Expected files:** New trace/log evidence; tests/deployment; installer scripts if root cause is there; perhaps a separately scoped application area if evidence points there.
- **Acceptance check:** Documented causal diagnosis plus targeted regression and repeated upgrade/repair success; no unfounded 'fixed' claim.
- **Dependencies:** R11; coordinate R08; final lifecycle acceptance after any change.
- **Responsible party:** Packaging maintainer/test operator.
- **src/ required: UNKNOWN.** Root cause has not been established. It may be installer, host or application behavior. Any proposed src/ change requires a separate owner decision and evidence identifying that area.

## Verification / requirement reconciliation

### R09 — P2

- **Checklist:** T12-09/AC; GLOBAL-18.
- **Finding:** RC4: consented handle-by-handle cleanup is not transactional; a late error/crash can leave a partially deleted confirmed tree.
- **Required action:** Owner explicitly defines the intended preservation guarantee. In disposable fixtures test late disposition failure, crash and concurrent new child; prove the preserved remainder/outside-tree boundary. If all-or-nothing rollback is mandatory, design and review recoverable packaging cleanup before implementation.
- **Expected files:** tests/deployment/Test-PersonalData.ps1 and harnesses; PersonalData.iss only if needed; clarified requirements/docs; new evidence.
- **Acceptance check:** Documented contract matches observed late-failure behavior; no unconsented or unrelated data is deleted.
- **Dependencies:** Owner interpretation; R11. Escalate to P1 if literal all-or-nothing cleanup is required.
- **Responsible party:** Owner and packaging maintainer.
- **src/ required: NO.** Installer cleanup and disposable verification only.

## Implemented functionality awaiting verification

### R12 — P1

- **Checklist:** T07-03/05/06/07/09/AC; T10-05/06/10; T13-04/05; GLOBAL-10/11.
- **Finding:** Both-scope actual installation/UI/shell/registry independence remains unverified.
- **Required action:** Test fresh CurrentUser and AllUsers, defaults and valid/invalid custom paths, equal-version coexistence, collision rejection, independent maintenance/uninstall, English/Russian/unsupported locale, language selection and desktop unchecked/opt-in behavior.
- **Expected files:** New isolated scenario logs/screenshots/hash/registry/shortcut snapshots; existing deployment test matrix.
- **Acceptance check:** Every mode/UI/path/shell scenario has actual PASS/FAIL evidence; unrelated installation and dummy data remain intact.
- **Dependencies:** R11 and R07; use stable corrected candidate from R01–R05.
- **Responsible party:** Test operator with owner UAC participation.
- **src/ required: NO.** Verification only; any defect is separately triaged.

### R13 — P1

- **Checklist:** T09-01/02/03/04/06/07/AC; T13-07; GLOBAL-12.
- **Finding:** App-Local probe passed, but real system redistributable install/failure/reboot/UAC and per-user no-admin behavior did not run.
- **Required action:** Exercise per-user without admin; AllUsers with absent/old/current runtime; official download failure/cancel; prerequisite launch/exit failures; reboot-required exits and rerun. Check AllUsers has no accidentally adjacent CRTs and never removes system runtime.
- **Expected files:** Disposable VM images and logs; tests/deployment runtime scenarios; exact MSVC package/hash evidence.
- **Acceptance check:** Actual installed JobTracker starts after each permitted branch; prerequisite success is checked beyond download; blocked/reboot paths do not mutate app data/files unsafely.
- **Dependencies:** R11, R06 and final runtime lock/candidate.
- **Responsible party:** Test operator/owner.
- **src/ required: NO.** Verification of existing branch implementation.

### R14 — P1

- **Checklist:** T10-01/02/07/08/09/AC; T13-04/09; GLOBAL-13/14.
- **Finding:** Synthetic metadata and migration tests do not prove real historical upgrade/shared SQLite/CV compatibility or concurrent equal-version use.
- **Required action:** Use actual older/newer binaries and dummy SQLite/CVs/settings for upgrade, repair and downgrade rejection in each scope. Test unequal-version blocks, loaded/unloaded/different-user profiles and permitted equal-version concurrent use; record unregistered-copy limits.
- **Expected files:** Historical test artifacts, synthetic data, new before/after integrity/DB/CV evidence and scenario matrix.
- **Acceptance check:** No data loss/schema misuse or other-copy damage; blocked combinations remain blocked; actual permitted workflows succeed.
- **Dependencies:** R11, R07/R08/R10 resolution and final candidate.
- **Responsible party:** Owner supplies artifacts; test operator runs cases.
- **src/ required: NO.** Verification only. Any application/schema defect requires separate scope and src/ authorization.

### R15 — P1

- **Checklist:** T11 acceptance; T12 acceptance; T13-07/08; GLOBAL-15/16/17/18.
- **Finding:** Real application manual closure, late uninstall, data-consent UI, cross-account and dual-scope cleanup remain unperformed.
- **Required action:** Run real JobTracker in disposable accounts; verify affected-path Retry/Cancel and other same-name/session survival. Test normal/silent/cancelled/partial uninstalls, explicit No/Yes, second registered copy, different-account UAC, locked files, ownership ambiguity, junction/hardlink and invalid root fixtures.
- **Expected files:** New VM logs/screenshots and before/after dummy-data snapshots; existing tests/deployment matrix.
- **Acceptance check:** Default/ambiguous cases preserve data, other copies remain unaffected, only explicitly approved safe cleanup occurs; no user process is force-closed.
- **Dependencies:** R11; approved R08/R09 contracts; stable candidate.
- **Responsible party:** Test operator with owner environment support.
- **src/ required: NO.** Verification only; defects would require separate triage.

### R16 — P1

- **Checklist:** T04-AC; T05-AC; T09-AC; T13-02/10; GLOBAL-4/5.
- **Finding:** Hidden probe is not actual JobTracker startup/rendering/native dialogs/SQLite/CV behavior on clean Windows. DXC/DXIL deployment warning persists.
- **Required action:** Install final candidates in clean Windows 10/11; launch the actual application with dummy data, render each supported graphics path, exercise native dialogs, SQLite and CV operations, and inspect icon/taskbar/shortcuts. Resolve actual missing graphics dependencies if demonstrated.
- **Expected files:** New clean-system evidence; packaging/deployment config only if a defect is demonstrated.
- **Acceptance check:** Supported installed app workflows and graphics succeed without Qt/VS SDK/PATH; missing-DXC warnings are resolved or backed by explicit supported-platform evidence.
- **Dependencies:** R11; R01–R05 final artifact choices; R13 runtime branch readiness.
- **Responsible party:** Test operator and release engineer.
- **src/ required: NO.** Checks do not require src/ changes. Any discovered application issue must be separately classified.

## Implemented configuration awaiting verification

### R18 — P2

- **Checklist:** T02-07/AC; T03-04/08/AC; T15-10; GLOBAL-3.
- **Finding:** Qt 6.10.3 Linux Debug configuration is preserved but actual Linux configure/build/test/startup remains NOT_RUN.
- **Required action:** On an available Linux host with Qt 6.10.3, supply its local SDK path and run documented Debug/test configure/build/CTest, then an isolated startup check. Do not add Linux packaging.
- **Expected files:** New Linux logs and local ignored preset/environment inputs; existing shared configuration only if a concrete defect is found later.
- **Acceptance check:** Actual Linux Debug/test results recorded, or a precise reproducible blocker; cross-platform behavior not inferred from Windows.
- **Dependencies:** Owner-provided Linux environment; retest if subsequent resource/build changes affect Linux.
- **Responsible party:** Owner/test operator.
- **src/ required: NO.** Verification only; any discovered defect needs separate triage.

## Owner / external decisions

### R06 — P1

- **Checklist:** T08-02; T09-01; T14-09; GLOBAL-20/22.
- **Finding:** Ownership, publisher terms and relevant tool/runtime entitlement are not proved by signatures. Source repository access settings are unverified.
- **Required action:** Confirm code/assets/contributor rights, publisher/product terms, source-repository visibility and Visual Studio eligibility/redistribution terms. Reconcile Inno commercial-use messaging with its exact license and chosen use; do not assume a purchase is mandated solely by the banner.
- **Expected files:** Owner/legal decision record; approved publisher metadata/customer terms; exact dependency/provenance references.
- **Acceptance check:** Written owner decisions tied to final components; applicable rights/terms established without narrowing third-party rights.
- **Dependencies:** R01–R05 evidence informs the decision; must precede public approval.
- **Responsible party:** Owner/legal.
- **src/ required: NO.** Governance and legal/metadata work.

## Owner / external environment

### R11 — P1

- **Checklist:** T07-AC; T09-AC; T13-04/05/07/08/10; GLOBAL-5/23.
- **Finding:** No disposable clean Windows environment was supplied/established for this audit; the host has Qt/VS/system runtime.
- **Required action:** Provide disposable Windows 10 and 11 x64 VMs/Sandbox or equivalent test accounts/images, non-admin and admin credentials, snapshots, dummy CV/database/settings fixtures and historical installers where available. Keep real account data out of every scenario.
- **Expected files:** Isolated VM configurations/test fixtures and evidence directories only.
- **Acceptance check:** Test images demonstrably lack Qt/VS SDK and cover present/missing MSVC plus same/different-account UAC; resettable before each scenario.
- **Dependencies:** Owner provisioning; no production personal environment substitute.
- **Responsible party:** Owner/test operator.
- **src/ required: NO.** Environment provision only.

## Owner / external publication and final verification

### R17 — P1

- **Checklist:** T08-AC; T13-11; T16-05/06; GLOBAL-7/22/23.
- **Finding:** Exact version-locked GitHub payload HEAD returned404; no successful public Online install or owner publication authorization exists.
- **Required action:** After hold resolution, owner authorizes exact assets. Publish only approved archive/checksum/notices to the selected fixed tag; verify remote bytes; compile a matching Setup; perform actual HTTPS download/failure/retry/cancel and clean-machine application checks; publish Setup only after separate final approval.
- **Expected files:** Approved public artifact set and new network/VM evidence; generated metadata/manifests; final handoff.
- **Acceptance check:** Public bytes match compiled SHA/version; final Setup passes on clean Windows after payload publication. No private source/build logs/generated includes are published.
- **Dependencies:** R01–R16 applicable blockers resolved; owner approval at archive and Setup publication boundaries.
- **Responsible party:** Owner/publication authority and test operator.
- **src/ required: NO.** Release assets, authorization and external tests only.

## Owner / requirements and final acceptance

### R19 — P2

- **Checklist:** T16-01/10/AC; GLOBAL-25/30/46.
- **Finding:** Original Promt_deploy.docx is missing; derivative 36-row map cannot prove original22-section wording and 22.1–22.13 coverage.
- **Required action:** Owner supplies the original or explicitly designates retained agreed requirements/checklist as authoritative replacement. Reconcile all wording/deltas, resolve RC1–RC5 and update completion records only in a separately authorized follow-up after required work is done.
- **Expected files:** Original input or owner decision; new reconciliation report; future STATUS/16 addendum only if authorized.
- **Acceptance check:** Every authoritative original requirement maps to current evidence/accepted deferral; no unqualified16/16 acceptance based solely on labels.
- **Dependencies:** Owner source-of-truth decision and remaining applicable findings.
- **Responsible party:** Owner and auditor.
- **src/ required: NO.** Requirements/reporting work; no implementation fix authorized here.

## Owner / repository housekeeping

### R20 — P2

- **Checklist:** T03-06; T15-AC; GLOBAL-34.
- **Finding:** CMakeUserPresets.json is still tracked despite ignore rule; originaltask03 instructed reporting safe untracking without unauthorized Git writes.
- **Required action:** In an explicitly authorized Git task, remove only the index entry while preserving the local preset file, then confirm shared configuration remains portable and local builds still discover the SDK.
- **Expected files:** Git index entry for CMakeUserPresets.json; local file preserved; .gitignore already present.
- **Acceptance check:** Local preset absent from tracked content but intact on disk; no shared machine-specific path introduced.
- **Dependencies:** Explicit owner Git-write authorization. Not a reason to mutate Git in this audit.
- **Responsible party:** Owner/repository maintainer.
- **src/ required: NO.** Index-only housekeeping.

## Optional improvements outside the original assignment

- **O1 — src/ required: YES if corrected.** The fresh production build retains warning C4715 at `src/maintenance/StorageMutationGate.hpp:93` (logs/03-build-release.log:102). A separately authorized backend review can determine whether the enum/control-flow contract needs a total return path. This audit did not investigate or change that logic and does not promote a compiler warning alone into a confirmed release defect. Owner permission is required for any src/ edit.
- **O2 — src/ required: NO.** CI/CD, Linux installers, an automatic updater, actual signing and a final branded icon are future scope, not failures of these 16 tasks. Any new project for them needs its own instruction. Signing absence/SmartScreen behavior must still be disclosed for this unsigned candidate.

No silent permission to modify src/, mutate Git, publish, or perform tests on personal data follows from this work list.




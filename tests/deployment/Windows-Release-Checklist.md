# Windows release acceptance checklist

Use this checklist for one completed Setup: the launcher's direct
`Installer/Windows/output/JobTracker-Setup-<version>-<UTC-time>-<id>.exe`, or the
manual builder's `Installer/Windows/output/<run-id>/JobTracker-Setup-<version>.exe`,
and its matching private `installer-build.json`. Record results with those
artifact hashes in the release/test evidence. This is a procedure, not a claim
of success. Use the [installer reference](../../For-Agent/Docs/installer.md)
and [suite commands](../../For-Agent/Docs/installer-testing.md).

## Evidence rules

- Record Windows edition/build, x64 architecture, account/elevation, runtime
  state, artifact hashes, language, destination, exit code, and log path.
- Use the exact creation invocation's evidence and approved filename; never
  select an arbitrary newest EXE. Launcher success does not execute acceptance.
- Record `PASS`, `FAIL`, or `NOT_RUN` per case. Distinguish source inspection,
  native harness, real installer integration, interactive UI, and clean VM.
- Run build/test processes sequentially. Retain evidence under fresh `build/`
  paths and owned test fixtures under
  `build/installer-tests/<run-id>/fixtures/<scenario>/`. Preserve failed results.
- Own only recorded test folders and per-copy shortcuts. Personal installations
  and data are outside fixture cleanup. Verify fixture provenance and the owner
  required by each case; do not normalize administrative-ownership fixtures.
- Preserve exact native errors, affected paths, and remaining files; never
  weaken path safeguards, terminate apps, or claim rollback merely to pass.
- Use disposable accounts/VMs for actual app startup and data. Environment
  overrides do not prove that Windows known-folder storage was redirected.

## Automated local pass

1. For release acceptance, build the requested production Release candidate and
   run the applicable separate test-enabled checks. Record configuration,
   selected Qt kit, and test isolation; no production target depends on tests.
2. Stage into a fresh prefix and run `Test-OfflinePayload.ps1`; retain negative
   fixtures and original-input hashes.
3. Compile Setup with authentic pinned local MSVC inputs, then run
   `Test-OfflineInstaller.ps1` with its matching JSON. Verify native x64 identity,
   exact embedding, no JobTracker registration, and local-uninstaller settings.
4. Run independent deployment checks against the stage plus the verified
   App-Local CRT; verify confined QML/plugin and loaded runtime paths.
5. Run runtime, installation-policy/repair, and personal-data harnesses. Execute
   real fixture installation separately. Verify two copies, per-copy shortcuts,
   repair, actual operation failures, and preserved unrelated files.
6. Execute the clean-host/UI matrix below with networking disconnected. Source
   or harness checks alone cannot establish those results.
7. Record fixture cleanup and confirm application C++/Qt/QML source, startup,
   database/schema/CV behavior, concurrency, and recovery remain unchanged.

The deployment probe checks dependencies and in-memory SQLite. It does not
execute `JobTrackerApp`, verify its window, or certify a clean machine.

## Installation and lifecycle matrix

Repeat applicable cases on clean Windows 10 x64 and Windows 11 x64 without Qt or
Visual Studio, with networking disconnected. Use snapshots and synthetic SQLite,
CV, and settings data, preserving before/after hashes.

| Case | Expected behavior and evidence |
|---|---|
| First CurrentUser installation, EN | No UAC; editable user default; Start Menu shortcut; desktop unchecked; local uninstaller; no JobTracker Installed apps entry. |
| First AllUsers installation, EN | Explicit UAC; editable machine default; common per-copy shortcuts; local uninstaller; no JobTracker Installed apps entry. Launch as a standard user. |
| Arbitrary suitable local drive, EN/RU | Selected regular local destination, including spaces/non-ASCII, works with actual permissions. No fixed drive-letter or volume-label list. |
| Additional copy, same/different scope or volume | Both copies coexist with separate program files, runtimes, metadata, uninstallers, and shortcut identities. Installing one changes neither the other nor shared data. |
| Another copy's drive disconnected | Install/repair into an available separate folder without requiring the unavailable copy. |
| Same-version repair | Restore missing/damaged owned files and a missing uninstaller. Preserve valid existing uninstall history and unrelated sentinels. |
| Newer version, same folder | Update only that copy, keeping data and ownership of prior-version files. Test a genuine older candidate separately from synthetic version fixtures. |
| Older version, known-newer same folder | Reject overwrite; preserve selected program files and data. |
| Older version, different folder | Allow installation despite a newer copy elsewhere. Do not infer database compatibility from this policy. |
| Whole program folder deleted in Explorer | Data remains; no uninstall prompt appears; fresh Setup succeeds at that or another suitable location without old metadata. |
| Partial deletion, missing EXE/INI/log | Use available local evidence for repair. If evidence cannot identify the nonempty folder, refuse adoption; missing files alone do not permanently forbid repair. |
| Missing/damaged uninstall data | Rebuild ownership only for identifiable files. Unrelated files remain unowned, and unavailable history is not claimed recovered. |
| Missing ownership ledger | Recognize only exact current-payload hashes during uninstall; changed unprovable files are preserved and reported incomplete. Repair establishes current bookkeeping. |
| Unrelated nonempty folder | Refuse silent adoption and preserve all files. |
| Nested installation/data overlap | Reject a folder containing/inside another identified installation and any overlap with personal data; outside sentinels remain unchanged. |
| Uninstall one of two copies | Remove only its owned files/metadata/shortcuts; preserve the other copy and unrelated files inside the selected folder. |
| Manual deletion of one copy | The other copy still launches with its own dependencies; a stale shortcut does not block reinstall. |
| Copied/moved/renamed folder | Before repair, local uninstall must not remove source-location files or shortcuts. Repair at the new path establishes local ownership; subsequent uninstall affects only that copy. |
| Copied binaries on another machine/account | No original-machine data transfer; existing per-user application path behavior remains. |
| Shortcut ownership and retargeting | Distinct shortcuts coexist; uninstall preserves another copy's links and a link retargeted by the user. RU/EN and desktop opt-in remain correct. |
| CurrentUser → AllUsers, same directory | After transition, Start Menu and selected desktop links use the common shell folders. Handle old verified user links explicitly, retain pending failed removals, and preserve retargeted/other-copy links. Reinstall and uninstall afterward. |
| AllUsers → CurrentUser, same directory | After transition, active links use the selected user's shell folders. Remove verified old common links when Windows permits; otherwise report incomplete operation and retain retry records. Reinstall and uninstall afterward. |
| Desktop checked → unchecked → checked | Deselecting on reinstall removes the verified owned desktop link and creates no replacement; selecting again creates or safely reuses the correct link. Metadata follows the actual result. |
| Desktop retargeted/unverifiable or deletion denied | Preserve retargeted/unverifiable links. A failed deletion of a verified owned link must keep its location recorded and report incomplete work; verify repair/retry. |

## Personal data matrix

The target is exactly the confirmed user's `%LOCALAPPDATA%\JobTracker\Data`.
Place sentinels in its parent, another user's fixture, and original-CV locations
outside `Data`; all must survive every cleanup case.

| Case | Expected behavior and evidence |
|---|---|
| Keep/default No/dismissal | Preserve all personal-data hashes. Display the exact profile/path and explain sharing by every copy used by that Windows user. |
| Silent uninstall | Preserve data, regardless of other copies or command-line requests for deletion. |
| Explicit Yes | Start cleanup immediately after confirmation; delete only the confirmed `Data` tree. Parent folder and outside original CVs survive. |
| Missing Data | Report cleanup already complete. Do not create/restore data. |
| User-owned or empty Data | Verify the fixture's intended owner before cleanup, including an empty root created after fixture initialization. With deletion permission, remove only Data and preserve all outside sentinels. |
| Administrators-owned Data root or child | Retain and verify `BUILTIN\Administrators` ownership during the test. With normal Windows deletion permission, cleanup succeeds inside the exact confirmed user's Data; it does not change ownership/ACLs or trust other administrative paths. |
| Selected or another copy running | Still offer the choice; Yes invokes cleanup without waiting, polling, terminating, restarting, or communicating with applications. Record actual OS errors/recreated files. |
| Closed remaining copy or disconnected copy | Yes invokes cleanup; presence/absence of other installations does not change the default or authorize automatic deletion. |
| Incomplete program removal/unrelated leftovers | Data choice/confirmed cleanup runs first. Attempt program removal immediately afterward; any owned-file failure returns native failure and retains uninstall metadata/log/EXE for retry. Unrelated leftovers do not require an empty folder. |
| Same-account elevation/AllUsers | Keep/delete remains available; show the intended profile and data path. Actual access errors are reported normally. |
| Alternate-account UAC credentials | Explicitly handle the account change and identify the target; never silently substitute the administrator's Local AppData or delete other users. |
| Locked/read-only/denied data | Report complete versus incomplete cleanup and each remaining failure. Files already removed are not restored or falsely reported preserved. |
| Reparse/hardlink/unsafe path | Stop unsafe traversal, preserve outside sentinels, and report the safety failure separately from normal OS operation failure. |
| App recreates files after cleanup | No new coordination/recovery logic is expected. Do not promise the directory remains absent while an app runs. |

## Failure and runtime matrix

| Case | Expected behavior and evidence |
|---|---|
| No network | Both scopes install from the single EXE and launch on the clean VM without dependency downloads. |
| Missing/mismatched stage/runtime input | Build rejects before publishing a usable Setup; preserve diagnostics and source hashes. |
| Debug/dev/test/source artifact or wrong PE identity | Exact payload validation rejects; embedded QML resources remain allowed. |
| Damaged embedded payload/prerequisite | Integrity checks reject affected bytes. No false successful installation or prerequisite execution. |
| Unsafe path/junction/traversal | Reject operations outside intended directories; preserve outside sentinels. |
| No machine CRT, either scope | Each installed copy has its own verified App-Local DLLs and launches independently. Test removal of a sibling copy. |
| AllUsers runtime present/missing | Verify the documented system-prerequisite skip/install branch with networking disconnected; shared runtime removal is not part of JobTracker uninstall. |
| Prerequisite failure/cancel/reboot | Exercise real prerequisite outcomes on snapshots; preserve personal data and report required next action. Harness return codes are not execution evidence. |
| Another app copy running | Installation in a different folder succeeds without a close request or process action. |
| Selected app running/late file lock | Attempt replacement/removal. Report actual Windows lock/access failures; no preliminary close gate or process retry loop. Incomplete owned-program removal fails while retaining local uninstall bookkeeping. |
| Insufficient space/permissions | Report the actual failure; preserve personal data and unrelated files; retain repairable local evidence where possible. |
| Cancellation/interruption | No personal-data reset/restore/migration. Verify subsequent local repair and report any remaining limits; do not claim atomic upgrade rollback. |
| Repeated sharing error | Retain timestamp/path, failing operation and access/share flags; obtain a narrow trace before attributing the lock. |

## Mixed-version application data

One Windows account shares `JobTracker\Data` across all copies and scopes. The
installer adds no database compatibility check, profile lock, single-instance
rule, or data isolation. Existing application schema validation, migration,
resume handling, concurrency, and recovery remain unchanged.

Test supported forward upgrades and newer-schema rejection using existing
application suites and genuine historical binaries on disposable snapshots.
Synthetic file/metadata version tests establish installer selection policy only.
Equal version strings do not prove identical binary content. Do not infer
multi-process CV/filesystem guarantees from SQLite transactions alone.

## Completion record

Include exact commands/hashes, evidence level, failed attempts, unresolved
issues, fixture cleanup, and unavailable environments. Mark every unexecuted
case `NOT_RUN`. Keep source, harness, real installation, UI, account/elevation,
and clean-machine results distinct.

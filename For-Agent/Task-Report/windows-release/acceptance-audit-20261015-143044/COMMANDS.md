# Audit Command and Verification Record

Created: 2026-10-15 15:10 local time (Asia/Baku, UTC+04:00).

All project process entry points used `D:/Project_CV/Root` and PowerShell 7.6.5. CMake/CTest 4.0.2, Qt 6.10.3, MSVC 19.44.35229.0 and Inno 6.7.1 are recorded in AUDIT.md and the actual configure/compile logs. Read-only file/Git/rg inspections support the source-line matrix; the reproducible grouped metadata checks are C00/C04/C18/C19/C21/C22. No missing output or still-running session was treated as completion.

Outputs are retained under this audit's logs/snapshots and the isolated directories described in AUDIT.md. Creation paths were new. Redirecting output to an audit log does not run a second project operation. Adaptations are explicit: new build/output paths, explicit production target, copied binary test runs, reused probe, and audit-only PE parser output paths. Their success does not by itself prove all original dated commands were replayed unchanged.

## C00

```powershell
pwsh -NoProfile -File 'For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Capture-Integrity.ps1' -Phase before
```

- Exit code: **0**; process complete: yes.
- Before modifying checks; all project/src snapshots plus Git status/diffs/HEAD.

## C01

```powershell
cmake --preset windows-release-local -B C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/01-configure-release.log 2>&1
```

- Exit code: **1**; process complete: yes.
- FAIL environment: SDK-discovery access denied. Preserved log; no claim of code/configuration failure.

## C02

```powershell
cmake --preset windows-release-local -B C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/02-configure-release-host-access.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS configure. Approved host-tool access; no Windows UAC/RunAs. Binary directory override is an audit adaptation.

## C03

```powershell
cmake --build C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production --config Release --target JobTrackerApp > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/03-build-release.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS fresh production Release compile/link, tests OFF; final exit awaited. Approved host-tool access per project rule. Targeted explicit build-directory form adapts documented preset command.

## C04

```powershell
pwsh -NoProfile -File 'For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Inspect-Artifacts.ps1' -Phase before > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/04-current-artifacts.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS current retained-artifact hashes, staging manifest and installer-source bindings; snapshots/artifacts-before.json and artifact-verification-before.json. Actual command also redirected output to logs/04-current-artifacts.log.

## C05

```powershell
cmake --install C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production --config Release --prefix C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/staging > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/05-staging.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS install/deploy into new absolute external prefix. Install manifest only written in fresh build tree.

## C06

```powershell
pwsh -NoProfile -File packaging/windows/Package-Release.ps1 -BuildDirectory C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production -StageDirectory C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/staging -OutputDirectory dist/acceptance-audit-20261015-143044/package > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/06-package.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS archive/sidecar/manifest and every extracted file; new ignored dist output.

## C07

```powershell
pwsh -NoProfile -File packaging/windows/Build-Installer.ps1 -BuildDirectory C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production -ManifestPath dist/acceptance-audit-20261015-143044/package/JobTracker-0.1.0-windows-x64.7z.manifest.json -OutputDirectory dist/acceptance-audit-20261015-143044/online -IsccPath "D:/Inno Setup 6/ISCC.exe" -RedistributablePath build/qt-6.10.3/stage09-evidence/vc_redist.x64.exe > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/07-compile-online.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS compile only. Explicit retained RedistributablePath is a documented supported input. No prerequisite/Setup executed.

## C08

```powershell
pwsh -NoProfile -File tests/deployment/Verify-Staging.ps1 -StageDir C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/staging -Probe build/qt-6.10.3/windows-deployment-tests/Release/JobTrackerDeploymentSmoke.exe -WorkRoot C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/smoke -MsvcRuntimeDirectory dist/acceptance-audit-20261015-143044/online/msvc-runtime > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/08-staging-smoke.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS isolated probe. Reused unchanged existing probe binary, copied to private staging; no production app startup.

## C09

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Run-IsolatedTests.ps1 -Configuration Release > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/09-release-tests.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS 18/18 retained Release binaries in isolated copy; copied CTest registration rewrites build/output paths only; no current test compile.

## C10

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Run-IsolatedTests.ps1 -Configuration Debug > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/10-debug-tests.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS 18/18 retained Debug binaries in isolated copy; same isolation/deviation as C09.

## C11

```powershell
pwsh -NoProfile -File tests/deployment/Test-ReleasePackage.ps1 -BuildDirectory C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production -StageDirectory C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/staging -ManifestPath dist/acceptance-audit-20261015-143044/package/JobTracker-0.1.0-windows-x64.7z.manifest.json > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/11-package-tests.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS 19 archive checks; fresh unique dist/package-tests-3080de2ed9a944138bb5c674a17695ad.

## C12

```powershell
pwsh -NoProfile -File tests/deployment/Test-OnlineInstaller.ps1 -BuildDirectory C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production -InstallerDirectory dist/acceptance-audit-20261015-143044/online -ManifestPath dist/acceptance-audit-20261015-143044/package/JobTracker-0.1.0-windows-x64.7z.manifest.json -WorkDirectory dist/acceptance-audit-20261015-143044/online-tests -IsccPath "D:/Inno Setup 6/ISCC.exe" > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/12-online-tests.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS 24 test-only native loopback checks; no -CheckPublicDownload, real registration, shortcuts or app launch.

## C13

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/14-evidence/Inspect-PrivateResources.ps1 > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/13-private-resources.json 2>&1
```

- Exit code: **0**; process complete: yes.
- Inspection command exit 0; private-source criterion FAIL. Reads/decompresses in memory only; no recovered source saved.

## C14

```text
web.open("https://github.com/turbodiesel606/Releases/releases/download/v0.1.0/JobTracker-0.1.0-windows-x64.7z")
```

- Exit code: **N/A — tool retrieval**; process complete: yes.
- Cache miss; HTTP absence not established. logs/14-public-asset-web.json.

## C15

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Check-PublicAsset.ps1
```

- Exit code: **0**; process complete: yes.
- Script exit 0 means observation logged, not successful request. Sandbox socket access denied; preserved logs/15-public-head-sandbox.json.

## C16

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Check-PublicAsset.ps1
```

- Exit code: **0**; process complete: yes.
- Same script with approved HTTPS host access: HEAD404. logs/15-public-head.json. Does not establish public installer success.

## C17

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Verify-Identity.ps1 > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/17-identity.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS actual icon/version bytes for current retained and fresh EXEs. New audit helper adapted read-only stage 05 PE parser to these paths/output; LoadLibraryEx uses data-file mode, not app execution.

## C18

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Collect-Summary.ps1
```

- Exit code: **0**; process complete: yes.
- PASS metadata/hash collection; snapshots/audit-summary.json. ISCC file metadata0.0 is distinct from compile engine6.7.1.

## C19

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Verify-StaticEvidence.ps1
```

- Exit code: **0**; process complete: yes.
- PASS bounded static comparisons; exact script and logs/19-static-evidence.json retained. No runtime/legal clearance inferred.

## C20a

```powershell
pwsh -NoProfile -File 'For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Capture-Integrity.ps1' -Phase after
```

- Exit code: **0**; process complete: yes.
- Initial helper comparison falsely reported date serialization differences; raw property comparison found no changes. Initial result retained. C21 is corrected comparison.

## C20b

```powershell
pwsh -NoProfile -File 'For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Inspect-Artifacts.ps1' -Phase after > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/20-artifact-integrity.log 2>&1
```

- Exit code: **0**; process complete: yes.
- Same date serialization artifact as C20a; original result retained; C22 is corrected comparison.

## C21

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Capture-Integrity.ps1 -Phase after > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/21-final-project-integrity.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS corrected string-preserving comparison: zero changed protected/src entries. Actual invocation redirected to logs/21-final-project-integrity.log.

## C22

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Inspect-Artifacts.ps1 -Phase after > For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/logs/22-final-artifact-integrity.log 2>&1
```

- Exit code: **0**; process complete: yes.
- PASS corrected artifact comparison: zero changed monitored files. Actual invocation redirected to logs/22-final-artifact-integrity.log.

The initial integrity comparison retained all baseline bytes. It deserialized date strings into DateTime objects before JSON comparison, changing formatting only. The read-only diagnostic used `ConvertFrom-Json -DateKind String` and `Compare-Object -Property Path,Kind,Length,SHA256,Attributes,LastWriteUtc,Target`; it found zero project changes and zero artifact changes. Only new audit helpers were corrected; no project file was reverted or repaired.

Current tests do not include real installation/uninstallation, system redist execution, production personal-data cleanup, actual JobTracker startup or VM/UI/public end-to-end acceptance. Those are NOT_RUN and remain in REMAINING_WORK.md.




## C23a / C23b — report validation

```powershell
pwsh -NoProfile -File For-Agent/Task-Report/windows-release/acceptance-audit-20261015-143044/Validate-Audit.ps1
```

Initial exit **1** came from the new audit validator expecting an empty staged-diff patch file: an empty PowerShell pipeline had created no file. No project defect or write occurred. The retained porcelain snapshots show zero staged entries. The audit-only validator now checks those actual records; exit **0**, result PASS. The initial error and final machine-readable validation log are preserved. This is report QA, not a new application test.



# JobTracker — Expected Results of the 16 Windows Release Tasks

**Detailed acceptance checklist for Codex's work**  
**Requirements source:** `Tasks/Windows-Release/01_*.md` … `16_*.md` from `JobTracker_Windows_Release_Tasks_v2.zip`, `00_AGREED_REQUIREMENTS.md`, `00_RUN_NEXT.md`, `00_COVERAGE_MATRIX.md`, and the original `Promt_deploy.docx`.  
**Scope:** Windows 10/11 x64, MSVC 2022/v143, C++20, Qt 6.10.3, CMake, Inno Setup, GitHub Releases.  
**Purpose of this document:** to verify **what Codex was expected to implement**, not to assert that it has already been implemented.

> **How to use:** after task `NN` has been completed, open `For-Agent/Task-Report/windows-release/NN.md` and compare the actual changed files, commands, and test results against the criteria below. Do not mark an item complete merely because it says “DONE”: find evidence in the source code, artifacts, or actual execution.

## Understanding Completion

- **IMPLEMENTED** — the code, script, or document actually exists and satisfies the requirement.
- **PASS** — the check was actually performed; its command/scenario and result were recorded.
- **FAIL** — the check was performed and revealed a problem; fixes and retesting are required.
- **NOT_RUN** — the check was not performed; this is **not** PASS. For example, a clean Windows VM or a public GitHub Release asset is unavailable.
- **DONE (stage)** — the mandatory implementation for that particular stage is complete, and the report and `STATUS.md` have been updated; separately, honestly identified external checks may remain `NOT_RUN` where the task permits this.
- **Release-ready** — a stricter outcome: mandatory tests and licensing requirements are satisfied, and there are no critical release blockers.

For **each of the 16 tasks**, the expected output is a separate report at `For-Agent/Task-Report/windows-release/NN.md`, an updated row in `For-Agent/Task-Report/windows-release/STATUS.md`, a list of changed files, actual commands/checks with `PASS/FAIL/NOT_RUN`, and the information needed by the next Codex chat. A `BLOCKED`/`IN_PROGRESS` task must not be skipped just to proceed to the next number.

---

## Phase I. Build System and Instructions (01–03)

### 01 — Audit and Initial Setup of Codex Instructions

**Expected results**

- [ ] Codex has inspected the actual `CMakeLists.txt`, CMake Presets, Qt SDK, Windows/Linux Debug workflows, QML startup, Qt SQL/SQLite, tests, user-data paths, and agent instruction hierarchy.
- [ ] `For-Agent/Docs/windows-release-requirements.md` has been created with the **agreed requirements**, including Windows-first, **Qt 6.10.3 on both operating systems**, the Online Installer, both installation modes, GitHub Releases, data safety, and deferred work.
- [ ] A **small pointer** to `Tasks/Windows-Release/00_RUN_NEXT.md` and `For-Agent/Task-Report/windows-release/STATUS.md` has been added to `AGENTS.md` without removing existing rules.
- [ ] `For-Agent/Docs/build.md` explains how **new, verified** Release/install/package commands may be added; commands not yet available are not presented as working.
- [ ] `For-Agent/Task-Report/windows-release/STATUS.md` has been created with 16 stages and the initial status of the remaining tasks set to `PENDING`.
- [ ] `.gitignore` has been changed **selectively**: `Tasks/Windows-Release/` can be tracked in Git, while previous unrelated files under `Tasks/` remain ignored according to the existing policy.
- [ ] A list of outdated documents to update in task 15 has been prepared; a large-scale rewrite of the instructions has not yet been performed.

**Acceptance check:** a new Codex chat can read the entry point, see the status, and select task 02 without rereading every task or violating previous rules. `01.md` exists; task 01 is `DONE` in `STATUS.md`, and task 02 is next.

**Important:** task 01 must not change the build or create an installer; it establishes reliable context handoff between chats.

### 02 — CMake Refactoring: Separating Lifecycle Stages

**Expected results**

- [ ] CMake clearly separates `configure` → `build` → `test` → `install/deploy` → `package`.
- [ ] The conflicting automatic Windows `POST_BUILD` deployment (`windeployqt`) has been removed or replaced with a justified, controlled developer-only mechanism.
- [ ] The destructive Linux `POST_BUILD` step that could clean the source-tree `deploy/` directory and run installation during ordinary compilation has been removed.
- [ ] Duplicate `install(TARGETS JobTrackerApp ...)` rules have been eliminated; installation rules are consistent across platforms.
- [ ] Ordinary compilation **does not create a commercial installer** or delete staging/deploy directories.
- [ ] Windows Debug still builds, runs tests, and remains convenient to launch in the developer environment.
- [ ] Linux Debug remains functional, or the inability to verify it is honestly stated; platform-specific CMake commands are appropriately guarded.
- [ ] QML resources, the SQLite path, and business logic have not been changed merely for packaging.
- [ ] The minimum Qt version in CMake is compatible with the API being used (at least 6.10); the exact release SDK version is established in stage 03.

**Acceptance check:** actual Windows Debug configure/build/test results are available; Linux configure/build results are included when that environment is available. Running `cmake --build` does not unexpectedly package the application or delete `deploy/`. Verified commands have been added to `For-Agent/Docs/build.md`.

### 03 — CMake Presets and a Consistent Qt 6.10.3 Version

**Expected results**

- [ ] `CMakePresets.json` is no longer tied to a developer-specific `D:/...` SDK path and contains no outdated shared Qt 6.9.x paths.
- [ ] Consistent Windows Release configure/build/test presets have been added, including a test-enabled variant where needed.
- [ ] Existing Windows Debug, Debug-with-tests, and Linux Debug presets have been preserved or correctly adapted.
- [ ] **Qt 6.10.3, MSVC 2022 x64** is selected for release configurations; the Qt 6.10.3 requirement for Linux is reflected in configuration/documentation without implementing a Linux installer.
- [ ] Local Qt paths are supplied through a user preset or environment variables, not shared hardcoded values.
- [ ] `CMakeUserPresets.json` is excluded from Git; if it was previously tracked, safe instructions explain how to remove it from the index **without deleting the local file**.
- [ ] Release does not include `QT_QML_DEBUG`, `/JMC`, or other Debug-only settings.
- [ ] Presets have been checked for syntax and tested with actual configure/build/test commands where the environment is available.

**Acceptance check:** a fresh repository copy can be configured after supplying its own Qt path. Windows Debug and Release build, Qt 6.9.x is not used accidentally, and Linux has not been broken.

---

## Phase II. A Ready-to-Run Windows Application Without the Qt SDK (04–06)

### 04 — Windows Deployment: Qt, QML, SQLite, and Staging

**Expected results**

- [ ] A correct Windows Release install/deployment workflow has been implemented using official Qt/CMake mechanisms, particularly `qt_generate_deploy_qml_app_script()` and `install(SCRIPT ...)` where suitable for the current configuration.
- [ ] `cmake --install`, or a documented equivalent command, creates a separate **staging directory outside the source tree**.
- [ ] Staging contains the executable, required Qt 6.10.3 DLLs, platform plugins such as `qwindows`, QML imports/plugins, required image plugins, `QSQLITE`, and third-party runtime dependencies.
- [ ] Duplicate dependency copying through conflicting mechanisms has been eliminated.
- [ ] Release staging **does not contain** Debug DLLs, headers, development `.lib` files, Qt SDK tools, or other unnecessary files.
- [ ] QML, the Qt platform plugin, and QSQLITE availability/loading have been checked against actual results as far as the environment permits.
- [ ] Environment settings do not conceal a dependency on the developer PATH / Qt SDK.
- [ ] The MSVC Runtime policy is not yet declared complete: the implementation accounts for Qt Deployment potentially copying the compiler runtime; the final implementation belongs to task 09.

**Acceptance check:** staging can be generated reproducibly, and JobTracker runs from it in an environment isolated from the Qt SDK as far as possible. Limitations of clean-Windows verification are documented.

**Stage boundary:** there is **not yet** an archive for downloading from GitHub or a Windows installer.

### 05 — Temporary Icon, Windows VERSIONINFO, and a Single Version Source

**Expected results**

- [ ] A temporary `.ico` icon has been added and can later be replaced in one obvious location.
- [ ] The icon is integrated into the Windows EXE; its path is available to the future Inno Setup configuration for shortcuts, the installer, and the uninstall entry.
- [ ] The Windows EXE contains VERSIONINFO: file/product version, description, and neutral metadata; no legal publisher name or signature has been invented.
- [ ] `project(... VERSION 0.1.0)`, or the actual configured CMake project version, is the **single source of version information** for downstream packaging.
- [ ] The internal `JobTrackerApp` target is preserved; the output EXE may optionally be named `JobTracker.exe` without indiscriminate CMake target renaming.
- [ ] Debug, Release, staging, and existing tests continue to work.

**Acceptance check:** the correct version can be read from the actual Release EXE's properties or resources, and the temporary icon is visible; consistent version propagation to the future installer has been verified.

### 06 — Versioned Application Archive and SHA-256

**Expected results**

- [ ] A repeatable packaging script or CMake target has been implemented using **Release staging only**.
- [ ] It produces an archive such as `JobTracker-<version>-windows-x64.7z`, or another format where there is a justified compatibility advantage with the selected Inno Setup extraction mechanism.
- [ ] The archive has a predictable directory structure and contains no absolute machine-specific paths, secrets, private source code, Debug DLLs, or developer-only files.
- [ ] SHA-256 is calculated for **the exact final archive**, not an arbitrary intermediate file.
- [ ] A machine-readable checksum/manifest is generated for building the Online Installer; a filename, version, or checksum mismatch stops the process.
- [ ] The archive is saved to an ignored `dist/` directory or another explicit generated-output directory, not to the Qt SDK or source files.
- [ ] Extraction, archive contents, and SHA-256 recalculation have been checked.
- [ ] Uploading to GitHub **has not been performed automatically**.

**Acceptance check:** Codex provides the exact command that regenerates an archive with the correct version and matching SHA-256. The archive can be extracted without compiling source code.

**Stage boundary:** the archive does not yet prove that all MSVC dependencies work on clean Windows; the final strategy is implemented in task 09 and tested in task 13.

---

## Phase III. Online Installer (07–09)

### 07 — Basic Inno Setup Installer with Two Installation Modes

**Expected results**

- [ ] `packaging/windows/JobTracker.iss` and the required compilation configuration/scripts have been created.
- [ ] Inno Setup initially works with a **local test payload**, without pretending to be the final Online Installer.
- [ ] The user can choose installation **for the current user** (without UAC, default `%LocalAppData%\Programs\JobTracker`) or **for all users** (with UAC, default `%ProgramFiles%\JobTracker`).
- [ ] A custom valid installation directory can be selected; the installer handles invalid paths correctly and is designed to retain the selected path during upgrades.
- [ ] Both modes **can coexist**. Their registration scope, uninstall data, shortcuts, and paths are correctly provided for; a shared `AppId` is not accepted as proof of independence without verification.
- [ ] The installer interface supports English (the default for an unsupported locale) and Russian; users can select the language, and the system locale is handled correctly.
- [ ] A Start Menu shortcut is always created; the desktop shortcut is optional, **unchecked by default**, and uses the correct scope.
- [ ] The temporary icon from task 05, the version, and metadata are used.
- [ ] The `.iss` script compiles; available local checks of both modes, registration, and uninstallation have been performed.
- [ ] User SQLite/CV/settings data **is not deleted** at this stage; complex data-deletion logic is deferred to task 12.

**Acceptance check:** the local Setup can actually install the payload and register both installation scopes; limitations of privilege/UAC verification are documented.

### 08 — A Real Online Installer: GitHub, HTTPS, and SHA-256

**Expected results**

- [ ] The installer is built for **one fixed CMake version**: a tag such as `v<version>`, a specific archive filename, and a specific URL under `https://github.com/turbodiesel606/Releases/releases/download/...`; **not** `latest`.
- [ ] The private repository containing C++/QML source code remains private; public Releases are intended to contain only distributable artifacts and notices.
- [ ] Setup preparation embeds the expected SHA-256 of **the exact archive** generated in task 06.
- [ ] A missing checksum, mismatched version, or questionable configuration causes a safe build/installation failure.
- [ ] HTTPS downloading, progress display, EN/RU error messages, and correct handling of missing files, network interruptions, HTTP errors, cancellation, and temporary files have been implemented.
- [ ] SHA-256 verification takes place **before extraction**; failure prevents installation of an unverified payload.
- [ ] Extraction prevents path traversal, escaping the installation directory, and arbitrary file writes.
- [ ] Built-in, documented Inno Setup functionality is used where possible, without unnecessary downloader programs.
- [ ] A **local test scenario** (fixture / local server / mock) is available without requiring a public upload.
- [ ] The final Setup must not retain a fake URL, test checksum, or claim of successful GitHub downloading when the asset has not been published.

**Acceptance check:** a compiled, version-locked Online Setup has been produced and correctly rejects a corrupt payload. If the corresponding GitHub Release does not yet exist, the actual public E2E test must remain `NOT_RUN`.

**Stage boundary:** runtime completeness, upgrade/uninstall behavior, clean VM testing, and legal questions remain to be addressed in tasks 09–14.

### 09 — Hybrid Microsoft Visual C++ Runtime Deployment

**Expected results**

- [ ] For **per-user** installation, the necessary, legally redistributable MSVC Runtime DLLs are deployed app-local next to `JobTracker.exe`; installation does not require administrator privileges.
- [ ] For **all-users** installation, detection of an installed compatible Visual C++ Redistributable x64 has been implemented; when necessary, the **official Microsoft** package is obtained and installed.
- [ ] The implementation handles a missing runtime, an already installed runtime, package download/launch failures, return codes, UAC, cancellation, and required reboots.
- [ ] No unofficial mirrors are used; the x64 architecture and prerequisite version are checked, and successful downloading is not assumed to mean successful installation.
- [ ] Conflicts with Qt Deploy behavior on Windows have been eliminated, including potential automatic copying of MSVC DLLs.
- [ ] The Release archive and Online Setup are consistent: per-user receives the app-local runtime, while all-users does not depend on accidentally bundled adjacent DLLs contrary to the agreed policy. Separate mode-specific files/sidecar assets are provided where necessary.
- [ ] Actual scenarios with the redistributable installed and missing have been tested where an isolated environment is available; otherwise, limitations are explicitly stated.

**Acceptance check:** each installation branch follows its agreed runtime strategy, and JobTracker can run on clean Windows without Visual Studio once the necessary prerequisite steps have completed.

---

## Phase IV. Upgrade and Uninstallation Safety (10–12)

### 10 — Upgrades, Reinstalls, Downgrades, and Coexistence

**Expected results**

- [ ] When version 1.0.0 is installed, installer 1.1.0 performs an upgrade **within the same installation mode**.
- [ ] Installing the same version again is supported as repair/reinstall.
- [ ] Attempting to install 1.1.0 over 1.2.0 is **rejected**; actual installed-version metadata is compared, not archive filenames.
- [ ] Upgrades retain the previously selected installation directory unless the user explicitly changes it.
- [ ] Per-user and all-users installations have independent paths, registry entries, updates, uninstallers, and shortcuts.
- [ ] The two modes cannot install binaries **into the same physical directory**.
- [ ] The implementation accounts for both copies, when run by the same Windows user, potentially sharing **one SQLite database and CV files** through `QStandardPaths::AppLocalDataLocation`.
- [ ] A conservative policy for different versions of the two installations has been defined and implemented: unsafe combinations are prevented or justifiably restricted, with the risk recorded.
- [ ] The installer **does not** run SQLite migrations; upgrades/reinstalls preserve user data.
- [ ] Installation/upgrades in both scopes, registry behavior, path reuse, and downgrade rejection have been checked where possible.

**Acceptance check:** specific upgrade/repair/downgrade scenarios behave predictably, do not damage the other installation, and do not affect SQLite/CV data.

### 11 — Closing a Running JobTracker Manually Only

**Expected results**

- [ ] Inno Setup **does not automatically terminate the process** through Windows Restart Manager or `taskkill /F`.
- [ ] Before replacing/removing binaries, the installer determines whether **the affected installation** of JobTracker is running.
- [ ] A clear EN/RU message asks the user to close the application manually.
- [ ] `Retry` after closing and `Cancel` are available, without data loss or partially deleted files.
- [ ] When two installations with the same EXE name are running, detection distinguishes their paths where possible and does not interfere with the other process.
- [ ] The check is repeated before a dangerous file operation, including uninstallation.
- [ ] Results of running/closed/cancel scenarios are recorded, and limitations of process detection are described.

**Acceptance check:** Setup/Uninstaller does not replace an in-use EXE or terminate the user's work; it continues after the application closes and preserves the original state on cancellation.

### 12 — Safe Uninstallation and Optional Data Deletion

**Expected results**

- [ ] Codex has inspected `StoragePaths.cpp` and the actual Qt organization/application identity, and has established **the real data path**, rather than assuming it.
- [ ] Normal uninstallation removes **only** the selected installation: binaries, its registry uninstall entry, and the shortcuts it created.
- [ ] By default, the SQLite database, CVs, settings, and other user data **are always preserved**.
- [ ] Deleting personal data requires a separate, clear, and **voluntary confirmation** in EN/RU.
- [ ] If another JobTracker installation remains for the current account, **shared data is not deleted even with consent**; the user receives an explanation instead.
- [ ] The all-users uninstaller does not enumerate or delete other Windows users' profiles.
- [ ] If UAC uses a different administrator account, or directory ownership/the path cannot be confirmed, **data is preserved**.
- [ ] The exact directory is checked before deletion; deleting all of `%LocalAppData%`, path traversal, symlink/reparse-point bypasses, and deletion of unrelated directories are prevented.
- [ ] Cancellation, partial failures, locked files, and corrupt/questionable configuration do not cause accidental data deletion.
- [ ] Default preservation, explicit deletion, both installations, UAC mismatch, cancellation, and unsafe-target scenarios have been tested where possible; the remainder are marked `NOT_RUN`.

**Acceptance check:** uninstalling the application **does not mean deleting its data**; every ambiguous or unsafe situation results in data preservation.

---

## Phase V. Readiness Evidence and Redistribution Rights (13–14)

### 13 — Integration Testing of the Installer and Clean Windows

**Expected results**

- [ ] The entire pipeline has been rerun: Windows Release configure → build → C++/Qt tests → staging → archive + SHA-256 → Inno Setup compilation.
- [ ] QML imports, the Qt platform plugin, `QSQLITE`, and other runtime dependencies have been confirmed available to the packaged application without Qt SDK/Visual Studio PATH entries.
- [ ] An **acceptance-scenario matrix** has been prepared with separate `PASS/FAIL/NOT_RUN` results and actual evidence.
- [ ] Fresh per-user and all-users installations, coexistence of both, upgrading each separately, reinstall, downgrade rejection, and an alternative installation path have been tested.
- [ ] The EN/RU interface, optional desktop shortcut (unchecked), correct registration, and independent uninstallation have been tested.
- [ ] Poor network/offline conditions, a missing GitHub asset, SHA mismatch, download cancellation, a corrupt archive, and path traversal have been tested.
- [ ] Installed/missing/failing MSVC runtime scenarios, running JobTracker during upgrade/uninstallation, and safe cancellation have been tested.
- [ ] Data preservation, an explicit deletion request, and prevention of deletion while a second copy remains installed have been tested.
- [ ] The risk of different versions sharing one SQLite database, and compliance with the adopted blocking policy, have been examined.
- [ ] Actual smoke tests have been performed where possible on a **clean Windows 10/11 x64 VM/Sandbox** without the Qt SDK or Visual Studio.
- [ ] An actual GitHub download is marked PASS **only if** the asset is published and the check was genuinely performed.
- [ ] In-scope errors discovered during the task have been fixed, and the corresponding checks rerun; untested cases are honestly marked.

**Acceptance check:** a test table includes commands/logs/screenshots or other observable evidence. “The Inno Setup script compiles” **does not mean** “the installer works on clean Windows.”

### 14 — Qt/MSVC Licenses and Distribution Security

**Expected results**

- [ ] An inventory has been made of the Qt DLLs, Qt plugins/QML modules, SQLite components, MSVC Runtime, and other third-party binaries **actually bundled**.
- [ ] Versions, provenance, licenses, and applicable redistribution terms for these files have been checked; the historical research at `For-Agent/Task-Report/JobTracker_Commercialization_Research.md` has been used, but current conclusions are supported for the actual payload.
- [ ] Applicable notices and LGPL/GPL obligations, access to Qt sources/library replacement where required, and Microsoft redistribution terms have been recorded separately.
- [ ] Incompatible GPL-only modules have not been silently included in the intended proprietary commercial release.
- [ ] Required license/notice files are included in the release, or explicitly listed as **release blockers** where unresolved.
- [ ] The package has been checked for secrets, private source code, developer files, and Debug libraries.
- [ ] The URL has been confirmed to point to a specific version, SHA-256 is set for the exact payload, and checksum verification is actually enabled.
- [ ] Digital signing **is not implemented** at this stage; certificates and keys are not created. The future SignTool integration point is documented without claims that SmartScreen warnings will be absent.
- [ ] Legal questions requiring a decision from the owner or a lawyer are clearly separated from technically verified conditions.

**Acceptance check:** there is an inventory, a substantiated release compliance checklist, prepared notices, and a list of remaining legal/security blockers. Commercial legality cannot be considered proven by a successful Qt deployment alone.

---

## Phase VI. Maintained Documentation and Final Acceptance (15–16)

### 15 — Synchronizing Codex Instructions and Project Documentation

**Expected results**

- [ ] `For-Agent/Docs/build.md` is the **single canonical** list of actual, verified commands: Windows Debug/tests/Release, staging, archive, Setup, smoke tests, and Linux Debug.
- [ ] `For-Agent/Docs/build.md` specifies Qt 6.10.3 for both operating systems; misleading instructions to use Qt 6.9.x have been removed.
- [ ] `For-Agent/Docs/windows-release.md` has been created or completed with the actual build, packaging, download, dual-mode installation, MSVC, versioning, process-closure, and SQLite/CV protection workflow.
- [ ] Task routing and release-documentation references in `AGENTS.md` have been updated without rewriting other agent rules.
- [ ] `.agents/skills/cmake-build-debug/SKILL.md` has been updated, and `.agents/skills/windows-release-packaging/SKILL.md` has been created with frontmatter matching the repository's conventions.
- [ ] Both skills reference the canonical instructions rather than maintaining conflicting copies of commands.
- [ ] `For-Agent/Docs/testing.md`, `.agents/skills/test-and-review/SKILL.md`, `README.md`, and `.gitignore` have been updated.
- [ ] `BuildCommands.txt` and `qt_bins.txt` have been aligned with the new deployment mechanism or replaced with short pointers to current documents.
- [ ] `docs/BUILDING.md`, `docs/WINDOWS-RELEASE.md`, and `docs/WINDOWS-INSTALLER.md` have been created/updated, **or** equivalent existing files have been explicitly identified and explained.
- [ ] The documentation explains that Linux Debug is supported, while a Linux installer/.deb/AppImage is not yet implemented.
- [ ] Documented commands have been checked against actual targets/presets, paths, versions, asset names, and the SHA-256 flow.
- [ ] Historical reports under `For-Agent/Task-Report/`/`For-Agent/Review/` have not been rewritten; `.codex/config.toml` and C++/QML agents have not been changed without a reason.

**Acceptance check:** a **new Codex chat**, seeing only the Git repository and current instructions, can determine how to build Windows Release and Online Setup without using outdated Qt/deployment commands. A separate list identifies documents changed and those intentionally left unchanged.

### 16 — Independent Final Audit and Manual Release Instructions

**Expected results**

- [ ] Codex has compared the implementation again against the original **22 sections** of the large prompt, including subsections 22.1–22.13, and `00_COVERAGE_MATRIX.md`.
- [ ] CMake, Qt staging, the archive, Inno Setup, runtime branches, versions, uninstall/data safety, updated agent rules, and documentation accuracy have been checked.
- [ ] Only affected tests have been rerun where necessary; a documentation-only change is not, by itself, a reason for a full rebuild.
- [ ] **Actually existing** artifacts, exact commands, paths, version, SHA-256, and PASS/FAIL/NOT_RUN results are reported; missing files are not presented as created.
- [ ] A manual release sequence has been provided: build → tests → staging → archive+SHA → **after owner authorization**, publish the archive to `turbodiesel606/Releases` under `v<version>` → build a strictly matching Setup → actual network/clean-environment tests → publish the installer after verification.
- [ ] If the GitHub release asset does not yet exist, actual GitHub downloading and a clean public Online Installer test remain `NOT_RUN`, not PASS.
- [ ] Current release blockers, if any, are listed, including LGPL/Qt questions, the unsigned installer/SmartScreen, shared-data risks between two versions, and unperformed VM tests.
- [ ] Completed documentation synchronization, unresolved limitations, decisions requiring the owner, and subsequent manual actions are recorded.
- [ ] It is confirmed that **CI/CD, Linux packaging, automatic updates, and code signing** were not implemented because they are separate future tasks.
- [ ] The final `For-Agent/Task-Report/windows-release/16.md` has been created, and `STATUS.md` reflects the actual state of all 16 tasks.

**Acceptance check:** the final report allows an independent decision on which requirements are actually fulfilled, what still needs testing, and whether publication may be authorized. `16/16 DONE` **is not a substitute for** commercial release approval.

---

## Summary of Expected Files and Artifacts

| Path / type | Available after | Purpose |
|---|---:|---|
| `Tasks/Windows-Release/00_RUN_NEXT.md` and `01_*.md` … `16_*.md` | Before starting | Unmodified input tasks for Codex |
| `For-Agent/Task-Report/windows-release/STATUS.md` | 01 → 16 | Single current status of the 16 tasks |
| `For-Agent/Task-Report/windows-release/01.md` … `16.md` | Every stage | Change audit, checks, and handoff |
| `For-Agent/Docs/windows-release-requirements.md` | 01 | Recorded agreed requirements |
| `CMakeLists.txt`, `CMakePresets.json` (adapted) | 02–04 | Correct Debug/Release/install/deploy workflow |
| `CMakeUserPresets.json` (local only) | 03 | Developer-specific SDK paths, not committed |
| Windows Release staging | 04 | Executable application and Qt/QML/SQLite libraries |
| Temporary `.ico` and Windows VERSIONINFO | 05 | EXE icon, version, and metadata |
| `dist/JobTracker-<version>-windows-x64.7z` (preferred) | 06 | Archive for GitHub Releases |
| SHA-256 manifest / checksum | 06 | Verification that the payload matches the expected bytes |
| `packaging/windows/JobTracker.iss` | 07–12 | Source of the complete Windows Online Installer |
| `dist/JobTracker-<version>-windows-x64-Setup.exe` (example) | 07–09 | Compiled installer for a specific version |
| Matrix / smoke-test scripts (implementation at Codex's discretion) | 13 | Actual acceptance testing of Windows behavior |
| License notices / manifest / audit | 14 | Redistribution and commercial distribution requirements |
| `For-Agent/Docs/windows-release.md` | 15 | Canonical agent guide for Windows Release |
| `.agents/skills/windows-release-packaging/SKILL.md` | 15 | Repeatable future releases from new Codex chats |
| `docs/BUILDING.md`, `docs/WINDOWS-RELEASE.md`, `docs/WINDOWS-INSTALLER.md` (or an explained equivalent) | 15 | Developer documentation |
| Complete handoff/report `16.md` | 16 | Final audit result and publication instructions |

**Note:** the implementation may justifiably change generated script names, the staging directory, checksum files, or the archive format. Acceptance depends on the actual compatibility of the components and documentation with exact names. Codex must not rewrite the original task directory without your request.

## Final Acceptance Matrix for an Actual Release

After all tasks have been completed, separately check the items below. They are particularly important because task 13 permits an honest `NOT_RUN` for environments Codex could not access.

### Build and Launch

- [ ] Windows Debug builds and existing tests pass.
- [ ] Windows Release builds with Qt **6.10.3 msvc2022_64**.
- [ ] Qt 6.10.3 is specified for Linux, and the existing Debug build is preserved, or a blocker/NOT_RUN is recorded.
- [ ] Qt 6.10.3 DLLs, QML, `qwindows`, `QSQLITE`, and other required runtime components are available to the installed application.
- [ ] On **clean Windows without Visual Studio/Qt SDK**, the application launches, displays its interface, and works with SQLite and test CVs.

### Installer and Delivery

- [ ] The correctly versioned archive extracts successfully and matches its declared SHA-256.
- [ ] Online Setup downloads **the actual published** archive over HTTPS from a version-specific GitHub URL.
- [ ] No internet connection, an HTTP error, an incorrect SHA, or a corrupt archive causes neither unsafe installation nor unwanted deletion.
- [ ] Restart/retry or cancellation after a network error has been tested.
- [ ] The installer offers **English and Russian**; button labels and user-facing error messages are correct.
- [ ] Both installation modes work independently, including shortcuts, uninstall registry entries, and directory selection.
- [ ] Per-user installation requires no administrator privileges and uses App-Local MSVC; all-users installation handles the official MSVC Redistributable correctly.

### Data Safety

- [ ] Upgrading to a newer version and reinstalling the same version do not lose SQLite/CV/settings data.
- [ ] Downgrades are prohibited, accounting for the risk of shared-database incompatibility between installation modes.
- [ ] The installer does not terminate a running JobTracker; the user closes it manually and can cancel the operation.
- [ ] Uninstallation preserves data by default.
- [ ] A data-deletion request requires explicit consent, and data is preserved when a second copy remains installed.
- [ ] A UAC error/different Windows user/ambiguous path/symlink results in data preservation rather than risky deletion.

### Pre-publication Checks

- [ ] Report 13 contains no critical unverified tests on which release safety depends.
- [ ] Report 14 resolves licensing obligations and information about the libraries actually distributed.
- [ ] Report 16 lists all known FAIL, NOT_RUN, and release blockers without concealing any.
- [ ] The owner has authorized a public GitHub Release, and the files have been manually published in the correct order.
- [ ] The final Setup has been tested on clean Windows **after** the actual archive was published.

## What Is Intentionally OUTSIDE These 16 Tasks

- A complete Linux Release build and Linux `.deb`/AppImage/Flatpak installers.
- GitHub Actions / CI/CD / automatic release creation and publication.
- Automatic JobTracker updates from within the application.
- Actual EXE/Setup code signing and a guarantee of no SmartScreen warnings.
- Rewriting controllers, QML business logic, the SQLite schema, or migrations.
- Automatic file publication or Git commits without separate authorization.

## How to Quickly Review Codex Reports

1. Open `For-Agent/Task-Report/windows-release/STATUS.md` and find all `BLOCKED`, `IN_PROGRESS`, `FAIL`, and `NOT_RUN` entries.
2. For each task number, compare `NN.md` with the corresponding section of this checklist: **files → code/scripts → command → result → limitation**.
3. Pay particular attention to reports **04, 08, 09, 10, 11, 12, 13, 14, and 16**, where mistakes can potentially be most costly.
4. Perform a separate test on a clean Windows VM/Sandbox using a test SQLite database and test CVs, not real personal files.
5. Do not publish the installer for users until critical scenarios have confirmed results and the necessary licensing questions have been resolved.

**Success criterion:** an independent engineer can take the private JobTracker repository and current instructions, produce a ready-to-run Windows Release, create a matching archive and Online Installer, and demonstrate correct installation, application behavior, safe upgrading, and uninstallation on a supported clean Windows system — without requiring the user to install Qt/CMake/Visual Studio.

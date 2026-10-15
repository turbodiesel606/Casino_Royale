# Windows Release — persistent task status

The current instance of this file lives in `For-Agent/Task-Report/windows-release/STATUS.md`.
Task 01 initializes it; later tasks update it after each task report.

States: `PENDING`, `IN_PROGRESS`, `BLOCKED`, `DONE`. Do not skip `BLOCKED` tasks.

| # | Stage | State | Report | Verification / blocker |
|---:|---|---|---|---|
| 01 | Audit and bootstrap agent instructions | PENDING | — | — |
| 02 | Clean CMake lifecycle without breaking Debug or Linux | PENDING | — | — |
| 03 | Normalize CMake presets and Qt 6.10.3 | PENDING | — | — |
| 04 | Deploy Qt/QML/SQLite into Windows Release staging | PENDING | — | — |
| 05 | Temporary icon, Windows metadata and unified version | PENDING | — | — |
| 06 | Versioned application archive and SHA-256 | PENDING | — | — |
| 07 | Inno Setup foundation: two installation modes and UI | PENDING | — | — |
| 08 | GitHub version-locked Online Installer and secure downloads | PENDING | — | — |
| 09 | Implement hybrid MSVC Runtime deployment | PENDING | — | — |
| 10 | Upgrades, reinstalls, downgrades and coexistence safety | PENDING | — | — |
| 11 | Manual closure of running JobTracker before changes | PENDING | — | — |
| 12 | Safe uninstall and optional deletion of personal data | PENDING | — | — |
| 13 | Installer integration tests and clean-machine verification | PENDING | — | — |
| 14 | Redistribution/license audit and release safeguards | PENDING | — | — |
| 15 | Final synchronization of Codex instructions and developer docs | PENDING | — | — |
| 16 | Independent final review and manual GitHub release handoff | PENDING | — | — |

Next task: `01`

Once task 01 is completed, change its row to `DONE`, link to `01.md` and set next task to `02`. The status index is maintained only in the report folder, not in `Tasks/`.

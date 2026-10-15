# JobTracker Windows Release — 16 sequential tasks

For each **new Codex chat**, send only:

> Выполни следующую незавершённую задачу по `Tasks/Windows-Release/00_RUN_NEXT.md`. Сделай только один этап, запиши отчёт и остановись.

All task files and work reports persist in the same repository across chats. First task is automatically 01 if no status index exists.

## Task order

| # | Task file | Goal |
|---:|---|---|
| 01 | [`01_audit_and_bootstrap_agent_instructions.md`](01_audit_and_bootstrap_agent_instructions.md) | Audit and bootstrap agent instructions |
| 02 | [`02_clean_cmake_lifecycle_without_breaking_debug_or_linux.md`](02_clean_cmake_lifecycle_without_breaking_debug_or_linux.md) | Clean CMake lifecycle without breaking Debug or Linux |
| 03 | [`03_normalize_cmake_presets_and_qt_6_10_3.md`](03_normalize_cmake_presets_and_qt_6_10_3.md) | Normalize CMake presets and Qt 6.10.3 |
| 04 | [`04_deploy_qt_qml_sqlite_into_windows_release_staging.md`](04_deploy_qt_qml_sqlite_into_windows_release_staging.md) | Deploy Qt/QML/SQLite into Windows Release staging |
| 05 | [`05_temporary_icon_windows_metadata_and_unified_version.md`](05_temporary_icon_windows_metadata_and_unified_version.md) | Temporary icon, Windows metadata and unified version |
| 06 | [`06_versioned_application_archive_and_sha_256.md`](06_versioned_application_archive_and_sha_256.md) | Versioned application archive and SHA-256 |
| 07 | [`07_inno_setup_foundation_two_installation_modes_and_ui.md`](07_inno_setup_foundation_two_installation_modes_and_ui.md) | Inno Setup foundation: two installation modes and UI |
| 08 | [`08_github_version_locked_online_installer_and_secure_downloads.md`](08_github_version_locked_online_installer_and_secure_downloads.md) | GitHub version-locked Online Installer and secure downloads |
| 09 | [`09_implement_hybrid_msvc_runtime_deployment.md`](09_implement_hybrid_msvc_runtime_deployment.md) | Implement hybrid MSVC Runtime deployment |
| 10 | [`10_upgrades_reinstalls_downgrades_and_coexistence_safety.md`](10_upgrades_reinstalls_downgrades_and_coexistence_safety.md) | Upgrades, reinstalls, downgrades and coexistence safety |
| 11 | [`11_manual_closure_of_running_jobtracker_before_changes.md`](11_manual_closure_of_running_jobtracker_before_changes.md) | Manual closure of running JobTracker before changes |
| 12 | [`12_safe_uninstall_and_optional_deletion_of_personal_data.md`](12_safe_uninstall_and_optional_deletion_of_personal_data.md) | Safe uninstall and optional deletion of personal data |
| 13 | [`13_installer_integration_tests_and_clean_machine_verification.md`](13_installer_integration_tests_and_clean_machine_verification.md) | Installer integration tests and clean-machine verification |
| 14 | [`14_redistribution_license_audit_and_release_safeguards.md`](14_redistribution_license_audit_and_release_safeguards.md) | Redistribution/license audit and release safeguards |
| 15 | [`15_final_synchronization_of_codex_instructions_and_developer_docs.md`](15_final_synchronization_of_codex_instructions_and_developer_docs.md) | Final synchronization of Codex instructions and developer docs |
| 16 | [`16_independent_final_review_and_manual_github_release_handoff.md`](16_independent_final_review_and_manual_github_release_handoff.md) | Independent final review and manual GitHub release handoff |

## Persistent report layout

```text
JobTracker/
├── Tasks/
│   ├── Add_CV_task.txt                    # existing file, leave as is
│   └── Windows-Release/
│       ├── 00_RUN_NEXT.md                 # entry point for each chat
│       ├── 00_TASK_INDEX.md
│       ├── 00_REPORT_TEMPLATE.md
│       ├── 00_STATUS_TEMPLATE.md
│       ├── 00_AGREED_REQUIREMENTS.md
│       ├── 00_COVERAGE_MATRIX.md
│       ├── 01_....md
│       └── ... 16_....md
└── For-Agent/
    ├── Docs/                           # stable instructions, NOT task progress
    └── Task-Report/
        ├── JobTracker_Commercialization_Research.md
        └── windows-release/
            ├── STATUS.md                # created by task 01
            ├── 01.md                    # stage 01 report
            └── ... 16.md
```

The **correct existing project path** is `For-Agent/Task-Report/`, not `For-Agent/Docs/Task-Report/`. Keep completed reports in their dedicated `windows-release` subdirectory.

Read `00_RUN_NEXT.md` for selection, verification and handoff rules. Do not launch all tasks in one session.

## Special restrictions

- CI/CD, Linux packaging, automatic updates and code signing remain deferred.
- Qt 6.10.3 is required for **both Windows and Linux**, although packaging is Windows-first.
- No unauthorised Git commits, pushes, GitHub publications, deletions or application business-logic changes.
- Status reports must report real actions and observed evidence, not just plans.

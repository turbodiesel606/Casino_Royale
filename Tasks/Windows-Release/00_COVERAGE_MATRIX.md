# Traceability: all sections of original 23-page prompt

Source: user-uploaded `Promt_deploy.docx`. The sections below are **not removed**; they are assigned to narrower, sequential tasks. Cross-cutting constraints apply to all task files.

| Original section | Covered by numbered task(s) |
|---|---|
| Role and objective; overall result | 01–16 |
| 1. Scope and restrictions | 01–16 |
| 2. Inspect project first | 01, 02, 04, 12, 15, 16 |
| 3. Toolchain requirements | 01, 02, 03, 04, 07, 09, 13 |
| 4. Refactor CMake build/deployment | 02, 04 |
| 5. CMake presets | 03 |
| 6. Output artifacts: archive and installer | 05, 06, 07, 08, 16 |
| 7. GitHub Releases and secure downloads | 08, 13, 16 |
| 8. Installation modes and coexistence | 07, 10, 12, 13 |
| 9. Upgrade/reinstall/downgrade rules | 10, 13 |
| 10. Running application handling | 11, 13 |
| 11. Hybrid MSVC Runtime | 04, 09, 13, 14 |
| 12. EN/RU installer UI | 07, 08, 11, 12, 13 |
| 13. Destination and shortcuts | 07, 10, 13 |
| 14. Temporary icon and EXE metadata | 05, 07 |
| 15. SQLite/CV user data safety | 10, 11, 12, 13 |
| 16. Signing deferred; future extensibility | 05, 14, 16 |
| 17. Preserve Linux build; Qt 6.10.3 | 01, 02, 03, 15, 16 |
| 18. Build/test scenarios | 02–14, 16 |
| 19. Developer release documentation | 15, 16 |
| 20. Deliverables | 02–16 |
| 21. Final report | 16 |
| 22. Mandatory documentation sync | 01 and 15–16 |
| 22.1 Preserve instruction architecture | 01, 15 |
| 22.2 Canonical For-Agent/Docs/build.md | 01, 02–04, 15 |
| 22.3 For-Agent/Docs/windows-release.md | 15 |
| 22.4 AGENTS.md task routing | 01, 15 |
| 22.5 cmake-build-debug skill | 15 |
| 22.6 windows-release-packaging skill | 15 |
| 22.7 Verification instructions | 13, 15 |
| 22.8 README and legacy BuildCommands/qt_bins | 15 |
| 22.9 .gitignore and local Qt config | 03, 15 |
| 22.10 Related architecture / historical files | 01, 15 |
| 22.11 Redistribution / notices | 14, 15 |
| 22.12 Final documentation check | 15, 16 |
| 22.13 Agent documentation report | 15, 16 |

## Explicit technical dependencies

- 02 → 03 → 04: build lifecycle before production presets before Qt Release staging.
- 04 → 06: stage Qt libraries before producing the application archive.
- 05 → 07: temporary icon and version metadata before installer UI.
- 06 → 07 → 08: first validate a local payload installer, then add network download and integrity verification.
- 04 and 08 → 09: reconcile Qt's default compiler-runtime copying with different MSVC runtime strategies for each install scope (see Qt 6.10.3 `NO_COMPILER_RUNTIME`).
- 07–09 → 10–12: scope/version/process/user data safety after baseline installer machinery exists.
- 02–12 → 13: end-to-end verification only when all features exist.
- 13 → 14 → 15 → 16: inspect actual payload licenses; sync docs after implementation; final audit and manual-release readiness.

## Special caveats

- An Online Installer is not proven by merely compiling an `.iss` script. GitHub asset and clean-machine acceptance testing are distinct.
- AppImage/.deb and GitHub Actions are **excluded**, not forgotten; they were explicitly deferred.
- No change to SQLite schema, CV data paths or controller behavior is authorized as a packaging shortcut.
- Stage 07 is a local-fixture installer only; it is deliberately not advertised as the final Online Installer.

**Workflow adaptation:** Stage definitions have not been removed or combined. Stage 01 bootstraps report tracking under `For-Agent/Task-Report/windows-release/`; every stage writes its own `NN.md`, and stage 15 performs final instructions synchronization.

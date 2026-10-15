# JobTracker Windows packaging — agreed decisions and stage map

These are the agreed requirements for the 16 task files in this directory. Read this when a specific requirement is relevant; do not mistake plans for implemented behavior. The execution entry point is `00_RUN_NEXT.md`.

## Agreed requirements, retained from Promt_deploy.docx

- Windows 10/11 x64, Visual Studio 2022/MSVC v143, C++20, **Qt 6.10.3 on both Windows and Linux**; Linux packaging is deferred.
- Keep existing source private; public binary GitHub Releases repository: https://github.com/turbodiesel606/Releases. Manual publication only; **no CI/CD**.
- Version-specific **Online Installer** (Inno Setup) with fixed asset URL (not `latest`), embedded SHA-256 and secure/fail-safe download/extraction; matching Windows Release application archive.
- Current-user and all-users installation **both available and allowed simultaneously**, independent installation/uninstall state; editable directory with correct defaults.
- Installer interface English (default) and Russian; Start Menu shortcut always, desktop shortcut optional, unchecked; replaceable temporary icon.
- Hybrid MSVC Runtime: App-Local DLLs for per-user installation; official Microsoft x64 redistributable for all-users if needed.
- Update through a new installer; newer and same-version reinstall allowed, older-over-newer prohibited; preserve previous install path and user data. No automatic updater.
- Ask the user to close JobTracker manually before replacing/removing files; never force-close.
- SQLite DB, CVs and settings are stored outside installation directory; data preserved by default. Optional deletion only with explicit consent and trustworthy path/ownership checks; never delete data other account or still used by other JobTracker installation.
- No code signing in this phase, but future SignTool integration path. No business logic refactor, migration changes, Windows-only CMake without guards, or speculative dependency manager.
- Build/verification evidence and production redistribution/license checks are mandatory. Do not claim clean-machine/GitHub end-to-end success unless performed.

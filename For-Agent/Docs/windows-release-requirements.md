# Windows Release Requirements

These are accepted requirements, not a claim that packaging is implemented.
Their source is `Tasks/Windows-Release/00_AGREED_REQUIREMENTS.md`. Execute one
stage through `Tasks/Windows-Release/00_RUN_NEXT.md`; implementation evidence and
current status live in `For-Agent/Task-Report/windows-release/STATUS.md` and its
linked reports. Canonical commands belong in `For-Agent/Docs/build.md`.

## Platform And Distribution

- Target Windows 10/11 x64 with Visual Studio 2022 / MSVC v143 and C++20.
- Require Qt 6.10.3 on both Windows and Linux. Preserve Linux compatibility;
  Linux packaging is deferred.
- Keep application source private. Publish binaries manually to
  [turbodiesel606/Releases](https://github.com/turbodiesel606/Releases).
  Release preparation does not authorize Git writes or GitHub publication.
- Produce a version-specific Inno Setup Online Installer and a matching Windows
  Release application archive. Use a fixed version asset URL, never `latest`,
  with embedded SHA-256 verification and secure, fail-safe download/extraction.
- Use a replaceable temporary application/installer icon and consistent version
  identity across application metadata, archive, and installer.

## Installation And Runtime

- Offer current-user and all-users modes. Both may coexist, with independent
  installation and uninstall state, editable destinations, and mode-appropriate
  default directories.
- Provide English by default and Russian in the installer. Always create a
  Start Menu shortcut; offer a desktop shortcut unchecked by default.
- Deploy the required Qt, QML, platform, and SQLite runtime dependencies.
- Use hybrid MSVC runtime deployment: App-Local DLLs for current-user installs;
  the official Microsoft x64 redistributable for all-users installs when needed.
- Allow upgrades and same-version reinstalls, preserve the previous install
  path and user data, and reject downgrades over a newer installed version.
- Require the user to close JobTracker manually before replacing or removing
  application files. Never force-close the application.

## User Data And Coexistence

- Keep SQLite databases, managed CVs, and any persisted settings outside the
  installation directory. Preserve personal data by default on upgrade,
  reinstall, and uninstall.
- Optional personal-data deletion requires explicit consent and trustworthy
  path and ownership checks. Never remove another account's data or data still
  used by another JobTracker installation. When safety cannot be established,
  preserve the data.
- Preserve the existing application storage identity. Current source sets the
  application name to `JobTracker` and resolves storage as
  `QStandardPaths::AppLocalDataLocation/Data`, containing `database.sqlite` and
  `Resumes/` (`src/app/App.cpp`, `src/storage/StoragePaths.cpp`). This location is
  independent of the install directory; two installations used by the same
  account can share it. Separate install records do not imply separate data.
- The stage-01 source audit found no `QSettings` or QML `Settings` use in `src/`
  or `qml/`. Do not invent a settings path or treat that search as an exhaustive
  inventory of future persisted data.

## Verification And Scope

- Preserve existing Debug development, Windows/Linux guards, production/test
  isolation, application behavior, and migration behavior.
- Require build/verification evidence and a redistribution/license audit of the
  final shipped dependency set. Source imports alone do not establish package
  readiness.
- Record `PASS`, `FAIL`, and `NOT_RUN` honestly. Clean-machine and GitHub download
  acceptance require actual runs; an existing build artifact is not proof.
- Defer CI/CD, Linux packaging, automatic updates, and code signing. Leave room
  for future SignTool integration without adding signing now.
- Do not introduce business-logic refactors, migration changes, unguarded
  Windows-only CMake behavior, or a speculative dependency manager.

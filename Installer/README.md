# JobTracker Installers

- **Windows 10/11 x64:** [Windows offline installer guide](../For-Agent/Docs/installer.md).
  `Stage-Release.ps1` stages the production Release build; `Build-Installer.ps1`
  verifies and embeds Qt/QML/SQLite and pinned local MSVC inputs in one offline
  Setup. Each invocation writes
  `Installer/output/<run-id>/JobTracker-Setup-<version>.exe`; private evidence
  stays under `build/`. See [Windows installer checks](../For-Agent/Docs/installer-testing.md).
- **Ubuntu 24.04 amd64:** [Linux runtime and DEB guide](Linux/README.md).
  Use the dedicated `linux-deb-local-test` preset, then invoke
  `bash Installer/Linux/Stage.sh ...` or `bash Installer/Linux/Package.sh ...`
  from the repository root with automatic fresh directories. Work/staging stays under
  `build/linux-deb/`; final DEBs go under `dist/linux-deb/<run-id>/`.
  See [Linux deployment checks](../tests/deployment/linux/README.md).

Both workflows consume an already built Release application and preserve old
outputs. Qt is private in both payloads. The Ubuntu DEB may still require
network access for missing declared system dependencies and uses placeholder
local-test release metadata.

[Build configuration](../For-Agent/Docs/build.md) owns common presets and local
SDK selection.

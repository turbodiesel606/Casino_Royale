# JobTracker Installers

- **Windows 10/11 x64:** [Windows offline installer guide](../For-Agent/Docs/installer.md).
  [Windows/Stage-Release.ps1](Windows/Stage-Release.ps1) stages the production
  Release build; [Windows/Build-Installer.ps1](Windows/Build-Installer.ps1)
  verifies and embeds Qt/QML/SQLite and pinned local MSVC inputs in one offline
  Setup. Each invocation writes
  `Installer/Windows/output/<run-id>/JobTracker-Setup-<version>.exe`; private evidence
  stays under `build/`. See [Windows installer checks](../For-Agent/Docs/installer-testing.md).
- **Ubuntu 24.04 amd64:** [Linux runtime and DEB guide](Linux/README.md).
  Choose `linux-deb-local-test` for placeholder engineering metadata or
  `linux-deb-release` for a confirmed release maintainer, then invoke
  `bash Installer/Linux/Package.sh ...` with automatic fresh directories.
  Separate runtime staging uses `linux-release` and
  `bash Installer/Linux/Stage.sh ...`; it is not a prerequisite for packaging.
  Run from the repository root. Work/staging stays under
  `build/linux-deb/`; final DEBs go under `Installer/Linux/output/<run-id>/`.
  See [Linux deployment checks](../tests/deployment/linux/README.md).

Both workflows consume an already built Release application and preserve old
outputs. Qt is private in both payloads. The Ubuntu DEB may still require
network access for missing declared system dependencies. Local-test packages use
placeholder metadata; release packages require explicitly supplied metadata.

[Shared build guidance](../For-Agent/Docs/build.md) owns action and local/shared
preset selection. Use the [Windows](../For-Agent/Docs/build-windows.md) or
[Linux](../For-Agent/Docs/build-linux.md) reference for the relevant OS's SDK,
command, and development-runtime details.

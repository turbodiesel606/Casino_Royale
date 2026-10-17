# Windows Offline Release Requirements

The binding, already approved specification is
[Tasks/Offline-Installer/00_REQUIREMENTS.md](../../Tasks/Offline-Installer/00_REQUIREMENTS.md).
It supersedes the historical 16-stage Windows release requirements for this
refactor. This page is a navigation summary, not a second specification.

- One self-contained native x64 offline Inno Setup EXE for Windows 10/11:
  `Installer/output/JobTracker-Setup-<version>.exe`. Direct local Release payload;
  no application archive/download requirement or installation-time network.
- CurrentUser uses verified App-Local MSVC CRTs without admin rights. AllUsers
  bundles the official locally verified x64 prerequisite and runs it only when
  required. Missing inputs fail; no silent downloading or lock refresh.
- Same-scope manual upgrades and repairs preserve data/location; reject downgrade
  and unsafe/ambiguous identity. Opposite scopes require manual uninstall
  preserving data, then reinstall. Never force-close JobTracker.
- Uninstall defaults to preservation. Only verified non-elevated CurrentUser
  removal can offer explicit optional cleanup; silent/unsafe cases preserve.
  AllUsers always preserves every profile's data.
- Windows/Linux Debug and Release production/test presets remain isolated.
  No test dependency in production, implicit packaging, CI/CD, automatic updater
  or Linux packaging.
- Root CMake owns version identity; signing is optional. Embedded QML resources
  are allowed; loose C++/build/test/PDB/internal documents do not ship.
- Never modify `src/`, unrelated QML, skills or workflows. No Git writes or
  publication. Licensing remediation is outside the refactor; preserve the
  [separate open issues](../Task-Report/offline-installer/LICENSE_ISSUES.md).

Enter through [00_RUN_NEXT](../../Tasks/Offline-Installer/00_RUN_NEXT.md), select
exactly one stage from [STATUS](../../Tasks/Offline-Installer/STATUS.md), write
its report, update STATUS last and stop. Commands live in [build.md](build.md);
implemented behavior in [windows-release.md](windows-release.md). Missing
runtime evidence remains NOT RUN; technical completion is not legal approval.

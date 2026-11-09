# Windows Installer Ownership

The Windows offline installer owns deployment and removal of program files. It
is separate from the C++/Qt/QML application and does not participate in startup,
schema validation/migration, CV storage, application concurrency, or recovery.
Use the [Windows installer guide](../installer.md) for user behavior and build
commands and [installer testing](../installer-testing.md) for verification.
Linux packaging retains its separate [runtime and DEB contract](../../../Installer/Linux/README.md).

## Components And Dependencies

| Component | Responsibility |
|---|---|
| `Installer/Windows/Create-Installer.ps1`, `Stage-Release.ps1` | Orchestrate the production Release build and stage a private payload through existing CMake deployment. |
| `PayloadValidation.ps1`, `Build-Installer.ps1` | Validate the exact payload/identity, snapshot local inputs, generate explicit file entries, compile one offline Setup, and retain build evidence. |
| `Prepare-MsvcRuntime.ps1`, `MsvcRuntime.iss` | Verify pinned Microsoft inputs and deliver runtime dependencies without relying on another copy's program folder. |
| `JobTracker.iss` | Compose the Inno Setup lifecycle, installation scope, local uninstaller, and localized UI. |
| `InstallationPolicy.iss`, `PayloadSafety.iss` | Enforce local identity/version/ownership/path boundaries and installer/uninstaller mutual exclusion. |
| `InstallOwnership.iss` | Maintain the local relative-file ledger, validate/reconstruct uninstall history, preserve unknown path collisions, and manage per-copy shortcuts. |
| `PersonalData.iss` | Resolve the intended Windows user, obtain explicit cleanup consent, and remove only that user's confirmed `Data` tree with independent failure reporting. |
| `tests/deployment/` | Consume installer contracts through fixtures, native harnesses, and opt-in integration; no production dependency on tests. |

## Local Ownership And Maintenance

Each installation is identified locally at its selected directory. There is no
JobTracker uninstall registry entry or global installed-copy catalog. Scope
chooses default destination and shell placement; separate directories may
coexist across scopes and arbitrary suitable local volumes. Disconnected other
copies and unloaded profiles are not installation prerequisites.

Local metadata and identifiable program files distinguish repair from adoption
of an unrelated nonempty folder. Same-directory repair/update retains valid
uninstall history. `JobTracker.install.ini` describes the local copy;
`JobTracker.ownership.ini` lists owned relative files under a `JobTracker-2`
identity/path/count header. Hash-verified local Inno history is appended.
Unverified, moved, or scope-changed history is retained with `.preserved-n`
suffixes before reconstructing current/available relative ownership. Unknown
payload-path collisions are likewise preserved, not silently overwritten.
Reconstruction after damage can manage only evidence-backed
files, not unavailable historical ownership. A reliably identified newer copy
blocks a same-directory downgrade, never an installation elsewhere.
Without an ownership ledger, uninstall recognizes only exact current-payload
hashes; changed unprovable files remain and produce an incomplete-removal result.

Installed folders cannot contain one another or overlap personal data. Canonical
local-path and reparse-point guards constrain operations. `CanonicalInstallPath`
resolves an existing ancestor without creating proposed folders, joins missing
components with one separator even at a drive root, and is idempotent. The
installation policy separately rejects a drive root as a program destination.
A moved/copied local
uninstaller cannot follow source-location records; repair at the new location
establishes new local bookkeeping. Per-copy shortcut identity and target checks
prevent removal/overwrite of another copy's or a retargeted shortcut.
Shortcut reuse also requires the correct user/common shell-folder location for
the selected scope. Same-directory scope transitions explicitly remove old
verified owned links and establish links in the new scope. Pending old locations
remain recorded across failed operations so retry/repair can finish them.
Desktop deselection on reinstall removes an owned link without replacing it;
selection creates or safely reuses one. A verified foreign/retargeted link is
preserved and released from this copy's records; unreadable/unverifiable links
remain untouched with a retained record and incomplete result. New intended
paths are recorded before creation. Start Menu and desktop reconciliation retain
their separate results, and a failed deletion must not erase its retry record.

Uninstall removes the selected copy's owned program files and metadata while
preserving unrelated files. Program-removal completion does not depend on an
empty directory and is not a prerequisite for the personal-data choice. Windows
replacement/deletion failures remain real operation failures, not evidence that
the application was closed or that an interrupted change was rolled back.
Program-root and ancestor handles pin filesystem paths during operations;
they do not enforce an application/profile idle state.

The `usUninstall` sequence first makes the data choice and performs confirmed
cleanup, then immediately attempts program removal. Any program-removal failure
logs its path/detail and aborts native uninstall before its uninstaller/log is
destroyed. Remaining local metadata and uninstall files support retry/repair.
Partial removal cannot produce complete native success, and the independent
data-cleanup result is not undone by a later program failure.

## Shared Personal Data Contract

The application resolves `%LOCALAPPDATA%\JobTracker\Data` per Windows account.
During install, repair, or update, Setup checks the destination against the
execution and target accounts' data paths. It opens each `Data` directory or
its nearest existing ancestor for file attributes to resolve the canonical
path. A failed path check blocks installation. Setup's access to personal data
ends with this directory check, including on cancellation or failure. Database
access, migration, and recovery belong to the application. Binaries copied
between machines do not transfer this data, and binaries launched by another
account retain existing per-user behavior. Separate installation folders do not
isolate databases or add cross-version/concurrent-access guarantees.

Interactive uninstall defaults to keeping data and asks at the start of program
removal. Confirmation names the intended account/SID/profile, exact `Data` path,
and executing account and explains the effect on all copies used by the target
user. An elevated uninstaller resolves the desktop shell's token and known
folders. If that identity is unavailable, a separate default-No confirmation
must explicitly select the displayed execution account before deletion can be
confirmed. Silent uninstall and declined/dismissed confirmation preserve data.

Confirmed cleanup starts immediately before program removal, regardless of running/remaining copies or
program-removal failures. There is no process poll, idle wait, application
communication, profile lock, or last-copy policy. Token/known-folder resolution
selects the intended account and exact Data boundary; per-object owner SIDs do
not decide deletion permission. Legitimate Administrators ownership of that
root or its children is permitted without trusting other paths, changing ACLs
or owners, or enabling privileges. Ordinary handle/path checks protect the exact
target; actual access/sharing failures are reported
separately with paths/error codes while other safe entries are attempted.
Missing `Data` is complete. The parent
`JobTracker` folder, other users, and original CV files outside `Data` remain
outside the cleanup boundary. No rollback or application-side response is
promised after deletion.

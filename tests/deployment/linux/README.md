# Linux package deployment checks

These standalone tools verify distribution, not application functionality.
They are never included by production CMake and are never shipped in the DEB.
Do not run the Windows deployment CMake project on Linux. Do not run CTest,
CRUD, CV imports, schema/migration scenarios or broad UI regression here.

Use a disposable Ubuntu 24.04 amd64 desktop with a disposable user, a real
GNOME Wayland session, XWayland and menu access. Record the image/release,
architecture, isolation, graphics/session details and all command exit codes.
The environment must have no access to the developer SDK, source/build mounts
or SDK linker configuration. System desktop Qt may exist; JobTracker must load
its own Qt/QML/plugins. Never install these packages on the developer host.
A cleared environment or extracted-package run there is preliminary evidence
only. Record unavailable environment capabilities separately from test failures.

## Launcher and provenance regressions

Run this standalone harness on the native Ubuntu 24.04 amd64 development host,
as an ordinary user. It requires Python 3.10+ (standard library), CMake/CPack 3.22+,
Ninja, Bash, `flock`, GNU core utilities and the existing Debian package tools.
It uses private source/build/SDK fixtures, the real production launcher, resolver,
path guards, result validator and provenance helper. A tiny CMake project replaces
application compilation; a CPack double creates a tiny real DEB with `dpkg-deb`.
It never installs, launches JobTracker, uses personal data or changes the real kit.
Production has no dependency on the harness or Python.

From the repository root, choose a fresh directory:

```bash
python3 tests/deployment/linux/Test-CreateInstaller.py --work build/linux-deb/launcher-regression-20261022
```

The harness exercises success/order, failures at each stage, nonzero exits and no
false PASS, flock contention, CLI/local/preset/environment precedence, explicit
invalid presets and automatic fallback, paired/unpaired builds, inherited/null
environment, spaces/Unicode, invalid results/paths, preservation, and provenance.
Paired cases distinguish null, empty and nonempty values, explicit/inherited
build overrides and disabled configure-environment inheritance. Conflicting
references to a removed parent variable are rejected before configure, including
references in the build directory, environment and native tool options.
Controlled stage failures and settings/input changes also compare final output
inventories and hashes: a rejected candidate must not publish a new DEB. Direct
`Package.sh` cases mutate source, executable and cache during the CPack double;
the private receipt helper is checked before its final package path exists.
All fixtures and command logs remain under the exact work path; `results.json`
records unittest totals and `commands.jsonl` records each command and exit. A fresh
run refuses an existing directory. Do not run these fixtures through sudo or bypass
native host checks. They do not validate Qt deployment, actual application DEBs,
desktop startup, installation or lifecycle acceptance. After these tests, perform
the separately authorized real native build/package checks below.

On Windows, only the platform-neutral CMake evidence helper can execute:

```powershell
python tests/deployment/linux/Test-CreateInstaller.py --portable --qt-root '<installed Qt 6.10.3 kit>' --work build/linux-provenance-check-20261022
```

This mode checks manifest stability/new/modified inputs without Git, receipt and
package hash links, secret exclusion, manual preset absence, mid-run changes and
old-record preservation. It uses a real local `qmake` for the version query and
synthetic build identity files. A PASS is not native Linux launcher/package evidence.
Bash `-n`, Python AST parsing and CMake/JSON structural checks have the same limit.

## Artifact selection and integrity

Build and package with the [release launcher](../../../Installer/Linux/README.md#one-command-release-deb)
or existing [manual Linux DEB workflow](../../../Installer/Linux/README.md#build-a-local-test-deb).
Use the exact `Package:` and `Package work:` paths printed by that successful
`Package.sh` invocation; its `paths.txt` records the work/output directories.
The launcher places timestamped DEBs directly under `Installer/Linux/output/`;
manual packaging defaults to `Installer/Linux/output/<run-id>/`. Private work,
`result.txt` (exact final filename/version/size/hash), and checksum evidence
remain under `build/linux-deb/`. The launcher also copies the result record to
its printed log directory and never runs these separate acceptance checks.
Do not select an arbitrary newest package or assume a versioned filename.
From the repository root, set these paths and verify the wrapper's exact-byte
SHA-256 manifest:

```bash
package_work=/absolute/path/from/Package-work
production_deb=/absolute/path/from/Package
sha256sum --check -- "$package_work/package.sha256"
production_sha256=$(sha256sum -- "$production_deb")
production_sha256=${production_sha256%% *}
```

Stop on a hash mismatch or failed command. Retain the selected paths and
`production_sha256` for fixture generation and the transfer procedure below.
No historical artifact names or hashes are required.

## Standalone QSQLITE probe

Configure and build on the development machine with the matching Qt 6.10.3 SDK,
using a fresh output directory. Example from the repository root:

```bash
cmake -S tests/deployment/linux -B build/linux-deb/my-verification/sql-probe-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QTDIR"
cmake --build build/linux-deb/my-verification/sql-probe-build --target JobTrackerSqlProbe --parallel 1
readelf -d build/linux-deb/my-verification/sql-probe-build/JobTrackerSqlProbe
```

The only RUNPATH must be `$ORIGIN/../lib`. The probe must run beside the
package's unchanged `bin/qt.conf`. It checks Qt 6.10.3, the expected canonical
prefix, plugin search paths, an in-memory QSQLITE `SELECT 42`, and actual
Qt/ICU/SQLite ELF mappings in `/proc/self/maps`. It does not construct `App`,
load application QML, create a user database or prove graphics availability.
It must fail on external Qt/plugin paths or an incorrect prefix. It changes
neither Qt search paths nor the packaged configuration to force success.

After installing the exact production DEB **in the disposable target only**,
place the separately transferred probe alongside the executable. This is an
unowned verification file, not a package payload change:

```bash
test ! -e /opt/jobtracker/bin/JobTrackerSqlProbe
sudo install -m 0755 ./JobTrackerSqlProbe /opt/jobtracker/bin/JobTrackerSqlProbe
env -i PATH=/usr/bin:/bin LANG=C.UTF-8 /opt/jobtracker/bin/JobTrackerSqlProbe /opt/jobtracker
sudo rm -- /opt/jobtracker/bin/JobTrackerSqlProbe
```

Record each exit and retain probe stdout/stderr; run the probe as the disposable
ordinary user, never with sudo. Remove only that newly copied probe before
package lifecycle comparisons. On the developer host, the same check may use
a **fresh extracted copy** of the exact DEB and an additional probe beside its
qt.conf. Preserve original package/staging evidence. This is not installation
or clean-target validation.

## Fixture generation

Prefer a known compatible older package when available. For the first DEB,
generate two explicit disposable revisions without changing `project(VERSION)`:

```bash
python3 tests/deployment/linux/MakeUpgradeFixtures.py "$production_deb" "$production_sha256" build/linux-deb/my-verification/upgrade-fixtures
```

The helper verifies the input SHA-256 and package/architecture, rejects root
execution and existing/outside output, then builds `<version>~verify1` and
`<version>~verify2`. Both sort before the production version. It changes only
Version and the Description's first line, checks payload bytes, symlinks,
ownership and modes against the original, and writes `fixtures.json` with
commands, exits, versions and hashes. It never installs or deletes anything;
partial output remains available after failure. Fixture packages stay under
`build/linux-deb`, outside the customer output. An upgrade between these
fixtures proves package replacement only, not historical data/schema compatibility.

## Transfer and verify artifacts

Set `fixture_a` and `fixture_b` to the exact paths in the generated
`fixtures.json`, in ascending version order. Compare `sha256sum` output for
both files with the hashes in that record. After building the probe and
generating the fixtures above, prepare a fresh transfer directory containing
only the selected artifacts and this guide, with a manifest using relative
filenames:

```bash
sha256sum -- "$fixture_a" "$fixture_b"
transfer_dir=$(mktemp -d "$PWD/build/linux-deb/transfer.XXXXXX")
cp -- "$production_deb" "$fixture_a" "$fixture_b" build/linux-deb/my-verification/sql-probe-build/JobTrackerSqlProbe tests/deployment/linux/README.md "$transfer_dir/"
(cd -- "$transfer_dir" && sha256sum -- *.deb JobTrackerSqlProbe README.md > SHA256SUMS && sha256sum --check -- SHA256SUMS)
```

Retain the source-side manifest and copy the transfer directory into the
disposable environment. From inside that copied directory, verify against the
same manifest before using any artifact; do not regenerate it on the target:

```bash
sha256sum --check -- SHA256SUMS
```

Stop on any missing file or mismatch. No SDK, compiler or test framework is
required on the target.

## Install, inspect and bounded startup

In the disposable environment, set `production_deb`, `fixture_a`, `fixture_b`
to the absolute paths of the transferred files verified above. Start with
JobTracker absent; preserve any unexpected pre-existing installation instead
of treating it as test data.

```bash
sudo apt-get install "$production_deb"
dpkg-query -W -f='${Status} ${Version} ${Architecture}\n' jobtracker
dpkg --verify jobtracker
dpkg-query -L jobtracker
desktop-file-validate /usr/share/applications/jobtracker.desktop
stat -c '%U:%G %a %n' /opt/jobtracker/bin/JobTrackerApp /usr/share/applications/jobtracker.desktop /usr/share/icons/hicolor/256x256/apps/jobtracker.png
```

Allow apt to resolve the declared dependencies. Record apt output, installed
version, missing/extraneous dependencies and trigger processing; do not fix a
missing Depends by manually installing unrelated libraries. Verify root:root,
0755 executable/directories, 0644 desktop/icons, all seven icons and unchanged
packaged content. If `desktop-file-validate` is unavailable, record that tool
gap separately; successful static validation on the developer host is reusable.

For direct launches, create temporary XDG directories as the disposable user:

```bash
verification_run=$(mktemp -d /tmp/jobtracker-deployment.XXXXXX)
mkdir -m 700 "$verification_run/data" "$verification_run/config" "$verification_run/cache" "$verification_run/state"
```

Use the actual real session's DISPLAY, WAYLAND_DISPLAY, XDG_RUNTIME_DIR and
DBus address. Do not replace XDG_RUNTIME_DIR with a fake compositor directory.
Run each backend sequentially, close its application before starting the next,
and retain logs and `/proc/<pid>/maps` while it is alive. Example native launch:

```bash
env -i PATH=/usr/bin:/bin LANG=C.UTF-8 DISPLAY="$DISPLAY" WAYLAND_DISPLAY="$WAYLAND_DISPLAY" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS" XDG_SESSION_TYPE=wayland XDG_CURRENT_DESKTOP=ubuntu:GNOME XDG_DATA_HOME="$verification_run/data" XDG_CONFIG_HOME="$verification_run/config" XDG_CACHE_HOME="$verification_run/cache" XDG_STATE_HOME="$verification_run/state" QT_QPA_PLATFORM=wayland QT_DEBUG_PLUGINS=1 QML_IMPORT_TRACE=1 /opt/jobtracker/bin/JobTrackerApp > "$verification_run/wayland.log" 2>&1 &
application_pid=$!
```

Once the main window is visible, record that observation (or screenshot) and
capture `cat /proc/"$application_pid"/maps > "$verification_run/wayland.maps"`.
Close the window normally, run `wait "$application_pid"`, and record the exit.
Repeat with `QT_QPA_PLATFORM=xcb`, `xcb.log` and `xcb.maps` in the same real
Wayland session. Label this **XCB through XWayland**, not native Xorg. Confirm
the matching platform plugin loaded, the main QML window appeared, and logs
have no missing library/import/plugin errors. Inspect all Qt, ICU, QML and
plugin mappings for `/opt/jobtracker` provenance; system drivers/libraries
remain system-owned. No job/CV interaction is needed. Do not count process
survival, an offscreen window or a timeout exit as successful startup proof.

Separately, use GNOME's actual application menu to launch the installed
JobTracker entry, observe the main window and icon, record its PID/executable
and mappings, then close it. Use the disposable user's normal data directories
for this menu check. Do not edit the installed desktop file, substitute a
terminal `gio launch` for the menu check, or run the GUI as root. Verify menu
and icon cache results after apt trigger processing.

## Reinstall, upgrade, remove and purge

Keep the application closed throughout these comparisons. After the startup
checks, create synthetic markers in the disposable user's actual expected
data location (`StoragePaths`: AppLocalDataLocation/Data, normally
`$XDG_DATA_HOME/JobTracker/Data`):

```bash
test -z "$(pgrep -u "$(id -u)" -x JobTrackerApp)"
marker_root="${XDG_DATA_HOME:-$HOME/.local/share}/JobTracker/Data"
mkdir -p "$marker_root/Resumes"
test ! -e "$marker_root/package-preservation.txt"
test ! -e "$marker_root/Resumes/package-preservation.txt"
printf 'synthetic package preservation marker\n' > "$marker_root/package-preservation.txt"
printf 'synthetic CV-directory marker; not a CV\n' > "$marker_root/Resumes/package-preservation.txt"
sha256sum "$marker_root/package-preservation.txt" "$marker_root/Resumes/package-preservation.txt" > "$verification_run/markers.sha256"
```

Stop on any failed command. Record each package-operation exit, installed
version/status, marker comparison, package-owned file comparison and menu/icon
observation. Run `sha256sum -c "$verification_run/markers.sha256"` after **each**
operation below. Never launch the application between marker comparisons.

1. `sudo apt-get install --reinstall "$production_deb"` — same-version reinstall.
2. `sudo apt-get remove jobtracker` — verify package-owned executable, desktop
   entry and seven icon files are absent, menu entry is gone, markers survive.
   Unowned files/directories need not be removed. Save the ownership list before
   removal; do not expect shared `/opt`, `/usr/share` or hicolor directories gone.
3. `sudo apt-get install "$fixture_a"` — record fixture A's exact version.
4. `sudo apt-get install "$fixture_b"` — a genuine ordered fixture upgrade;
   verify version advanced, owned files match and markers survive.
5. `sudo apt-get install "$production_deb"` — an ordered replacement restoring
   the exact production artifact; verify production version and file hashes.
6. `sudo apt-get purge jobtracker` — verify all package-owned application and
   desktop/icon files absent, package status purged/absent, menu entry gone and
   markers unchanged. No autoremove or unrelated package removal is required.

The package has no conffiles/custom maintainer scripts, so removal may already
leave no package-status record. Purge is nevertheless exercised separately
from remove, on the restored installed package. No home-directory cleanup,
downloads, root GUI launch or developer Qt mutation is permitted. Record any
failure before fixing the packaging inputs.

Required results: clean-target install/dependency resolution, main QML startup,
private loaded paths, QSQLITE, native Wayland, XCB through XWayland, installed
menu launch, reinstall, fixture upgrade, remove, purge and marker preservation.
Do not report success for unexecuted checks. Functional verification is outside
this deployment procedure.

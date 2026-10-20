#!/usr/bin/env bash
set -euo pipefail
umask 022

usage() {
    cat <<'EOF'
Create-Installer.sh [--preset name] [--qt-root kit] [--maintainer 'Name <email>']
  [--parallel jobs] [--config file] [--help]
Optional one-time setup: copy Create-Installer.example.conf to
Create-Installer.local.conf and fill the required overrides. CLI > local config
> existing preset/environment defaults. Requires native Ubuntu 24.04 amd64.
See Installer/Linux/README.md. Never installs or launches JobTracker.
EOF
}
declare -A overrides=()
config=''
while (( $# )); do
    case "$1" in
        --help|-h) usage; exit 0 ;;
        --preset|--qt-root|--maintainer|--parallel|--config)
            (( $# >= 2 )) && [[ -n $2 ]] || { printf 'Missing value for %s\n' "$1" >&2; exit 2; }
            case "$1" in
                --preset) overrides[INSTALLER_PRESET]=$2 ;;
                --qt-root) overrides[INSTALLER_QT_ROOT]=$2 ;;
                --maintainer) overrides[INSTALLER_MAINTAINER]=$2 ;;
                --parallel) overrides[INSTALLER_PARALLEL]=$2 ;;
                --config) config=$2 ;;
            esac
            shift 2 ;;
        *) printf 'Unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
source "$script_dir/Common.sh"
if [[ -n $config ]]; then
    [[ -f $config ]] || jobtracker_fail "Local configuration is missing: $config. Copy Create-Installer.example.conf and fill the required overrides."
else
    config="$script_dir/Create-Installer.local.conf"
fi
if [[ -f $config ]]; then source "$config"; fi
for key in "${!overrides[@]}"; do printf -v "$key" '%s' "${overrides[$key]}"; done
parallel=${INSTALLER_PARALLEL:-}
[[ -z $parallel || ( $parallel =~ ^[1-9][0-9]*$ && ${#parallel} -le 4 && $parallel -le 1024 ) ]] ||
    jobtracker_fail 'INSTALLER_PARALLEL/--parallel must be between 1 and 1024.'
for tool in cmake cpack ninja dpkg dpkg-deb dpkg-shlibdeps readelf file flock realpath mktemp sha256sum cmp ln tee; do
    command -v "$tool" > /dev/null || jobtracker_fail "Required local tool is missing: $tool. See Installer/Linux/README.md."
done
cd -- "$jobtracker_root"
run=$(jobtracker_reserve_directory "$jobtracker_root/build/installer-create" linux)
printf 'Launcher logs: %s\n' "$run"
step=resolve
printf '[resolve] Inspect Release preset, Qt SDK and maintainer\n'
trap 'status=$?; if (( status != 0 )); then printf "FAIL: installer creation during %s (exit %s). Logs: %s\n" "$step" "$status" "$run" | tee "$run/failure.log" >&2; fi' EXIT
export JOBTRACKER_CREATE_PRESET=${INSTALLER_PRESET:-}
export JOBTRACKER_CREATE_QT=${INSTALLER_QT_ROOT:-}
export JOBTRACKER_CREATE_MAINTAINER=${INSTALLER_MAINTAINER:-}
export JOBTRACKER_CREATE_RESULT="$run/preset.txt"
resolver="$jobtracker_root/Installer/Resolve-CreatePreset.cmake"
jobtracker_run_logged "$run/resolve.log" cmake -P "$resolver"
while IFS= read -r line; do
    key=${line%%=*}; value=${line#*=}
    case "$key" in
        preset) preset=$value ;; buildPreset) build_preset=$value ;; build) build=$value ;; qt) qt=$value ;;
        version) version=$value ;; maintainer) maintainer=$value ;;
        env.JOBTRACKER_CREATE_*|unset.JOBTRACKER_CREATE_*) jobtracker_fail 'Preset environment must not override reserved JOBTRACKER_CREATE_* launcher inputs.' ;;
        env.*|unset.*) ;;
        *) jobtracker_fail 'Invalid preset result record.' ;;
    esac
done < "$run/preset.txt"
jobtracker_check_path "$build/.jobtracker-installer.lock" "$jobtracker_root/build" > /dev/null
step=lock
mkdir -p -- "$build"
exec {lock_fd}>> "$build/.jobtracker-installer.lock"
flock -n "$lock_fd" || jobtracker_fail "Another installer launcher holds the lock for $build. Wait for it to finish."
# Inspect again while holding the lock; a concurrent earlier run may have
# configured the tree between resolution and lock acquisition.
jobtracker_run_logged "$run/resolve-locked.log" cmake -P "$resolver"
step=configure
printf '[configure] %s\n' "$preset"
jobtracker_run_logged "$run/configure.log" cmake --preset "$preset" \
    "-DCMAKE_PREFIX_PATH:PATH=$qt" '-DJOBTRACKER_BUILD_TESTS:BOOL=OFF' \
    '-DJOBTRACKER_ENABLE_DEB_PACKAGING:BOOL=ON' '-DJOBTRACKER_DEB_LOCAL_TEST:BOOL=OFF' \
    "-DJOBTRACKER_DEB_MAINTAINER:STRING=$maintainer"
jobtracker_run_logged "$run/verify-configured-cache.log" cmake -P "$resolver"
step=build
printf '[build] JobTrackerApp (Release, tests OFF)\n'
build_args=(--build "$build" --config Release --target JobTrackerApp)
if [[ -n $build_preset ]]; then build_args=(--build --preset "$build_preset" --config Release --target JobTrackerApp); fi
if [[ -n $parallel ]]; then build_args+=(--parallel "$parallel"); fi
(
    if [[ -z $build_preset ]]; then
        while IFS= read -r line; do
            case "$line" in
                env.*) export "${line#env.}" ;;
                unset.*) key=${line%%=*}; unset "${key#unset.}" ;;
            esac
        done < "$run/preset.txt"
    fi
    jobtracker_run_logged "$run/build.log" cmake "${build_args[@]}"
)
step=package
printf '[package] Release DEB through Package.sh\n'
# Package work follows the existing build/linux-deb boundary.
package_work=$(jobtracker_reserve_directory "$jobtracker_root/build/linux-deb" create)
jobtracker_run_logged "$run/package.log" bash "$script_dir/Package.sh" "$build" "$package_work/package" --timestamped-output
installer=''
while IFS= read -r line; do
    case "$line" in installer=*) installer=${line#*=} ;; esac
done < "$package_work/package/result.txt"
[[ -n $installer && -f $installer ]] || jobtracker_fail 'Packaging did not return its finished installer.'
cp -- "$package_work/package/result.txt" "$run/result.txt"
printf 'PASS: JobTracker %s\nInstaller: %s\nEvidence/checksum: %s/package\nLogs/result: %s\n' \
    "$version" "$installer" "$package_work" "$run"

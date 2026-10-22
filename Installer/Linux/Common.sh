#!/usr/bin/env bash
# Shared validation and fresh-directory handling for Stage.sh and Package.sh.
# Sourced only; neither application builds nor cleanup belong in these helpers.

jobtracker_fail() {
    printf '%s\n' "$*" >&2
    exit 2
}

jobtracker_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)
(( EUID != 0 )) || jobtracker_fail 'Run as an ordinary user, without sudo.'
if [[ $(uname -s) != Linux || $(uname -m) != x86_64 ]] ||
        ! grep -Fqx 'ID=ubuntu' /etc/os-release ||
        ! grep -Fqx 'VERSION_ID="24.04"' /etc/os-release; then
    jobtracker_fail 'This workflow requires native Ubuntu 24.04 amd64.'
fi

jobtracker_check_path() {
    local path=$1 allowed=$2 parent
    [[ -n $path ]] || jobtracker_fail 'An explicit path must not be empty.'
    path=$(realpath -ms -- "$path") || return
    case "$path" in
        "$allowed"/*) ;;
        *) jobtracker_fail "Path must be strictly below $allowed: $path" ;;
    esac
    parent=$path
    while [[ $parent != / ]]; do
        [[ ! -L $parent ]] || jobtracker_fail "Symlink path is not permitted: $parent"
        parent=$(dirname -- "$parent")
    done
    [[ $(realpath -m -- "$1") == "$path" ]] || jobtracker_fail "Symlink path is not permitted: $1"
    printf '%s\n' "$path"
}

jobtracker_check_new_path() {
    local path
    path=$(jobtracker_check_path "$1" "$2") || return
    [[ ! -e $path && ! -L $path ]] || jobtracker_fail "Output already exists; choose a fresh path: $path"
    printf '%s\n' "$path"
}

jobtracker_reserve_directory() {
    local allowed=$1 label=$2 path
    if (( $# == 3 )); then
        path=$(jobtracker_check_new_path "$3" "$allowed") || return
        mkdir -p -- "$(dirname -- "$path")" || return
        jobtracker_check_new_path "$path" "$allowed" > /dev/null || return
        mkdir -- "$path" || return
    else
        # Check ancestors before and after creating the allowed root. mktemp
        # atomically reserves a unique run, including simultaneous invocations.
        jobtracker_check_path "$allowed/reservation" "$allowed" > /dev/null || return
        mkdir -p -- "$allowed" || return
        jobtracker_check_path "$allowed/reservation" "$allowed" > /dev/null || return
        path=$(mktemp -d -- "$allowed/$label-$(date -u +%Y%m%dT%H%M%SZ).XXXXXX") || return
    fi
    printf '%s\n' "$path"
}

jobtracker_validate_build() {
    jobtracker_build=$(realpath -e -- "$1")
    jobtracker_cache="$jobtracker_build/CMakeCache.txt"
    local setting qt_dir qt_kit
    for setting in \
        "CMAKE_HOME_DIRECTORY:INTERNAL=$jobtracker_root" \
        'CMAKE_GENERATOR:INTERNAL=Ninja' \
        'CMAKE_BUILD_TYPE:STRING=Release' \
        'JOBTRACKER_BUILD_TESTS:BOOL=OFF' \
        'CMAKE_INSTALL_PREFIX:PATH=/opt/jobtracker' \
        'CMAKE_INSTALL_BINDIR:PATH=bin' \
        'CMAKE_INSTALL_LIBDIR:PATH=lib'; do
        grep -Fqx -- "$setting" "$jobtracker_cache" || jobtracker_fail "Unexpected build configuration; expected $setting"
    done
    qt_dir=$(sed -n 's/^Qt6_DIR:PATH=//p' "$jobtracker_cache")
    [[ -n $qt_dir ]] || jobtracker_fail 'Missing Qt6_DIR in the selected cache.'
    qt_kit=$(realpath -e -- "$qt_dir/../../..")
    [[ -x $qt_kit/bin/qmake && $("$qt_kit/bin/qmake" -query QT_VERSION) == 6.10.3 ]] ||
        jobtracker_fail 'The selected build must use the Qt 6.10.3 SDK.'
    [[ -x $jobtracker_build/JobTrackerApp ]] || jobtracker_fail 'Build the production Release executable first.'
}

jobtracker_run_logged() {
    local log=$1
    shift
    printf 'Command: ' > "$log"
    printf '%q ' "$@" >> "$log"
    printf '\n' >> "$log"
    if "$@" 2>&1 | tee -a "$log"; then
        printf 'Exit code: 0\n' | tee -a "$log"
    else
        local statuses=("${PIPESTATUS[@]}")
        printf 'Command exit code: %s; log writer exit code: %s\n' "${statuses[0]}" "${statuses[1]}" >&2
        printf 'Command exit code: %s; log writer exit code: %s\n' "${statuses[0]}" "${statuses[1]}" >> "$log"
        if (( statuses[0] != 0 )); then return "${statuses[0]}"; fi
        return "${statuses[1]}"
    fi
}

# Validate the exact invocation's record, including its immutable input receipt.
# Merely finding an old DEB at an arbitrary returned path is not success.
jobtracker_validate_result() {
    local record=$1 work=$2 expected_version=${3:-} line key value hash size
    local -A fields=()
    [[ -f $record && ! -L $record ]] || jobtracker_fail 'Missing package result record.'
    while IFS= read -r line || [[ -n $line ]]; do
        [[ $line == *=* ]] || jobtracker_fail 'Malformed package result record.'
        key=${line%%=*}; value=${line#*=}
        case "$key" in
            version|packageVersion|installer|size|sha256|evidence|log|provenance|provenanceSha256) ;;
            *) jobtracker_fail "Unknown package result field: $key" ;;
        esac
        [[ ! -v fields[$key] && -n $value ]] || jobtracker_fail "Duplicate/empty package result field: $key"
        fields[$key]=$value
    done < "$record"
    for key in version packageVersion installer size sha256 evidence log provenance provenanceSha256; do
        [[ -v fields[$key] ]] || jobtracker_fail "Missing package result field: $key"
    done
    [[ ${fields[evidence]} == "$work" && ${fields[log]} == "$work/cpack.log" &&
       ${fields[provenance]} == "$work/provenance.json" ]] || jobtracker_fail 'Stale or foreign package evidence.'
    [[ -z $expected_version || ${fields[version]} == "$expected_version" ]] || jobtracker_fail 'Unexpected application version.'
    [[ ${fields[packageVersion]} == "${fields[version]}-"* &&
       ${fields[sha256]} =~ ^[0-9a-f]{64}$ && ${fields[provenanceSha256]} =~ ^[0-9a-f]{64}$ &&
       ${fields[size]} =~ ^[1-9][0-9]*$ ]] || jobtracker_fail 'Invalid package identity/hash/size.'
    jobtracker_check_path "${fields[installer]}" "$jobtracker_root/Installer/Linux/output" > /dev/null
    [[ -f ${fields[installer]} ]] || jobtracker_fail 'Packaging did not return its finished installer.'
    hash=$(sha256sum -- "${fields[installer]}"); hash=${hash%% *}
    size=$(stat -c %s -- "${fields[installer]}")
    [[ $hash == "${fields[sha256]}" && $size == "${fields[size]}" ]] || jobtracker_fail 'Package result bytes do not match.'
    hash=$(sha256sum -- "${fields[provenance]}"); hash=${hash%% *}
    [[ $hash == "${fields[provenanceSha256]}" ]] || jobtracker_fail 'Package provenance hash does not match.'
    cmake -DMODE=verify "-DWORK=$work" "-DPACKAGE=${fields[installer]}" \
        "-DPACKAGE_VERSION=${fields[packageVersion]}" "-Dpackage_hash=${fields[sha256]}" \
        "-Dpackage_size=${fields[size]}" -P "$jobtracker_root/Installer/Linux/Provenance.cmake"
    jobtracker_installer=${fields[installer]}
}

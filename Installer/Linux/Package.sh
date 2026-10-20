#!/usr/bin/env bash
set -euo pipefail
umask 022

timestamped=false
arguments=()
for argument in "$@"; do
    case "$argument" in
        --timestamped-output) timestamped=true ;;
        --*) printf 'Unknown option: %s\n' "$argument" >&2; exit 2 ;;
        *) arguments+=("$argument") ;;
    esac
done
set -- "${arguments[@]}"
if (( $# < 1 || $# > 3 )) || { $timestamped && (( $# > 2 )); }; then
    printf 'Usage: %s <Release build> [new work root under build/linux-deb/] [new output under Installer/Linux/output/] [--timestamped-output (no output argument)]\n' "$0" >&2
    exit 2
fi
source "$(dirname -- "${BASH_SOURCE[0]}")/Common.sh"
jobtracker_validate_build "$1"
[[ $(dpkg --print-architecture) == amd64 ]] || jobtracker_fail 'DEB packaging requires amd64.'
grep -Fqx 'JOBTRACKER_ENABLE_DEB_PACKAGING:BOOL=ON' "$jobtracker_cache" ||
    jobtracker_fail 'Unexpected build configuration; expected JOBTRACKER_ENABLE_DEB_PACKAGING:BOOL=ON'
[[ -f $jobtracker_build/CPackConfig.cmake ]] || jobtracker_fail 'Configure the DEB preset before packaging.'

# Validate every explicit path before reserving any output.
if (( $# >= 2 )); then jobtracker_check_new_path "$2" "$jobtracker_root/build/linux-deb" > /dev/null; fi
if (( $# == 3 )); then jobtracker_check_new_path "$3" "$jobtracker_root/Installer/Linux/output" > /dev/null; fi
jobtracker_work=$(jobtracker_reserve_directory "$jobtracker_root/build/linux-deb" package "${@:2:1}")
if $timestamped; then
    jobtracker_output="$jobtracker_root/Installer/Linux/output"
    jobtracker_check_path "$jobtracker_output/reservation" "$jobtracker_output" > /dev/null
    mkdir -p -- "$jobtracker_output"
else
    jobtracker_output=$(jobtracker_reserve_directory "$jobtracker_root/Installer/Linux/output" package "${@:3:1}")
fi
jobtracker_log="$jobtracker_work/cpack.log"
printf 'Package work: %s\nPackage output: %s\nCPack log: %s\nDependency evidence: %s/dependency-check\n' \
    "$jobtracker_work" "$jobtracker_output" "$jobtracker_log" "$jobtracker_work" | tee "$jobtracker_work/paths.txt"
jobtracker_run_logged "$jobtracker_log" env -u LD_LIBRARY_PATH -u LD_PRELOAD -u DESTDIR LC_ALL=C \
    JOBTRACKER_DEB_WORK="$jobtracker_work" \
    cpack --config "$jobtracker_build/CPackConfig.cmake" -G DEB -C Release -B "$jobtracker_work/cpack"

shopt -s nullglob
jobtracker_packages=("$jobtracker_work"/cpack/*.deb)
(( ${#jobtracker_packages[@]} == 1 )) || jobtracker_fail 'Expected exactly one DEB in the new CPack output.'
jobtracker_deb=${jobtracker_packages[0]}
jobtracker_package_version=$(dpkg-deb --field "$jobtracker_deb" Version)
jobtracker_arch=$(dpkg-deb --field "$jobtracker_deb" Architecture)
[[ $(dpkg-deb --field "$jobtracker_deb" Package) == jobtracker && $jobtracker_arch == amd64 &&
    $jobtracker_package_version =~ ^[0-9]+\.[0-9]+\.[0-9]+-[0-9][A-Za-z0-9.+]*(~localtest)?$ ]] ||
    jobtracker_fail 'Unexpected DEB package identity/version.'
jobtracker_name="jobtracker_${jobtracker_package_version}_${jobtracker_arch}.deb"
[[ $(basename -- "$jobtracker_deb") == "$jobtracker_name" ]] || jobtracker_fail 'Unexpected CPack DEB filename.'
if $timestamped; then
    IFS= read -r jobtracker_id < /proc/sys/kernel/random/uuid
    jobtracker_id=${jobtracker_id//-/}
    jobtracker_name="jobtracker_${jobtracker_package_version}_${jobtracker_arch}_$(date -u +%Y%m%dT%H%M%SZ)_${jobtracker_id:0:12}.deb"
fi
jobtracker_final="$jobtracker_output/$jobtracker_name"
if $timestamped; then
    # Publish complete, validated bytes atomically, without overwriting any entry.
    ln -T -- "$jobtracker_deb" "$jobtracker_final" ||
        jobtracker_fail "Cannot publish $jobtracker_final without overwrite. Check collisions/permissions and keep build and output on the same filesystem. CPack evidence: $jobtracker_work"
else
    # Preserve the legacy exclusive copy, including cross-filesystem outputs.
    (set -o noclobber; cat -- "$jobtracker_deb" > "$jobtracker_final")
fi
cmp -- "$jobtracker_deb" "$jobtracker_final"
sha256sum -- "$jobtracker_final" | tee "$jobtracker_work/package.sha256"
jobtracker_hash=$(sha256sum -- "$jobtracker_final")
jobtracker_hash=${jobtracker_hash%% *}
jobtracker_size=$(stat -c %s -- "$jobtracker_final")
printf 'version=%s\npackageVersion=%s\ninstaller=%s\nsize=%s\nsha256=%s\nevidence=%s\nlog=%s\n' \
    "${jobtracker_package_version%-*}" "$jobtracker_package_version" "$jobtracker_final" \
    "$jobtracker_size" "$jobtracker_hash" "$jobtracker_work" "$jobtracker_log" > "$jobtracker_work/result.txt"
printf 'Package: %s\nCPack and dependency evidence: %s\n' "$jobtracker_final" "$jobtracker_work"

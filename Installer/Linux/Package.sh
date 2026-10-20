#!/usr/bin/env bash
set -euo pipefail
umask 022

if (( $# < 1 || $# > 3 )); then
    printf 'Usage: %s <Release build> [new work root under build/linux-deb/] [new output under Installer/Linux/output/]\n' "$0" >&2
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
jobtracker_output=$(jobtracker_reserve_directory "$jobtracker_root/Installer/Linux/output" package "${@:3:1}")
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
jobtracker_final="$jobtracker_output/$(basename -- "$jobtracker_deb")"
# noclobber uses exclusive creation; a colliding destination fails, even if its
# bytes happen to match. Preserve partial output on write/disk failure.
(set -o noclobber; cat -- "$jobtracker_deb" > "$jobtracker_final")
cmp -- "$jobtracker_deb" "$jobtracker_final"
sha256sum -- "$jobtracker_final" | tee "$jobtracker_work/package.sha256"
printf 'Package: %s\nCPack and dependency evidence: %s\n' "$jobtracker_final" "$jobtracker_work"

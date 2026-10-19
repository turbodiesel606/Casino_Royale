#!/usr/bin/env bash
set -euo pipefail
umask 022

# Run after a successful production Release build. Never reuse or erase output.
if (( $# < 1 || $# > 2 )); then
    printf 'Usage: %s <Release build directory> [new stage root under build/linux-deb/]\n' "$0" >&2
    exit 2
fi
source "$(dirname -- "${BASH_SOURCE[0]}")/Common.sh"
jobtracker_validate_build "$1"
if (( $# == 2 )); then
    jobtracker_stage=$(jobtracker_check_new_path "$2" "$jobtracker_root/build/linux-deb")
fi
jobtracker_work=$(jobtracker_reserve_directory "$jobtracker_root/build/linux-deb" stage)
if (( $# == 1 )); then jobtracker_stage="$jobtracker_work/stage"; fi
jobtracker_stage=$(jobtracker_reserve_directory "$jobtracker_root/build/linux-deb" stage "$jobtracker_stage")
jobtracker_log="$jobtracker_work/install.log"
printf 'Stage root: %s\nPayload: %s/opt/jobtracker\nStage evidence: %s\nInstall log: %s\n' \
    "$jobtracker_stage" "$jobtracker_stage" "$jobtracker_work" "$jobtracker_log" | tee "$jobtracker_work/paths.txt"
jobtracker_run_logged "$jobtracker_log" env -u LD_LIBRARY_PATH -u LD_PRELOAD DESTDIR="$jobtracker_stage" \
    cmake --install "$jobtracker_build" --config Release --prefix /opt/jobtracker
printf 'PASS: Stage root: %s\n' "$jobtracker_stage"

#!/usr/bin/env python3
"""Create two disposable revisions of an identified DEB; never install anything."""

import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tarfile


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def payload_inventory(package):
    """Compare content, links, modes and ownership independently of compression."""
    process = subprocess.Popen(["dpkg-deb", "--fsys-tarfile", str(package)], stdout=subprocess.PIPE)
    entries = {}
    with tarfile.open(fileobj=process.stdout, mode="r|") as archive:
        for entry in archive:
            name = entry.name.removeprefix("./").rstrip("/")
            # dpkg-deb adds a harmless root-directory header that CPack omits.
            if name in ("", "."):
                if not entry.isdir() or entry.mode != 0o755 or entry.uid != 0 or entry.gid != 0:
                    raise RuntimeError("Unsafe archive root directory")
                continue
            content = None
            if entry.isfile():
                content = hashlib.file_digest(archive.extractfile(entry), "sha256").hexdigest()
            entries[name] = [entry.type.decode("ascii"), entry.mode, entry.uid, entry.gid,
                             entry.linkname, content]
    process.stdout.close()
    if process.wait() != 0:
        raise RuntimeError("dpkg-deb payload inspection failed")
    return entries


def main():
    if len(sys.argv) != 4:
        raise SystemExit("Usage: MakeUpgradeFixtures.py <deb> <expected-sha256> <fresh-build/linux-deb/output>")
    if os.getuid() == 0:
        raise SystemExit("Create fixtures as an ordinary user")
    repository = Path(__file__).resolve().parents[3]
    package = Path(sys.argv[1]).resolve(strict=True)
    requested_output = Path(sys.argv[3])
    output = requested_output.resolve()
    allowed = repository / "build/linux-deb"
    if (requested_output.exists() or requested_output.is_symlink() or
            not output.is_relative_to(allowed) or output == allowed):
        raise SystemExit("Output must be fresh and strictly beneath this repository's build/linux-deb")
    if not re.fullmatch(r"[0-9a-f]{64}", sys.argv[2]) or sha256(package) != sys.argv[2]:
        raise SystemExit("Production DEB SHA-256 mismatch")
    fields = subprocess.check_output(
        ["dpkg-deb", "--field", str(package), "Package", "Version", "Architecture"], text=True)
    metadata = dict(line.split(": ", 1) for line in fields.splitlines())
    if metadata["Package"] != "jobtracker" or metadata["Architecture"] != "amd64":
        raise SystemExit("Expected a JobTracker amd64 package")
    version = metadata["Version"]
    if "-" not in version:
        raise SystemExit("Expected a separate Debian packaging revision")
    versions = [version + "~verify1", version + "~verify2"]
    for lower, upper in [(versions[0], versions[1]), (versions[1], version)]:
        subprocess.run(["dpkg", "--compare-versions", lower, "lt", upper], check=True)
    inventory = payload_inventory(package)
    output.mkdir(parents=True)
    evidence = {"classification": "DISPOSABLE_PACKAGING_FIXTURES_NOT_RELEASES",
                "production_deb": str(package), "production_sha256": sys.argv[2],
                "production_version": version, "commands": [], "fixtures": [],
                "payload_unchanged": False, "package_operations": "NOT_RUN"}

    def run(command):
        result = subprocess.run(command, text=True, capture_output=True)
        evidence["commands"].append({"command": command, "exit_code": result.returncode,
                                     "stdout": result.stdout, "stderr": result.stderr})
        if result.returncode:
            raise RuntimeError(f"Command failed: {command}: {result.stderr}")

    try:
        tree = output / "tree"
        run(["dpkg-deb", "--raw-extract", str(package), str(tree)])
        control = tree / "DEBIAN/control"
        original = control.read_text()
        for fixture_version in versions:
            changed, count = re.subn(r"^Version: .+$", "Version: " + fixture_version,
                                     original, flags=re.MULTILINE)
            if count != 1:
                raise RuntimeError("Expected one Version field")
            changed, count = re.subn(r"^Description: .+$",
                "Description: Disposable JobTracker packaging fixture (NOT A RELEASE)",
                changed, flags=re.MULTILINE)
            if count != 1:
                raise RuntimeError("Expected one Description field")
            control.write_text(changed)
            fixture = output / f"jobtracker_{fixture_version}_amd64.deb"
            run(["dpkg-deb", "--build", "--root-owner-group", "-Zxz", "-z1", "--threads-max=1",
                 str(tree), str(fixture)])
            if payload_inventory(fixture) != inventory:
                raise RuntimeError("Fixture payload differs from the production artifact")
            evidence["fixtures"].append({"path": str(fixture), "version": fixture_version,
                                         "sha256": sha256(fixture), "size_bytes": fixture.stat().st_size})
        evidence["payload_unchanged"] = True
        evidence["version_order"] = versions + [version]
        evidence["production_unchanged"] = sha256(package) == sys.argv[2]
        if not evidence["production_unchanged"]:
            raise RuntimeError("Production DEB changed during fixture preparation")
    finally:
        (output / "fixtures.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(json.dumps(evidence, indent=2))


if __name__ == "__main__":
    main()

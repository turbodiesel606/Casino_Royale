#!/usr/bin/env python3
"""Verify an explicitly selected source handoff manifest without Git or writes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re


def excluded(relative, policy):
    return (any(part in policy["directory_names"] for part in relative.parts)
            or any(relative.as_posix() == prefix or relative.as_posix().startswith(prefix + "/")
                   for prefix in policy["relative_prefixes"])
            or relative.name in policy["file_names"]
            or any(relative.name.startswith(prefix) for prefix in policy["file_prefixes"])
            or relative.suffix in policy["file_suffixes"])


def project_inputs(root, roots, policy):
    paths = set()
    for name in roots:
        relative = Path(name)
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError(f"Unsafe input root: {name}")
        entry = root / relative
        if entry.is_symlink():
            raise ValueError(f"Unexpected input-root symlink: {name}")
        if entry.is_file():
            paths.add(relative.as_posix())
        elif entry.is_dir():
            for parent, directories, files in os.walk(entry, followlinks=False):
                parent = Path(parent)
                directories[:] = [d for d in directories if not excluded((parent / d).relative_to(root), policy)]
                for directory in directories:
                    if (parent / directory).is_symlink():
                        raise ValueError(f"Unexpected source symlink: {parent / directory}")
                for filename in files:
                    rel = (parent / filename).relative_to(root)
                    if not excluded(rel, policy): paths.add(rel.as_posix())
        else:
            raise ValueError(f"Missing input root: {name}")
    return sorted(paths)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    root = args.root.resolve()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    if manifest["schema_version"] != 1 or not manifest["inputs"]:
        parser.error("Unsupported or empty handoff manifest")
    expected = {}
    errors = []
    for item in manifest["inputs"]:
        name = item["path"]
        path = root / name
        if Path(name).is_absolute() or ".." in Path(name).parts or not path.resolve().is_relative_to(root):
            parser.error(f"Unsafe input path: {name}")
        if name in expected or not re.fullmatch("[0-9a-f]{64}", item["sha256"]):
            parser.error(f"Invalid or duplicate manifest entry: {name}")
        expected[name] = item["sha256"]
        if not path.is_file() or path.is_symlink() or sha256(path) != item["sha256"]:
            errors.append("Missing/changed input: " + name)
    actual = set(project_inputs(root, manifest["input_roots"], manifest["exclusions"]))
    errors.extend("Unexpected input: " + name for name in sorted(actual - expected.keys()))
    errors.extend("Excluded/missing manifest input: " + name for name in sorted(expected.keys() - actual))
    if errors:
        print("\n".join(errors))
        return 1
    print(f"PASS: {len(expected)} project input SHA-256 values and inventory match; Git not required.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

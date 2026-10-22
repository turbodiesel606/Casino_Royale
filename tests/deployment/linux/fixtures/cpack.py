#!/usr/bin/env python3
"""External CPack double: creates a tiny real DEB; never stages the application."""
import os
from pathlib import Path
import subprocess
import sys

if sys.argv[1:] == ["--version"]:
    print("cpack version 3.22.0 (test double)")
    sys.exit(0)
root = Path(os.environ["FIXTURE_ROOT"])
with (root / "trace.txt").open("a") as out:
    out.write("package\n")
if os.environ.get("FIXTURE_FAIL") == "package":
    print("Intentional fixture package failure")
    sys.exit(37)
output = Path(sys.argv[sys.argv.index("-B") + 1])
if os.environ.get("FIXTURE_MUTATE_STAGE") == "package":
    with Path(os.environ["FIXTURE_MUTATE_PATH"]).open("a") as out: out.write("\n ")
control = output / "fixture/DEBIAN/control"
control.parent.mkdir(parents=True)
control.write_text("Package: jobtracker\nVersion: 0.1.0-1\nArchitecture: amd64\n"
                   "Maintainer: Fixture Owner <fixture@jobtracker.test>\nDescription: Test only\n")
sys.exit(subprocess.run(["dpkg-deb", "--build", "--root-owner-group", str(control.parent.parent),
                         str(output / "jobtracker_0.1.0-1_amd64.deb")]).returncode)

"""Fault injection only in a private fixture, after the real Package.sh returns."""
import hashlib
import json
import os
from pathlib import Path
import sys

record = Path(sys.argv[1])
mode = sys.argv[2]
fields = dict(line.split("=", 1) for line in record.read_text().splitlines())
def injected():
    Path(os.environ["FIXTURE_ROOT"], "injection-complete.txt").write_text(mode)

if mode == "missing":
    record.rename(record.with_name("saved-result.txt"))
    injected()
    sys.exit(0)
if mode == "malformed":
    with record.open("a") as out: out.write("not-a-field\n")
    injected()
    sys.exit(0)
if mode == "missing-path": fields["installer"] += ".missing"
elif mode == "outside-path": fields["installer"] = str(record.parent / "outside.deb")
elif mode == "stale-package":
    previous = Path(os.environ["FIXTURE_ROOT"], "Installer/Linux/output/previous.deb")
    fields["installer"] = str(previous)
    fields["sha256"] = hashlib.sha256(previous.read_bytes()).hexdigest()
    fields["size"] = str(previous.stat().st_size)
elif mode == "stale": fields["evidence"] = str(record.parent / "previous-run")
elif mode == "hash": fields["sha256"] = "0" * 64
elif mode == "receipt":
    path = Path(fields["provenance"])
    receipt = json.loads(path.read_text())
    receipt["PACKAGE"] = "foreign package"
    path.write_text(json.dumps(receipt))
    fields["provenanceSha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
else: raise ValueError(mode)
record.write_text("".join(f"{k}={v}\n" for k, v in fields.items()))
injected()

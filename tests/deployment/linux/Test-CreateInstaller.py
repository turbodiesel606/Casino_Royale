#!/usr/bin/env python3
"""Native Ubuntu launcher regressions; --portable runs only provenance helpers.

All fixtures and evidence are retained under a new, caller-selected build path.
Production scripts/resolver are copied verbatim. Only expensive external work
and the tiny fixture application are doubles; host/security guards stay intact.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import unittest

REPOSITORY = Path(__file__).resolve().parents[3]
WORK = None
QT_ROOT = None
PORTABLE = False
CMAKE = shutil.which("cmake")


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write(path, text, executable=False):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")
    if executable:
        path.chmod(0o755)


class Fixture:
    def __init__(self, name):
        self.root = WORK / name / "Проект с пробелами"
        self.root.mkdir(parents=True)
        self.build = self.root / "build" / "Сборка с пробелами"
        self.calls = 0
        self.env = {k: v for k, v in os.environ.items()
                    if not k.startswith(("JOBTRACKER_", "INSTALLER_", "FIXTURE_"))
                    and k not in ("QTDIR", "CXX", "CC", "CMAKE_TOOLCHAIN_FILE")}
        self.env.update(FIXTURE_ROOT=str(self.root), FIXTURE_UNSET="parent-value",
                        JOBTRACKER_DEB_MAINTAINER="Fixture Owner <fixture@jobtracker.test>")
        for rel in ("Installer/Resolve-CreatePreset.cmake", "CMakePresets.json",
                    "Installer/Linux/Common.sh", "Installer/Linux/Package.sh",
                    "Installer/Linux/Create-Installer.sh", "Installer/Linux/Provenance.cmake"):
            dst = self.root / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(REPOSITORY / rel, dst)
        write(self.root / "src/input.cpp", "// Fixture source\n")
        write(self.root / "qml/Nested.qml", "import QtQuick\nQtObject {}\n")
        write(self.root / "CMakeLists.txt", (Path(__file__).parent / "fixtures/Application.cmake").read_text(encoding="utf-8"))
        shutil.copyfile(Path(__file__).parent / "fixtures/Build.cmake", self.root / "fixture-build.cmake")
        if QT_ROOT:
            self.qt = QT_ROOT
        else:
            self.qt = self.sdk("Qt SDK ü")
        self.env["QTDIR"] = str(self.qt)
        self.local = {"name": "linux-deb-release-local", "inherits": "fixture-parent",
                      "binaryDir": "${sourceDir}/build/Сборка с пробелами",
                      "cacheVariables": {"CMAKE_PREFIX_PATH": {"type": "PATH", "value": str(self.qt)}},
                      "environment": {"FIXTURE_UNSET": None,
                                      "FIXTURE_NESTED": "$env{FIXTURE_PARENT}/child"}}
        self.parent = {"name": "fixture-parent", "hidden": True, "inherits": "linux-deb-release",
                       "environment": {"FIXTURE_PARENT": "${sourceDir}/inherited"}}
        self.pair = {"name": self.local["name"], "configurePreset": self.local["name"],
                     "environment": {"FIXTURE_BUILD": "paired"}}
        self.build_parents = []
        self.presets()
        self.previous = {self.root / "Installer/Linux/output/previous.deb": b"previous-output",
                         self.root / "build/previous-cache/CMakeCache.txt": b"previous-cache"}
        for path, contents in self.previous.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(contents)
        if not PORTABLE:
            bin_dir = self.root / "fixture-bin"
            write(bin_dir / "cpack", (Path(__file__).parent / "fixtures/cpack.py").read_text(encoding="utf-8"), True)
            self.env["PATH"] = str(bin_dir) + os.pathsep + self.env["PATH"]

    def sdk(self, name, version="6.10.3"):
        kit = self.root / name
        (kit / "lib/cmake/Qt6").mkdir(parents=True)
        write(kit / "bin/qmake", f"#!/bin/sh\ncase \"$2\" in QT_VERSION) echo '{version}';; QMAKE_XSPEC) echo 'linux-g++';; *) exit 2;; esac\n", True)
        return kit

    def presets(self):
        write(self.root / "CMakeUserPresets.json", json.dumps(
            {"version": 3, "configurePresets": [self.parent, self.local],
             "buildPresets": self.build_parents + ([self.pair] if self.pair else [])}, ensure_ascii=False, indent=2))

    def run(self, *args, ok=True, env=None):
        self.calls += 1
        started = time.monotonic()
        result = subprocess.run([str(a) for a in args], cwd=self.root,
                                env=self.env | (env or {}), text=True, encoding="utf-8",
                                errors="replace", stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                timeout=90)
        log = self.root.parent / f"command-{self.calls:02d}.log"
        write(log, result.stdout)
        with (WORK / "commands.jsonl").open("a", encoding="utf-8") as out:
            out.write(json.dumps({"fixture": self.root.parent.name, "command": [str(a) for a in args],
                                  "exit": result.returncode, "seconds": round(time.monotonic() - started, 3),
                                  "log": str(log.relative_to(WORK))}, ensure_ascii=False) + "\n")
        if (result.returncode == 0) != ok:
            raise AssertionError(f"Unexpected exit {result.returncode}: {args}\n{result.stdout}")
        if not ok and "PASS: JobTracker" in result.stdout:
            raise AssertionError("False launcher PASS after failure")
        for path, contents in self.previous.items():
            if path.read_bytes() != contents:
                raise AssertionError(f"Previous artifact changed: {path}")
        return result

    def resolve(self, ok=True, **env):
        self.presets()
        result = self.root / "resolved.txt"
        run = self.run(CMAKE, "-P", self.root / "Installer/Resolve-CreatePreset.cmake", ok=ok,
                       env={"JOBTRACKER_CREATE_RESULT": str(result)} | env)
        return dict(line.split("=", 1) for line in result.read_text(encoding="utf-8").splitlines()) if ok else run.stdout

    def launch(self, *args, ok=True, **env):
        self.presets()
        return self.run("bash", self.root / "Installer/Linux/Create-Installer.sh", *args, ok=ok, env=env)

    def trace(self):
        path = self.root / "trace.txt"
        return path.read_text(encoding="utf-8").splitlines() if path.exists() else []

    def published_packages(self):
        return {p.relative_to(self.root).as_posix(): sha(p)
                for p in (self.root / "Installer/Linux/output").rglob("*.deb")}

    def package_work(self):
        records = list((self.root / "build/linux-deb").glob("create-*/package/result.txt"))
        if len(records) != 1:
            raise AssertionError(f"Expected exactly one invocation, got {records}")
        return records[0].parent

    def provenance(self, mode, work, ok=True, **values):
        return self.run(CMAKE, f"-DMODE={mode}", f"-DBUILD={self.build}", f"-DWORK={work}",
                        *(f"-D{k}={v}" for k, v in values.items()), "-P",
                        self.root / "Installer/Linux/Provenance.cmake", ok=ok)

    def fake_build(self):
        # Portable helper tests need identity files, not a native ELF/Qt application.
        version = subprocess.check_output([CMAKE, "--version"], text=True).split()[2]
        write(self.build / "CMakeCache.txt", f"""CMAKE_CACHE_MAJOR_VERSION:INTERNAL={version.split('.')[0]}
CMAKE_CACHE_MINOR_VERSION:INTERNAL={version.split('.')[1]}
CMAKE_CACHE_PATCH_VERSION:INTERNAL={version.split('.')[2]}
CMAKE_BUILD_TYPE:STRING=Release
Qt6_DIR:PATH={self.qt.as_posix()}/lib/cmake/Qt6
JOBTRACKER_BUILD_TESTS:BOOL=OFF
JOBTRACKER_ENABLE_DEB_PACKAGING:BOOL=ON
CMAKE_CXX_FLAGS:STRING=-DSECRET=UNIQUE_SECRET_VALUE
CMAKE_CXX_COMPILER_LAUNCHER:STRING=cmake;-E;env;TOKEN=UNIQUE_SECRET_VALUE;launcher
UNLISTED_SECRET:STRING=UNIQUE_SECRET_VALUE
""")
        write(self.build / f"CMakeFiles/{version}/CMakeCXXCompiler.cmake", """set(CMAKE_CXX_COMPILER "/fixture/compiler")
set(CMAKE_CXX_COMPILER_ID "Fixture")
set(CMAKE_CXX_COMPILER_VERSION "1.2.3")
""")
        for path in ("JobTrackerApp", "CPackConfig.cmake", "cmake_install.cmake", "build.ninja"):
            write(self.build / path, f"fixture {path}\n")


class ProvenanceTests(unittest.TestCase):
    def fixture(self):
        return Fixture(self._testMethodName)

    def test_seal_private_candidate_before_publication(self):
        f = self.fixture(); f.fake_build()
        work = f.root / "build/private-receipt"; work.mkdir()
        f.provenance("capture", work)
        candidate = work / "candidate.deb"; candidate.write_bytes(b"validated private candidate")
        package = f.root / "Installer/Linux/output/final.deb"
        f.provenance("seal", work, PACKAGE=package, PACKAGE_CANDIDATE=candidate, PACKAGE_VERSION="0.1.0-1")
        receipt = json.loads((work / "provenance.json").read_text(encoding="utf-8"))
        self.assertFalse(package.exists())
        self.assertEqual(receipt["PACKAGE"], str(package))
        self.assertEqual(receipt["package_hash"], sha(candidate))
        self.assertEqual(int(receipt["package_size"]), candidate.stat().st_size)
        shutil.copyfile(candidate, package)
        f.provenance("verify", work, PACKAGE=package, PACKAGE_VERSION="0.1.0-1",
                     package_hash=sha(package), package_size=package.stat().st_size)

    def test_manifest_stable_no_git_and_new_modified_inputs(self):
        f = self.fixture()
        def digest(name):
            out = f.root / "build" / (name + ".sha256")
            f.provenance("manifest", f.root, OUTPUT=out)
            return out.read_bytes()
        original = digest("original")
        self.assertFalse((f.root / ".git").exists())
        os.utime(f.root / "src/input.cpp", (1000000000, 1000000000))
        write(f.root / "build/generated/cache.txt", "ignored")
        write(f.root / "Installer/Linux/output/new.deb", "ignored")
        write(f.root / ".git/HEAD", "ignored git metadata")
        write(f.root / "Installer/Linux/Create-Installer.local.conf", "SECRET=UNIQUE_SECRET_VALUE")
        self.assertEqual(original, digest("unpacked"))
        write(f.root / "src/input.cpp", "local change\n")
        self.assertNotEqual(original, digest("modified"))
        write(f.root / "src/input.cpp", "// Fixture source\n")
        self.assertEqual(original, digest("restored"))
        write(f.root / "qml/new/Nested.qml", "import QtQuick\nQtObject {}\n")
        self.assertNotEqual(original, digest("added"))

    def test_receipt_integrity_secrets_manual_presets_and_preservation(self):
        f = self.fixture(); f.fake_build()
        work = f.root / "build/receipt-one"; work.mkdir()
        config = f.root / "Installer/Linux/Create-Installer.local.conf"
        write(config, "SECRET=UNIQUE_SECRET_VALUE\n")
        f.provenance("capture", work, LOCAL_CONFIG=config)
        inputs = json.loads((work / "inputs.json").read_text(encoding="utf-8"))
        self.assertEqual(inputs["configure_preset"], "")
        self.assertEqual(inputs["build_preset"], "")
        self.assertEqual(inputs["qt_version"], "6.10.3")
        self.assertNotIn("UNIQUE_SECRET_VALUE", (work / "inputs.json").read_text(encoding="utf-8"))
        package = f.root / "Installer/Linux/output/fixture.deb"; package.write_bytes(b"exact package")
        f.provenance("seal", work, PACKAGE=package, PACKAGE_VERSION="0.1.0-1", LOCAL_CONFIG=config)
        saved = {p: p.read_bytes() for p in work.iterdir()}
        def verify(ok=True):
            return f.provenance("verify", work, ok=ok, PACKAGE=package, PACKAGE_VERSION="0.1.0-1",
                                package_hash=sha(package), package_size=package.stat().st_size)
        verify()
        write(f.build / "CMakeCache.txt", "later reconfiguration")
        verify()  # Receipts do not consult a subsequently reused build tree.
        for path, contents in saved.items(): self.assertEqual(path.read_bytes(), contents)
        package.write_bytes(b"tampered package")
        verify(False)
        package.write_bytes(b"exact package")
        write(work / "inputs.json", "{}")
        verify(False)

    def test_source_build_cache_changes_and_existing_receipts_rejected(self):
        for changed in ("source", "binary", "cache"):
            with self.subTest(changed=changed):
                f = Fixture(self._testMethodName + "-" + changed); f.fake_build()
                work = f.root / "build/receipt"; work.mkdir()
                f.provenance("capture", work)
                f.provenance("capture", work, ok=False)
                target = {"source": f.root / "src/input.cpp", "binary": f.build / "JobTrackerApp",
                          "cache": f.build / "CMakeCache.txt"}[changed]
                write(target, "changed during package")
                package = f.root / "Installer/Linux/output/new.deb"; write(package, "new package")
                f.provenance("seal", work, ok=False, PACKAGE=package, PACKAGE_VERSION="0.1.0-1")
                self.assertFalse((work / "provenance.json").exists())

    def test_local_settings_changes_rejected_before_capture_and_seal(self):
        for key in ("preset", "config"):
            with self.subTest(key=key):
                f = Fixture(self._testMethodName + "-" + key); f.fake_build()
                config = f.root / "local.conf"; write(config, "# initial\n")
                settings = f.root / "build/settings.json"
                f.provenance("settings", f.root, OUTPUT=settings, LOCAL_CONFIG=config, SOURCE_CONFIG_HASH=sha(config))
                work = f.root / "build/receipt"; work.mkdir()
                f.provenance("capture", work, SETTINGS_MANIFEST=settings, LOCAL_CONFIG=config)
                changed = f.root / "CMakeUserPresets.json" if key == "preset" else config
                with changed.open("a") as out: out.write("\n ")
                next_work = f.root / "build/next-receipt"; next_work.mkdir()
                f.provenance("capture", next_work, ok=False, SETTINGS_MANIFEST=settings, LOCAL_CONFIG=config)
                package = f.root / "Installer/Linux/output/new.deb"; write(package, "new package")
                f.provenance("seal", work, ok=False, PACKAGE=package, PACKAGE_VERSION="0.1.0-1", LOCAL_CONFIG=config)


class NativeLauncherTests(unittest.TestCase):
    def fixture(self, suffix=""):
        return Fixture(self._testMethodName + suffix)

    def test_success_paired_build_and_provenance(self):
        f = self.fixture(); f.pair["nativeToolOptions"] = ["-v"]
        result = f.launch()
        self.assertIn("PASS: JobTracker", result.stdout)
        self.assertEqual(f.trace(), ["configure", "build", "package"])
        env = (f.build / "build-environment.txt").read_text(encoding="utf-8")
        self.assertIn("FIXTURE_BUILD=paired", env)
        self.assertIn("FIXTURE_UNSET=", env)
        self.assertIn("FIXTURE_UNSET_DEFINED=0", env)
        self.assertIn("FIXTURE_UNSET_DEFINED=0", (f.build / "configure-environment.txt").read_text())
        self.assertNotIn("parent-value", env)
        self.assertIn("-DROOT=", result.stdout)
        work = f.package_work()
        inputs = json.loads((work / "inputs.json").read_text(encoding="utf-8"))
        self.assertEqual(inputs["configure_preset"], "linux-deb-release-local")
        self.assertEqual(inputs["build_preset"], "linux-deb-release-local")

    def test_paired_environment_overrides_and_inheritance(self):
        cases = (("value", "build-value", 1), ("empty", "", 1), ("null", "", 0),
                 ("disabled", "parent-value", 1), ("parent-value", "build-parent", 1),
                 ("parent-null", "", 0), ("disabled-parent-null", "", 0),
                 ("first-parent-null", "", 0))
        for mode, expected, defined in cases:
            with self.subTest(mode=mode):
                f = self.fixture("-" + mode)
                if mode in ("value", "empty", "null"):
                    f.pair["environment"]["FIXTURE_UNSET"] = {"value": "build-value", "empty": "", "null": None}[mode]
                    if mode == "null": f.local["environment"]["FIXTURE_UNSET"] = "configured"
                if mode.startswith("disabled"):
                    f.pair["inheritConfigureEnvironment"] = False
                if "parent" in mode:
                    f.build_parents = [{"name": "build-parent", "hidden": True,
                                        "environment": {"FIXTURE_UNSET": "build-parent" if mode == "parent-value" else None}}]
                    f.pair["inherits"] = "build-parent"
                    if mode == "disabled-parent-null":
                        del f.pair["inheritConfigureEnvironment"]
                        f.build_parents[0]["inheritConfigureEnvironment"] = False
                    if mode == "parent-null": f.local["environment"]["FIXTURE_UNSET"] = "configured"
                    if mode == "first-parent-null":
                        del f.local["environment"]["FIXTURE_UNSET"]
                        f.build_parents.append({"name": "second-parent", "hidden": True,
                                                "environment": {"FIXTURE_UNSET": "second"}})
                        f.pair["inherits"] = ["build-parent", "second-parent"]
                f.launch()
                environment = (f.build / "build-environment.txt").read_text().splitlines()
                self.assertIn("FIXTURE_UNSET=" + expected, environment)
                self.assertIn("FIXTURE_UNSET_DEFINED=" + str(defined), environment)

    def test_null_macro_dependencies_rejected_before_configure(self):
        for location in ("binaryDir", "environment", "nativeToolOptions"):
            with self.subTest(location=location):
                f = self.fixture("-" + location)
                f.local["environment"]["FIXTURE_UNSET"] = "configured"
                f.pair["environment"]["FIXTURE_UNSET"] = None
                if location == "binaryDir":
                    f.local["binaryDir"] = "${sourceDir}/build/$penv{FIXTURE_UNSET}"
                elif location == "environment":
                    f.pair["environment"]["COPY"] = "$penv{FIXTURE_UNSET}"
                else:
                    f.pair["nativeToolOptions"] = ["-d", "$penv{FIXTURE_UNSET}"]
                previous = f.published_packages()
                result = f.launch(ok=False)
                self.assertIn("Cannot unset FIXTURE_UNSET", result.stdout)
                self.assertEqual(f.trace(), [])
                self.assertEqual(previous, f.published_packages())

    def test_failures_stop_sequence_and_preserve_previous_outputs(self):
        for stage, trace in (("configure", ["configure"]), ("build", ["configure", "build"]),
                             ("package", ["configure", "build", "package"])):
            with self.subTest(stage=stage):
                f = self.fixture("-" + stage)
                previous = f.published_packages()
                result = f.launch(ok=False, FIXTURE_FAIL=stage)
                self.assertIn("FAIL: installer creation", result.stdout)
                self.assertIn("Intentional fixture " + stage + " failure", result.stdout)
                self.assertEqual(f.trace(), trace)
                self.assertFalse(list((f.root / "build/installer-create").glob("*/result.txt")))
                self.assertEqual(previous, f.published_packages())

    def test_flock_contention(self):
        import fcntl
        f = self.fixture(); f.build.mkdir(parents=True)
        with (f.build / ".jobtracker-installer.lock").open("w") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            result = f.launch(ok=False)
        self.assertIn("holds the lock", result.stdout)
        self.assertEqual(f.trace(), [])

    def test_precedence_cli_local_preset_environment(self):
        f = self.fixture()
        config = f.root / "Настройки с пробелами.conf"
        local_qt = f.sdk("local Qt"); cli_qt = f.sdk("CLI Qt")
        write(config, f"INSTALLER_QT_ROOT='{local_qt}'\nINSTALLER_MAINTAINER='Local Owner <local@jobtracker.test>'\n")
        f.launch("--config", config, "--qt-root", cli_qt, "--maintainer", "CLI Owner <cli@jobtracker.test>")
        inputs = json.loads((f.package_work() / "inputs.json").read_text(encoding="utf-8"))
        self.assertEqual(inputs["qt_kit"], str(cli_qt))
        self.assertEqual(inputs["cache"]["JOBTRACKER_DEB_MAINTAINER"], "CLI Owner <cli@jobtracker.test>")
        g = self.fixture("-local")
        config = g.root / "local.conf"
        kit = g.sdk("selected local Qt")
        write(config, f"INSTALLER_QT_ROOT='{kit}'\nINSTALLER_MAINTAINER='Local Owner <local@jobtracker.test>'\n")
        g.launch("--config", config)
        self.assertEqual(json.loads((g.package_work() / "inputs.json").read_text(encoding="utf-8"))["qt_kit"], str(kit))
        h = self.fixture("-preset")
        h.local["cacheVariables"]["JOBTRACKER_DEB_MAINTAINER"] = "Preset Owner <preset@jobtracker.test>"
        self.assertEqual(h.resolve()["maintainer"], "Preset Owner <preset@jobtracker.test>")

    def test_explicit_invalid_and_automatic_fallback(self):
        f = self.fixture()
        f.local["cacheVariables"]["CMAKE_PREFIX_PATH"] = str(f.root / "missing-sdk")
        result = f.resolve()
        self.assertEqual(result["preset"], "linux-deb-release")
        self.assertIn("Missing Qt SDK", f.resolve(False, JOBTRACKER_CREATE_PRESET=f.local["name"]))
        self.assertIn("does not exist", f.resolve(False, JOBTRACKER_CREATE_PRESET="missing-preset"))
        f.launch("--preset", "missing-preset", ok=False)
        self.assertEqual(f.trace(), [])
        f.launch()
        self.assertEqual(f.trace(), ["configure", "build", "package"])
        self.assertEqual(json.loads((f.package_work() / "inputs.json").read_text(encoding="utf-8"))["configure_preset"],
                         "linux-deb-release")
        g = self.fixture("-wrong-version")
        g.local["cacheVariables"]["CMAKE_PREFIX_PATH"] = str(g.sdk("wrong SDK", "6.9.0"))
        self.assertEqual(g.resolve()["preset"], "linux-deb-release")

    def test_unpaired_inherited_environment_and_null(self):
        f = self.fixture(); f.pair = None
        resolved = f.resolve()
        self.assertEqual(resolved["buildPreset"], "")
        self.assertEqual(resolved["unset.FIXTURE_UNSET"], "")
        self.assertEqual(resolved["env.FIXTURE_NESTED"], str(f.root) + "/inherited/child")
        f.launch()
        environment = (f.build / "build-environment.txt").read_text(encoding="utf-8")
        self.assertIn("FIXTURE_NESTED=" + str(f.root) + "/inherited/child", environment)
        self.assertNotIn("parent-value", environment)
        self.assertIn("FIXTURE_UNSET_DEFINED=0", environment)
        inputs = json.loads((f.package_work() / "inputs.json").read_text(encoding="utf-8"))
        self.assertEqual(inputs["build_preset"], "")

    def test_incompatible_cache_and_wrong_build_pair(self):
        f = self.fixture(); f.build.mkdir(parents=True)
        cache = f.build / "CMakeCache.txt"
        write(cache, "CMAKE_HOME_DIRECTORY:INTERNAL=/foreign\nJOBTRACKER_BUILD_TESTS:BOOL=ON\n")
        original = cache.read_bytes()
        self.assertIn("Incompatible cache", f.launch(ok=False).stdout)
        self.assertEqual(original, cache.read_bytes()); self.assertEqual(f.trace(), [])
        g = self.fixture("-pair"); g.pair["configurePreset"] = "linux-debug"
        self.assertIn("must refer", g.resolve(False))

    def test_missing_malformed_stale_result_and_path(self):
        diagnostics = {"missing": "Missing package result record", "malformed": "Malformed package result record",
                       "missing-path": "Packaging did not return its finished installer",
                       "outside-path": "Path must be strictly below", "stale": "Stale or foreign package evidence",
                       "hash": "Package result bytes do not match", "receipt": "Provenance/package mismatch",
                       "stale-package": "Provenance/package mismatch: PACKAGE"}
        for mode, diagnostic in diagnostics.items():
            with self.subTest(mode=mode):
                f = self.fixture("-" + mode)
                original = f.root / "Installer/Linux/Package.sh"
                original.rename(original.with_name("RealPackage.sh"))
                write(original, (Path(__file__).parent / "fixtures/PackageResult.sh").read_text(encoding="utf-8"))
                shutil.copyfile(Path(__file__).parent / "fixtures/CorruptResult.py", f.root / "corrupt-result.py")
                result = f.launch(ok=False, FIXTURE_RESULT=mode)
                self.assertEqual((f.root / "injection-complete.txt").read_text(encoding="utf-8"), mode)
                self.assertIn(diagnostic, result.stdout)
                self.assertEqual(f.trace(), ["configure", "build", "package"])
                self.assertFalse(list((f.root / "build/installer-create").glob("*/result.txt")))

    def test_manual_package_preserves_receipts_and_rejects_pending_build(self):
        f = self.fixture(); f.launch()
        work = f.package_work()
        old = {p: p.read_bytes() for p in work.iterdir() if p.is_file()}
        manual = f.root / "build/linux-deb/manual"
        f.run("bash", f.root / "Installer/Linux/Package.sh", f.build, manual)
        inputs = json.loads((manual / "inputs.json").read_text(encoding="utf-8"))
        self.assertEqual(inputs["configure_preset"], "")
        self.assertEqual(inputs["build_preset"], "")
        # Force a real Ninja dependency to become dirty, then require rejection.
        write(f.root / "src/input.cpp", "changed source\n")
        f.run("bash", f.root / "Installer/Linux/Package.sh", f.build,
              f.root / "build/linux-deb/pending", ok=False)
        self.assertFalse((f.root / "build/linux-deb/pending/result.txt").exists())
        for path, contents in old.items(): self.assertEqual(path.read_bytes(), contents)

    def test_settings_mutated_during_build_or_package_are_rejected(self):
        for key in ("preset", "config"):
            for stage in ("build", "package"):
                with self.subTest(key=key, stage=stage):
                    f = self.fixture("-" + key + "-" + stage)
                    config = f.root / "local.conf"; write(config, "# fixture settings\n")
                    changed = f.root / "CMakeUserPresets.json" if key == "preset" else config
                    previous = f.published_packages()
                    result = f.launch("--config", config, ok=False, FIXTURE_MUTATE_STAGE=stage, FIXTURE_MUTATE_PATH=str(changed))
                    self.assertIn("Local/preset settings changed", result.stdout)
                    self.assertFalse(list((f.root / "build/installer-create").glob("*/result.txt")))
                    self.assertEqual(f.trace(), ["configure", "build"] + (["package"] if stage == "package" else []))
                    for pattern in ("*/package/result.txt", "*/package/provenance.json"):
                        self.assertFalse(list((f.root / "build/linux-deb").glob(pattern)))
                    self.assertEqual(previous, f.published_packages())

    def test_manual_package_input_mutations_do_not_publish(self):
        for changed in ("source", "binary", "cache"):
            with self.subTest(changed=changed):
                f = self.fixture("-" + changed)
                f.launch()
                previous = f.published_packages()
                target = {"source": f.root / "src/input.cpp", "binary": f.build / "JobTrackerApp",
                          "cache": f.build / "CMakeCache.txt"}[changed]
                work = f.root / "build/linux-deb/mutated"
                result = f.run("bash", f.root / "Installer/Linux/Package.sh", f.build, work,
                               ok=False, env={"FIXTURE_MUTATE_STAGE": "package",
                                              "FIXTURE_MUTATE_PATH": str(target)})
                self.assertEqual(f.trace(), ["configure", "build", "package", "package"])
                self.assertIn({"source": "Source inputs changed", "binary": "Build input changed",
                               "cache": "Build cache changed"}[changed], result.stdout)
                self.assertFalse((work / "result.txt").exists())
                self.assertFalse((work / "provenance.json").exists())
                self.assertEqual(previous, f.published_packages())


def main():
    global WORK, QT_ROOT, PORTABLE
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--portable", action="store_true", help="Only platform-neutral CMake provenance helpers")
    parser.add_argument("--qt-root", type=Path, help="Real Qt 6.10.3 kit for portable checks on Windows")
    args = parser.parse_args()
    PORTABLE = args.portable
    QT_ROOT = args.qt_root.resolve() if args.qt_root else None
    if not CMAKE: parser.error("cmake is required")
    if not PORTABLE:
        release = Path("/etc/os-release").read_text(encoding="utf-8") if Path("/etc/os-release").exists() else ""
        if sys.platform != "linux" or os.uname().machine != "x86_64" or os.geteuid() == 0 or \
                'ID=ubuntu\n' not in release or 'VERSION_ID="24.04"' not in release:
            parser.error("Native Ubuntu 24.04 amd64, ordinary user required; no host-guard bypass")
        for tool in ("bash", "ninja", "dpkg", "dpkg-deb", "dpkg-shlibdeps", "readelf", "file", "flock", "cpack"):
            if not shutil.which(tool): parser.error(f"Required native tool missing: {tool}")
    elif sys.platform == "win32" and not QT_ROOT:
        parser.error("Portable Windows checks require --qt-root pointing to Qt 6.10.3")
    WORK = args.work.absolute()
    if not WORK.resolve().is_relative_to((REPOSITORY / "build").resolve()) or WORK.exists():
        parser.error("Use a fresh work directory strictly below the repository build/")
    if any(p.is_symlink() for p in [WORK, *WORK.parents]): parser.error("Symlink work paths are not permitted")
    WORK.mkdir(parents=True)
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(ProvenanceTests)
    if not PORTABLE:
        suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(NativeLauncherTests))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    write(WORK / "results.json", json.dumps({"mode": "portable" if PORTABLE else "native-ubuntu",
          "tests": result.testsRun, "failures": len(result.failures), "errors": len(result.errors),
          "success": result.wasSuccessful()}, indent=2))
    print(f"Evidence: {WORK}")
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(main())

# Windows redistribution checklist and release hold

Audited: 2026-10-15, stage-13 JobTracker 0.1.0 Windows x64 candidates.
**Publication status: HOLD. Stage 14 completes an audit, not legal or release approval.**

This is maintainer guidance, not an EULA, source offer, customer notice bundle,
commercial-license grant, or automated publication gate. The owner must resolve
the items below before publishing. No commercial Qt entitlement is established.
The current application/installer bytes are preserved for evidence.

Exact inputs, commands and results are in [stage 14](../../For-Agent/Task-Report/windows-release/14.md).
The [complete inventory](../../For-Agent/Task-Report/windows-release/14-evidence/payload-inventory.md)
lists all 148 common files and ten installer-provided CRT DLLs. Its accompanying
JSON records hashes, SPDX identities, copyright statements, source commits,
build descriptions, signatures and a 144-record dependency/attribution graph.
Those internal artifacts must not be included in a public release.

## Publication blockers

| ID | Observed finding | Required resolution / evidence |
|---|---|---|
| R1 | `bin/Qt6Lottie.dll`, `bin/Qt6LottieVectorImageGenerator.dll`, `plugins/vectorimageformats/qlottievectorimage.dll` have `LicenseRef-Qt-Commercial OR GPL-3.0-only`, without an LGPL alternative. | For the planned proprietary/LGPL distribution, exclude the unnecessary Lottie deployment dependency, produce a fresh payload and verify functionality/closure, or establish another owner-approved license route. Do not silently select commercial terms or publish these candidates. |
| R2 | Common archive has no Qt LGPL/GPL texts, Qt copyright/usage notice, third-party notice bundle, source-access instructions or replacement instructions. Setup adds only `licenses/MSVC-NOTICE.txt`. | Deliver reviewed, version-specific notices and full license texts with both installation scopes and the downloadable payload. Extend the strict packaging path inventory deliberately; it currently permits only `bin`, `plugins`, `qml`, `translations`. Merely adding a repository file cannot fix delivery. |
| R3 | No distributor-controlled corresponding-source arrangement or modified-compatible-Qt replacement test is established. Most installed Qt binary SHA-1 values also differ from the SDK SBOM file hashes. | Retain the exact sources, Qt changes/build configuration, dependencies and fulfillment method; reconcile the binary/source evidence; document and demonstrate compatible replacement in a disposable environment. |
| R4 | `bin/icuuc.dll` is byte-identical to this host's `C:/Windows/System32/icuuc.dll`, signed by Microsoft Windows, absent from the Qt kit path. | Establish an explicit redistribution grant for this exact Windows binary or change deployment to a supported system/independently licensed dependency strategy and verify it. An ICU upstream license or Windows signature alone does not establish permission for this copy. |
| R5 | `opengl32sw.dll` and `D3Dcompiler_47.dll` match the Qt kit but lack a complete version-specific redistribution dossier; the installer engine has additional embedded components outside the Qt manifest. | Finish the Mesa/LLVM, Windows SDK and Inno/reduced-decoder provenance/notice review below. Record exact sources/terms; do not infer them from DLL names or signatures. |
| R6 | Actual executable contains recoverable private QML: 28 raw resource entries; `Main.qml` decompressed in memory and matched the project source hash. | Resolve the strict private-source requirement with an explicit owner policy decision or a separately implemented and verified resource policy. Discarding QML source has cache/runtime and compatible-Qt replacement implications; do not treat one build flag as proven protection or as a substitute for testing. |
| R7 | Publisher/ownership and applicable Visual Studio entitlement are not established by local files. | Owner confirms code/assets/contributor rights, intended terms, publisher identity, Visual Studio Community eligibility and redistribution rights. Any proprietary agreement must preserve third-party rights. Legal review resolves these questions; code cannot certify them. |

R1 is directly supported by the installed `qtlottie-6.10.3.spdx.json` and
[Qt 6.10.3 Lottie licensing](https://doc.qt.io/qt-6.10/qtlottieanimation-index.html#licenses).
It is not a claim that every Qt module is GPL-only.

## Actual dependency and notice worklist

| Distributed component | Evidence / intended review |
|---|---|
| 82 Qt DLLs/plugins, including QML plugins | 79 file records offer LGPLv3; the three Lottie files are R1. All 82 match installed Qt-kit SHA-256 values and have valid Qt signatures. Module versions are 6.10.3. |
| 29 QML `qmldir` files | Each matches the installed kit. License attribution is through its sibling plugin/module; these files are absent from the binary SBOM file list. Runtime `typeinfo`/`designersupported` entries are module metadata, not shipped project sources or `.qmltypes` tools. |
| 32 `qt_*.qm` catalogs | Qt translations SBOM offers BSD-3-Clause or commercial terms. Thirty-one differ from the SDK's individual `qt_*.qm`; deployment merges translations. Exact merged-input provenance is still required. |
| Qt Core and GUI embedded dependencies | Inventory identifies PCRE2 10.47, zlib 1.3.2, libpng 1.6.56, HarfBuzz 13.2.1, FreeType 2.14.3, Unicode data/CLDR and smaller components. Select FreeType's FTL alternative with its notices, not its GPL alternative. Preserve each applicable copyright, license and disclaimer; do not replace all permissive licenses with one generic MIT text. |
| Qt Network | Public Suffix List data `2026-01-16_13-11-47_UTC` is MPL-2.0; libpsl has BSD-3-Clause attribution. Include applicable notices and a source-availability route for MPL-covered data. Schannel/cert-only TLS plugins are shipped; no separate OpenSSL library is in this payload. |
| Image/SVG plugins | libjpeg-turbo 3.1.4 (IJG AND BSD-3-Clause), libtiff 4.7.1, libwebp 1.6.0, and xsvg attribution. Full expressions/owners/copyrights are retained in the inventory JSON. |
| `plugins/sqldrivers/qsqlite.dll` | Qt driver offers LGPLv3; bundled SQLite is 3.51.3, recorded as `blessing`. SQLite's own public-domain status does not replace the Qt driver's obligations. |
| `bin/opengl32sw.dll` | SHA-256 `b04de4541863bc7d8879040a78889c4849c1b1da2784c4630f734c146c2998ce`; valid Qt signature; strings identify Mesa 11.2.2 and LLVM 3.6.2. This is not a verified full source/build closure. Gather matching Mesa, LLVM and incorporated-component notices. Do not use modern LLVM Apache terms for this older version. |
| `bin/D3Dcompiler_47.dll` | Version 6.3.9600.16384; valid Microsoft signature; kit hash match. Retain the applicable SDK distributable-code terms for this copy; it is not covered by the MSVC CRT notice. |
| `bin/icuuc.dll` | Version 64.2.0.0; SHA-256 `e76201d65fc58d1db0ec99bc8e875f47d692bcaa16bb6448b197bc86c08d1816`; Windows-system provenance, R4. |
| MSVC App-Local | Ten x64 Release DLLs, version 14.44.35211.0, each matching the lock, both installers' inputs and Visual Studio `VC/Redist/MSVC/14.44.35112/x64/Microsoft.VC143.CRT`. All signatures valid; not a proof of distributor entitlement. |
| AllUsers VC++ prerequisite | Official Microsoft `VC_redist.x64.exe` 14.44.35211.0, 25,635,768 bytes, SHA-256 `cc0ff0eb1dc3f5188ae6300faef32bf5beeba4bdd6e8e445a9184072096b713b`; downloaded when required, not part of the common archive. Retained local copy has a valid Microsoft signature and matches the lock. No execution in this audit. |
| Inno Setup 6.7.1 runtime | Both Setup EXEs include loader/runtime/uninstaller functionality and Pascal Script/LZMA functionality. Preserve upstream notices; exact engine source/component closure remains R5. Compiler/IDE/Scintilla/compression build tools must not automatically be called shipped dependencies. |
| Reduced 7-Zip decoder | Both compile logs read/verify `is7zxr.dll`; current compiler-side file is 25.01, SHA-256 `728b368dc4f2831cc4d13291f46eeaf9f89b7e3a91356cf793c330e953b8e17b`, metadata says public domain. Exact version/source terms and correspondence with embedded bytes remain unverified; do not apply full 7-Zip licensing by filename. |
| JobTracker executable and generated install files | Project code, QML and temporary icon ownership require owner confirmation. `qt.conf`, `JobTracker.install.ini` and uninstall data are generated; they do not create a new third-party license. Windows-provided system DLLs are not blanket redistribution candidates. |

The SBOM dependency graph includes build/source attributions and `WrapAtomic:
NOASSERTION`; it is a conservative notice worklist, not 144 shipped libraries.
Raw SBOM SHA-1 agrees for only `qt_en.qm`; 113 of 114 directly mapped payload
files differ (translation entries have duplicate SBOM records). The 82 Qt binaries match the installed kit and validate as signed
Qt binaries, but post-build signing is only a possible explanation of that
checksum difference, not proven history. Thirty-one translation differences
also need merged-input evidence. Preserve these qualifications with the mapping.

Relevant primary material: [Qt SBOM](https://doc.qt.io/qt-6.10/sbom.html),
[SQLite copyright](https://www.sqlite.org/copyright.html),
[MPL 2.0](https://www.mozilla.org/en-US/MPL/2.0/),
[Qt software OpenGL](https://doc.qt.io/qt-6.10/windows-graphics.html#dynamically-loading-opengl),
[Mesa licensing](https://docs.mesa3d.org/license.html),
[LLVM 3.6.2 license](https://releases.llvm.org/3.6.2/LICENSE.TXT),
[Windows ICU](https://learn.microsoft.com/en-us/windows/win32/intl/international-components-for-unicode--icu-),
[Inno extraction modes](https://jrsoftware.org/ishelp/topic_setup_archiveextraction.htm)
and [Inno component credits](https://jrsoftware.org/ishelp/topic_credits.htm).
These sources identify obligations; they do not close the noted provenance gaps.

## LGPL, source and replacement checklist

- [ ] Select an applicable license for each component; remove R1 before assuming an LGPL-only Qt route.
- [ ] Supply a prominent Qt notice, applicable copyrights, and complete LGPLv3 and GPLv3 texts with the combined work.
- [ ] Preserve permission to modify the library and reverse engineer for debugging those modifications in any product terms.
- [ ] Maintain a suitable shared-library mechanism that runs with interface-compatible modified Qt; provide installation information where required.
- [ ] Establish a compliant corresponding-source delivery/offer method for the actual distributed libraries and modifications; keep its fulfillment under the distributor's control.
- [ ] Supply the separate third-party notices and any covered source, including the MPL data, with exact version provenance.
- [ ] Demonstrate replacement in both installation modes without touching real user data; record QML cache/Qt-version limitations, paths and required permissions.

The first four items follow [LGPLv3 sections 3–4](https://doc.qt.io/qt-6.10/lgpl.html).
Corresponding-source conveyance is governed by [GPLv3 section 6](https://doc.qt.io/qt-6.10/gpl.html)
as incorporated by LGPLv3; [Qt's obligations guidance](https://www.qt.io/development/open-source-lgpl-obligations)
also explicitly covers unmodified libraries. A bare upstream URL, SDK install,
dynamic-link observation or passing smoke test is not a completed source offer
or replacement test. This checklist is not legal advice or an owner approval.

Microsoft permits listed unmodified files subject to a valid Visual Studio
license and its terms. The publisher must establish eligibility and comply with
the applicable distributable-code conditions. Debug CRT is not an acceptable
substitute. See [Visual Studio 2022 redistribution](https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution)
and [C++ redistribution guidance](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files).
The existing `MSVC-NOTICE.txt` is attribution, not a grant of rights.

## Security, privacy and later signing

The verified Online candidate fixes the HTTPS `v0.1.0` asset URL and embeds the
archive hash; it verifies download size/hash before extraction and all 148 file
hashes before installation. Stage 14 reran 24 native checks successfully. There
is no mutable `latest` lookup or remotely trusted checksum manifest.

Public asset selection must be explicit: reviewed versioned archive, checksum,
public manifest and Setup only, plus reviewed customer legal/source materials.
**None of the current candidates is approved yet.** Never publish the whole
`dist/` directory, build metadata, generated `.iss`, logs, `work/`, `verify-*`,
internal audit JSON or private-source proof. No loose `.cpp`, `.hpp`, project
`.qml`, PDB, test, credential or developer files were in the 148-file inventory.
The embedded-source finding R6 remains a separate failure.

A limited raw ASCII/UTF-16 scan of 164 inputs found PEM delimiter literals in
two signed SDK-matching Qt TLS binaries, no complete PEM key blocks, and no
matches for the other selected token patterns. This is not exhaustive secret
detection, especially inside compressed resources. Vendor PDB path strings
exist in Qt/Mesa binaries; they are not private JobTracker source files.

The application and both Setup EXEs are unsigned. Payload hashes protect bytes
relative to the installer; they do not authenticate a replaced unsigned Setup.
Code signing remains deferred, with no certificate, key, publisher claim or
SmartScreen promise created. `JobTrackerIdentity.iss.in` retains the blank
publisher field consumed by `AppPublisher`; obtain the owner's approved identity
before filling it. Future SignTool integration must sign application bytes before
staging/archive hashing and sign Setup before its final published hash is
recorded. Rebuild all affected manifests/trust anchors after any byte change.
Do not add an execution-time Qt DLL hash lock that prevents compatible replacement.

The installed Inno license permits commercial applications. Its compiler banner
alone does not establish a commercial-use prohibition; the owner should review
procurement against the [Inno commercial-license FAQ](https://jrsoftware.org/isorder.php).

Clean-machine/GUI/AllUsers/historical-upgrade acceptance, successful public asset
download, and the earlier intermittent sharing error remain separate unresolved
items from stage 13. This audit does not change them. Stage 15 is documentation
work only until a new authorized implementation addresses the blockers.

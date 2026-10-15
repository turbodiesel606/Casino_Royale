# Current external and independent static evidence

Created: 2026-10-15 local time (Asia/Baku).

Two read-only agents independently examined (1) installer/data safety tasks07–12 and (2) licensing/documentation/handoff tasks14–16. Neither edited files or executed installers/tests. The lead reconciled their findings and performed the logged current checks. Relevant conclusions were checked against current implementation and artifact bytes; old DONE labels were not proof.

The complete reproducible current static collector is `Verify-StaticEvidence.ps1`, invoked as C19, exit 0. It retains `logs/19-static-evidence.json`:43 canonical PowerShell blocks parsed; zero parse errors, missing preset names or missing scripts; six referenced presets; 12 scripts; 43 preserved historical/agent inputs with no differences; 148 inventory/manifest paths equal; no delivered legal paths in the common payload; 10 runtime inputs match their vendor and inventory hashes; six Qt SPDX inputs match; three shipped Lottie hashes and license conclusions recorded; payload ICU matches the Windows-system file and is absent at the Qt-kit bin path.

The earlier reviewer observed those same comparisons, but two initial raw command exit codes were not retained. Those observations are not reported as exit 0. C19 supplies the explicit current process exit/log record instead. The earlier docs-parser command did retain exit 0. Static parsing does not validate every command's runtime behavior.

The installer reviewer recalculated both retained Setup hashes and compared all six current installer-source SHA-256s with stage 13 build metadata. The lead's `Inspect-Artifacts.ps1` / artifact snapshots retain the equivalent current record. Prior native results were read directly (Online25, runtime15, manual-close21, personal-data19, policy54); they are PREVIOUS_EVIDENCE, not new execution. Current Online24/package19/copied backend18+18/probe runs are separate.

Official sources verified on **2026-10-15**:
- [Qt 6.10 Lottie licenses](https://doc.qt.io/qt-6.10/qtlottieanimation-index.html#licenses): commercial/GPLv3 alternatives; no LGPL alternative for the three actual files. Their current installed SPDX conclusions agree.
- [Qt LGPL obligations](https://www.qt.io/development/open-source-lgpl-obligations): relevant notice, source and replacement obligations remain distinct work; this audit does not establish legal fulfillment.
- [Visual Studio2022 redistribution](https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution): redistribution depends on applicable licensing/terms; a binary signature is not proof of owner entitlement.
- [Qt QML resource contents option](https://doc.qt.io/qt-6.10/qt-add-qml-module.html#discard-qml-contents): removing source has cache/version compatibility implications. A possible CMake option is not a verified privacy/runtime solution.
- [Inno Setup license](https://jrsoftware.org/files/is/license.txt) and [official product page](https://jrsoftware.org/isinfo.php): current license permits commercial applications subject to its conditions; the product page requests commercial licenses. The fresh6.7.1 compiler banner says “Non-commercial use only.” Do not infer from that banner alone that the public license categorically forbids commercial use or that all exact shipped-component terms are settled. Owner/tool-use and embedded-component review remain R03/R06. The lead successfully read the official license after the reviewer's earlier retrieval failure.

Current public asset observation: C16 HEAD on the exact v0.1.0 archive URL returned404 at 10:44:25UTC (14:44:25Asia/Baku). C14 web retrieval returned cache miss, and C15 sandbox HEAD was denied socket access; neither earlier attempt establishes absence. The successful C16 HTTP request does. No public payload or installer was uploaded, and no production Setup was executed.

Current old-failure inspection: `rg -n -i --glob '*.log' 'error 32|error code 32|failed.*32|code.*32|sharing violation' dist/stage12-policy-tests dist/stage12-policy-tests-repeat dist/stage12-policy-stage11-control` identified preserved failed gates at version-0.0.9.log:258, repeat:314 and control repair.log:388. Later successes did not identify the cause.

Fresh build warnings retained: MSB8029 warns that temporary output may affect incremental builds (this was a deliberately isolated fresh audit tree); existing C4715 in StorageMutationGate.hpp; missing optional Vulkan headers at configure; missing dxcompiler.dll/dxil.dll at staging. No warning was suppressed or corrected.



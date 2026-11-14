# Dependencies

Required runtime: separately obtained Gothic II DNdR 2.6-fix/LAA (x86), Union 1.0m with its menu script, Ninja 4.1.16, appropriate x86 Visual C++ runtime and our own runtime files.

Optional examined profile: Quickloot 1.9.5 and Sprint 2.4.0 with compatibility bridge revision 1, Toolkit 16.0 and PatchMenu 1.1.1. Toolkit is conditionally required by those comfort modules in the examined Ninja 4 environment. The minimal core needs none of the four comfort archives. Exact hashes are in `config/dependencies.json`; unknown variants fail closed.

Build: MSVC v142, Windows 10 SDK, MSBuild, C++17, Python 3.12 and zenkit 1.3.0.4 for the own Ninja archive. External SDK reference: `https://gitlab.com/gothiccoop/gothiccoop.git`, commit `0255381eeb86ea4b888f420fafaa4a10ee4d85e3`. SDK and foreign binaries are not published here.

Primary sources: [Ninja](https://github.com/szapp/Ninja), [Toolkit](https://github.com/szapp/Toolkit), [Quickloot](https://github.com/Kirides/ninja-quickloot/releases/tag/v1.9.5), [Union archive](https://github.com/Gratt-5r2/gothic-fix-archive). Obtain Sprint/PatchMenu separately from their authors or the corresponding distribution channel. Version pins describe the examined profile, not the latest available release.

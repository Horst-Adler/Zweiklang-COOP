# Building from source

## Toolchain

Windows, MSVC v142, Windows 10 SDK, MSBuild, C++17, Release/Win32 and x86 Visual C++ runtime (`/MD`). The own localization builder uses standard Python. Building the own Ninja VDF additionally requires Python 3.12 with `zenkit==1.3.0.4`.

The repository does not include an SDK. The examined external Union/ZenGin definitions are from `https://gitlab.com/gothiccoop/gothiccoop.git`, commit `0255381eeb86ea4b888f420fafaa4a10ee4d85e3`. Supply `UnionAfx.h`, `.build_settings.inl`, G2A/UserAPI definitions, Union SDK v1.0l and DirectX 7 headers/import libraries separately. The foreign co-op gameplay implementation is not linked.

## Commands

```powershell
python -m pip install zenkit==1.3.0.4
.\scripts\Build-Alpha.ps1 -SdkRoot .\local\sdk\GothicCoop

# Standalone C++ tests without the external game SDK:
.\scripts\Build-Alpha.ps1 -TestsOnly
```

Use `-MSBuildPath <MSBuild.exe>` and `-PythonPath <python.exe>` when needed. The full build first validates/generates `LocalizationCatalogue.h`, then builds the own Ninja VDF/hash header, then the DLL and technical suite. Preserve this order.

Outputs: `build/alpha/G2NotR/OreboundCoop.dll`, `build/alpha/ninja/OreboundCoop_Core.vdf`, its manifest and `build/alpha/tests/OreboundNetworkTests.exe`. Run tests from the checkout root so synthetic fixtures resolve.

Default tests include interactive DE/US keyboard checks. `--headless` explicitly skips only that check for a service session; the normal interactive run remains required. Real loopback UDP and file recovery tests do not execute Gothic. A successful build/static script check is not native Ninja/Union parser or two-PC gameplay acceptance.

`scripts/Test-Tools.py` tests configuration preservation. `scripts/Audit-Publication.py --staged` verifies the complete explicit publication allowlist. Never stage original games, third-party archives, SDK binaries, real logs or website files.

Optional own main-menu artwork: install Pillow and pass `-MenuArtworkSource <your-image.png>`. The converter updates the own hash catalogue before the DLL build. See [artwork instructions](docs/MENU-ARTWORK.md). Without the optional archive, the native menu is retained.

# Orebound Co-op — own source, Alpha.34

Two-player Gothic II: Night of the Raven co-op plugin. Own network, protocol, world/combat/inventory adapters and diagnostics; externally supplied Union/ZenGin SDK declarations. The foreign GothicCoop gameplay implementation is not compiled into this plugin.

Current contract: protocol 11, Ninja ABI 2, journal 4, recovery 2, actor archive 1. English/German own UI is selected locally in the Union menu. Legacy script/save identifiers remain for compatibility.

Build with the checkout's `scripts/Build-Alpha.ps1`; see [BUILDING](../../BUILDING.md) in the public repository. MSVC v142, Win32/C++17 and an external SDK are required for the DLL. Standalone network/policy tests need no game SDK.

4,735 technical assertions pass; native menu/parser and shared gameplay acceptance remain open. No completed campaign or support for other original script profiles is claimed. The current narrow Cavalorn adapter targets the examined German scripts.

See the root README, current changelog, installation and localization documentation for current scope. Own code: AGPL-3.0-only. Original games, SDKs and optional third-party runtimes are obtained separately; their rights remain intact.

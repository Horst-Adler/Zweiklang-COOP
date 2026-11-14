# Installing Orebound Co-op Alpha.33

This is a developer build, not a completed campaign release. Close Gothic on both PCs and back up test saves before installing.

## Prepare and build

Use separately obtained Gothic II: Night of the Raven 2.6-fix/LAA, Union 1.0m, Ninja 4.1.16 and the matching x86 Visual C++ runtime. Retain your own `System/Autorun/Menu/zUnionMenu.d`, or supply it with `-UnionMenuScriptPath`. Follow [BUILDING.md](BUILDING.md).

The four own runtime files are:

```text
System/Autorun/OreboundCoop.dll
System/Autorun/OreboundCoop_Menu.d
System/Autorun/OreboundCoop_Npc.d
Data/OreboundCoop_Core.vdf
```

Settings are stored in `System/OreboundCoop.ini`. The Core VDF contains our Ninja scripts, not a custom start screen.

## Minimal installation

```powershell
python .\scripts\Configure-Qol.py "D:\Games\Gothic II" --loader-only
.\scripts\Install-Orebound.ps1 -GamePath "D:\Games\Gothic II" -WhatIf
.\scripts\Install-Orebound.ps1 -GamePath "D:\Games\Gothic II"
```

The loader-only command adds Ninja to the game's plugin list without changing graphics or creating comfort settings. New profiles default to English. Existing legacy profiles retain personal settings and default to German if they do not already declare a language. Use the Union menu to select English/German and save the selection. `-Language en` or `-Language de` explicitly selects a language during installation.

## Legacy migration

The installer backs up both old and new owned runtime files, saves, state and logs. It removes the exact old `ZweiklangCoop.dll`, two old Autorun scripts, old Core VDF and old INI from active paths after backing them up. Both DLL names must never be loaded together.

Existing `ZweiklangCoopState`, `ZweiklangLogs`, `ZWEIKLANG_RECOVERY.BIN`, `ZWEIKLANG_ACTORS.BIN` and legacy script symbols are deliberately retained when present. These are compatibility identifiers, not active product branding. Save/journal formats are unchanged. Existing native journal text remains historical text; new own entries use the selected language.

## Optional comfort modules

The minimal core requires none of the four comfort archives. Known Quickloot 1.9.5 and Sprint 2.4.0 require bridge revision 1; Toolkit 16.0 is a conditional dependency for those modules and PatchMenu 1.1.1. Obtain inputs separately and prepare local copies:

```powershell
python .\scripts\Prepare-QolCompatibility.py --input .\local\originals --output .\local\compatible --modules Quickloot
.\scripts\Install-Orebound.ps1 -GamePath "D:\Games\Gothic II" -QolDirectory .\local\compatible -OptionalModules Quickloot
```

Select `Quickloot,Sprint,PatchMenu` for the full examined profile. The installer includes required Toolkit in the selection. Retained optional files are validated; unknown versions are not enabled by a language or compatibility switch. Both peers need matching optional profiles and recalculated content IDs.

## Start, test and rollback

Start Gothic manually. Configure role, name, IP, port and friendly fire under Union & Plugins. Load a compatible world/save and wait for full readiness. F6 hosts, F7 joins, F8 disconnects. Saving has coordinated pause controls; loading/world changes still disconnect. Shared one-click campaign saving is not released.

Use new test slots. Begin with join/readiness, regular pickups, optional Quickloot separately, containers and sequential saving, then the Cavalorn route. [Test plan](docs/TESTPLAN-CAVALORN.md). English game-script compatibility and native language-switch/menu acceptance remain separate open tests.

Restore only a verified runtime manifest with `scripts/Restore-Runtime.ps1`; it restores approved runtime files without resetting saves. Keep matching save/recovery backups before trying older builds.

## Optional main-menu artwork

Alpha.34 can install the matching own artwork archive with `-MenuArtworkDirectory <folder>`. Existing source-root `build/alpha/menu` output is selected automatically. Missing optional artwork retains the original menu; unknown supplied/retained archives are rejected. See [menu artwork](docs/MENU-ARTWORK.md) for local building, disabling and visual acceptance.

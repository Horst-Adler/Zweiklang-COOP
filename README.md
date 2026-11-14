# Orebound Co-op

**A two-player Gothic II: Night of the Raven co-op project built around Union and Ninja.**

Updates preserve existing character profiles, saves and inventory journals.

**Current development build: 0.1.0-alpha.33 · protocol 11 · Ninja script ABI 2.** This is a developer alpha. Native two-PC acceptance is still open; a completed co-op campaign is not claimed.

[Install](INSTALLATION.md) · [Build](BUILDING.md) · [Roadmap](ROADMAP.md) · [Architecture](docs/ARCHITECTURE.md) · [Languages](docs/LOCALIZATION.md) · [Changelog](CHANGELOG.md) · [Deutsch](README.de.md)

## What the project provides

Each player runs a separate licensed Gothic II installation. One plugin supports either host or client role. Union owns the network, shared world authority, combat replication, inventory transactions, recovery and diagnostics. Our own Ninja module provides bounded script commands for merchant preparation, corpse inventory, quest XP and localized journal text.

- LAN connection and reconnect, partner movement and appearance.
- Shared NPC/combat state with separate local player health and a host-controlled friendly-fire rule.
- Ground items, corpses, containers, deposits, locks and NPC trade through confirmed inventory operations.
- Recovery journals/checkpoints, readiness diagnostics and shared test markers.
- A narrow first quest adapter for Cavalorn's bandits; general quest and campaign synchronization remains unfinished.
- Personal NPC/character state, special movement, transformations and supported summons, implemented with additional native tests still required.
- English/German plugin UI, selected locally through **Union & Plugins → Orebound Co-op → Language / Sprache**. Host and client can use different UI languages.

Technical implementation is distinguished from gameplay acceptance. Previous tests confirmed parts of connection, movement, combat, guest dialogue and player collision handling. Loot, saves and subsequent extensions need the current regression run.

## Requirements and optional modules

Supported development baseline: Gothic II DNdR 2.6-fix/LAA, Union 1.0m, Ninja 4.1.16 and the appropriate x86 Visual C++ runtime. Original games, Union, Ninja, SDKs and third-party archives are obtained separately.

The core does not require Quickloot, Sprint, Toolkit or PatchMenu. Known Quickloot 1.9.5/Sprint 2.4.0 variants require compatibility bridge revision 1. These comfort modules and PatchMenu 1.1.1 require Toolkit 16.0 in the examined Ninja 4 profile. Unknown variants are rejected. Both PCs currently need the same optional module profile and content fingerprint.

The UI language is a local option and does not alter the network fingerprint. Translating plugin text does not establish support for another original-game script profile. The current Cavalorn adapter is tied to the examined German script baseline; other editions require separate validation.

## Scope and limits

This is a separate-view, two-PC project. General shared quests, unrestricted client quest turn-in, complete combat XP sharing, coordinated world/chapter transitions and one-click campaign save/load are unfinished. Different worlds, Internet/NAT operation and dedicated servers are not released features.

Direct player trading, additional appearances, starter armor, pings and map notes are planned for beta. Polish/Russian plugin translations, co-op achievements, selected duo NPC reactions and a travel journal are planned for the full version. Odyssee and Gothic 1 are separate later projects.

## Source and distribution

Own code is AGPL-3.0-only; see [LICENSE](LICENSE) and [THIRD_PARTY.md](THIRD_PARTY.md). This repository contains own source, build/install tools, documentation and synthetic test fixtures. It does not contain original game data, external SDK binaries, transformed third-party mods, private diagnostics or the productive work website. No public binary release is assumed.

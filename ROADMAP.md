# Roadmap

Target: two-player Gothic II DNdR co-op. Current developer build: Alpha.33. Technical implementation and native gameplay acceptance are tracked separately; no fixed release date or campaign-wide compatibility promise.

## Alpha: reliable shared core

- Validate connection/reconnect, movement, shared combat, guest aggro, local health and friendly fire.
- Validate ground items, corpses, containers, deposits, locks, merchants and recovery without duplication or loss.
- Finish general shared quests and authorized turn-in by either player. Eligible quest rewards are 100% per player, not a 50/50 split. Faction quests remain separate unless both players qualify through the same faction.
- Define and implement general combat XP, special/quest item handling, doors/switches, companion/dialogue conflicts, time/sleep and defeat rules.
- Complete coordinated world/chapter transitions and campaign save/resume. Two activation areas in one world are implemented; unrestricted independent worlds are not.
- Accept personal NPC/crime state, local training/permanent bonuses, special movement, transformations and owned summons in separate native test cases.
- Introduce Orebound Co-op branding and English/German local UI. Public documentation is English-first; the productive work website stays German. Alpha.33 contains the technical foundation; native menu/parser and mixed-language acceptance remain open.
- Preserve Union as the authority and Ninja as the bounded script layer. Comfort modules remain optional with conditional dependencies and exact supported profiles.

Alpha exit: reproducible shared base workflows on both PCs, safe reconnect/resume and no unresolved critical crashes, duplicate/lost items or progression blockers.

## Beta: extended play and comfort

- Direct player dialogue and item/gold trade with mutual confirmation and authoritative atomic exchange.
- Two male and two female appearances; Velaya leather/cloth outfit.
- Starter armor with male/female visuals in Xardas' laboratory chest behind the fireplace-operated gate, one safe allocation per character.
- Partner pings and shared map notes.
- Revisit the custom Orebound Co-op start screen after resolving native menu/render ordering; earlier artwork experiments were withdrawn.
- Longer campaign sections, independent paths/factions, dense NPC regions, latency/loss/jitter, resource use and multi-hour sessions.
- Update/save migrations, diagnostics/usability and versioned content adapters.
- Stabilize language changes, placeholders/fallbacks and fonts; prepare Polish diacritics/Cyrillic support.

Revival, absence catch-up and joint story voting still need rule decisions. Optional female player voice-over is an idea, not a mandatory release requirement.

## Full version: Gothic II base release

- Complete campaign, save/resume, chapter/world transition and stability acceptance.
- Reproducible packages, tested installation/update/rollback, compatibility matrix and documented resource provenance.
- Co-op achievements, selected NPC reactions to the duo and a shared travel journal.
- Add Polish and Russian plugin translations: English, Deutsch, Polski, Русский; validate fonts and all 16 host/client language pairs.
- Keep public documentation English-first. A future public website is separate from the German productive board.

Only verified combinations are released. A version label alone does not establish a completed campaign.

## Separate later extensions

Odyssee requires its own pinned content version, quest/world/persistence adapters and chapter acceptance. Gothic 1 requires a separate engine adapter/build and script profiles. Neither is an exit condition for the Gothic II full version.

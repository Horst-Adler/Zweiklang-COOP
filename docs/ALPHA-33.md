# Alpha.33 — Orebound Co-op and local English/German UI

The current own product, C++ namespace, source folder, project and runtime artifacts now use Orebound Co-op / OreboundCoop. The installer backs up and migrates the previous runtime/INI and removes the old loader files. Legacy script IDs, existing log/state paths, save filenames and journal formats remain compatible.

Each PC selects English or German in Union & Plugins → Orebound Co-op → Language / Sprache. Save to persist; leaving without saving restores the stored selection. New profiles default to English; migrated profiles keep German. Both languages ship in the same build and do not affect the content fingerprint or network rules.

The shared catalogue contains 195 EN/DE keys, 41 menu text bindings and three own Cavalorn journal strings. Union owns the local selection/UI. Ninja ABI 2 receives the local language command. Protocol 11, journal 4, recovery 2 and actor format 1 remain unchanged. Existing journal entries retain their historical language.

Additional findings addressed: native ANSI/UTF-8/UTF-16 player name handling; valid UTF-8 diagnostic JSON; exact whole-status translation without mutating names or item descriptions; renamed diagnostic export paths and current documentation.

Validation: 4,723 technical assertions on both Windows PCs and the independent public build; 20 content-identity checks; host DE/US keyboard checks; structural menu/ABI/resource checks; new/legacy/UTF-16 installer fixtures with WhatIf, idempotence and language-only fingerprint invariance. Client headless tests deliberately skip keyboard interaction. No game was launched. Native parser, fonts, layout, preview/save and gameplay acceptance remain open.

All three development installations match across 40 content files. Existing saves, journals and logs were preserved and backed up. The productive work board stays German. Public primary documentation is English with a German companion and exact historical records.

Translation does not change original dialogue/item language or establish compatibility with another GOTHIC.DAT. The narrow Cavalorn adapter still targets the examined German script profile. Polish/Russian belong to the full-version plan; original English campaign support, public website, Gothic 1 and Odyssee require separate validation/work.

Final finding corrected: the loot audit journal now follows the selected diagnostics directory on both fresh Orebound installations and migrated legacy profiles.

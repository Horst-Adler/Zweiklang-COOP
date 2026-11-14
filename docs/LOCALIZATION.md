# English/German localization and naming

Alpha.33 introduces Orebound Co-op as the visible name. Current source and build artifacts use OreboundCoop; legacy save filenames, script symbols and optional bridge guard names are intentionally retained. Existing game installation roots are not moved by the public installer.

Union owns local language selection, persistence, native menus and diagnostics. Ninja ABI 2 receives a local language command and supplies own Cavalorn journal strings. Language never enters a network rule or content fingerprint; both languages ship in the same build.

Source catalogue: `src/OreboundCoop/localization/ui.json`. Stable IDs map complete `en`/`de` text, menu symbols and tooltips. `Build-Localization.py` validates keys/placeholders and generates the own header. UTF-8 source text is converted to the native engine codepage for display. Technical event IDs/field names remain English; human-readable details may reflect the local language.

New profiles default to English. Legacy profiles default to German if no language is declared. Set `[Interface] Language=en` or `de`, or use Union & Plugins → Orebound Co-op → Language / Sprache and save. Leaving without saving restores the persisted selection. Names, original item descriptions and other player data are not subject to substring translation.

Unknown locale values fall back to English. Missing keys are logged and show a safe English fallback. Native font/menu/layout acceptance is still required; standalone tests cover catalogues, native encoding, fallback and four local language pairs with real loopback connections.

Original dialogue/item language follows the installed game. Other GOTHIC.DAT/quest profiles need separate support; the current narrow Cavalorn adapter targets the examined German profile. Old saved journal entries retain their original text; new own entries use the selected language.

Polish and Russian, including font/codepage validation and all 16 language pairs, are full-version work. The productive board remains German; no public website is deployed by this alpha update.

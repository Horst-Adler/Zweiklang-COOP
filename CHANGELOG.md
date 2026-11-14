# Changelog

## 0.1.0-alpha.34 — 2026-11-14

- Add an optional Union/Ninja main-menu artwork contract. Use the existing native full-screen window for the image, clear only the inner window background, and retain original Gothic II logos and menu items.
- Restore original background texture references before native menu teardown; no per-frame texture clearing, custom rendering callback or global texture override.
- Build a uniquely named opaque BC1 texture with independently verified mip colors and VDF contents. Validate the optional archive before use/install.
- Preserve in-game pause menus, native text/alpha, settings and saves. Add apply/restore/fallback diagnostics, optional source-only fallback and installer rollback checks.
- 4,735 technical assertions pass. Native Ninja parsing and visual selection/submenu checks remain open; no game launched.

## 0.1.0-alpha.33 — 2026-11-14

- Rename current product/source artifacts to Orebound Co-op / OreboundCoop; preserve legacy save and script identifiers.
- Add a complete English/German UI catalogue, per-PC Union menu language setting, live preview/persistence, native text encoding and safe fallback diagnostics.
- Extend the own Ninja contract to ABI 2 for local journal text; retain protocol 11 and existing persistence formats.
- Add safe legacy runtime/INI migration without duplicate DLLs, and English-first public documentation with a German companion.
- Validate 4,723 technical assertions on both Windows PCs and the independent public build, plus 20 content-identity checks. Cover mixed local language pairs, native encodings, migration/defaults and configuration idempotence. Native parser/menu/gameplay acceptance remains open.

## Previous engineering milestones

- Alpha.32: versioned Ninja interface and optional comfort module profiles; 3,671 technical assertions.
- Alpha.31: personal character/crime state, special movement, transformations and owned summons.
- Alpha.30: confirmed save pause, inventory readiness, item matching and bounded independent log rotation.
- Alpha.29: validated interrupted journal recovery and idempotent decisions.
- Alpha.28: confirmed partner collision correction; further inventory/save regression required.

Exact original German release records are retained in [the historical archive](docs/de/history/CHANGELOG-through-alpha32.md). They describe historical evidence, not current native acceptance.

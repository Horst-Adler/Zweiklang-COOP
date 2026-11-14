# Orebound Co-op Union menu

`OreboundCoop_Menu.d` is loaded after the separately supplied `zUnionMenu.d`. The public repository does not redistribute that foreign menu script. Use **Union & Plugins → Orebound Co-op**.

One plugin configures either role, player name, host/client IPv4, port, character, logging, engine messages, friendly fire and local English/German language. Save applies/persists choices; leaving without saving restores the stored language. The language choice previews the own menu only; shared Union items are not rewritten.

Native input handling retains the Windows keyboard layout. Existing menu/script command identifiers remain compatible. The generated own text catalogue owns captions/tooltips, including a dedicated own Back item. Structural validation covers declaration order, option bindings and row spacing; the Union parser and visible native behavior still need acceptance.

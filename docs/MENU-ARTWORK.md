# Optional main-menu artwork — Alpha.34

The intended layout is an own full-screen combat image with **Orebound Co-op** centered at the top, a transparent inner main-menu background, and the original Gothic II / Night of the Raven logos and native menu entries in front. Only the out-of-game `MENU_MAIN` is modified. Pause menus and submenus retain their native behavior.

Ninja adds two uniquely named constants to its menu parser via `Ninja/ZweiklangCore/Menu_G2.src`. Union validates this menu ABI 1 contract and the matching optional archive, then uses the already existing full-screen native `m_pWindow`. It removes only `m_pInnerWindow`'s background texture. It does not rewrite `MENU_MAIN`, its items, original logos, alpha, geometry, renderer state or per-frame callbacks. Original texture references are retained and restored before native ScreenDone. Content ABI remains 2; protocol remains 11.

The archive `Data/OreboundCoop_Menu.vdf` contains only `OREBOUND_COOP_MENU-C.TEX`. It does not replace `STARTSCREEN`, menu or logo textures. BC1/DXT1 encoding avoids the earlier RGB24 color-channel path. Every mip is compared with an independent DDS decoder and checked opaque; the VDF is mounted/read back. These checks do not establish actual renderer compatibility.

## Build and install

This source repository contains the conversion tool and expected hash catalogue, not an artwork binary or original game resources. The development artwork is generated locally. To build with your own suitable source image, install Pillow and zenkit and provide it explicitly:

```powershell
.\scripts\Build-Alpha.ps1 -SdkRoot .\local\sdk\GothicCoop -MenuArtworkSource .\local\my-background.png
.\scripts\Install-Orebound.ps1 -GamePath 'C:\Games\Gothic II' -MenuArtworkDirectory .\build\alpha\menu
```

The conversion updates the own texture hash header and `config/menu-artwork.json`; compile the DLL afterwards and use the same resulting files on both PCs. A source-only build without the optional archive keeps the original menu. A supplied or retained artwork archive must match the compiled catalogue. The installer backs up the optional archive alongside owned runtime files and rolls it back on failed peer validation.

Set `[Interface] MenuArtwork=0` in the plugin INI while Gothic is closed to keep the original main menu without changing game content. Existing language/player options are retained. New diagnostics: `menu_artwork_apply`, `menu_artwork_restore`, `menu_artwork_fallback`.

## Native acceptance still required

Check warm colors, one full-screen picture, centered title, original foreground logos, readable selection, repeated Up/Down changes, settings/load/back navigation, and regular game exit. After loading a game, verify the pause menu uses the game view. Do not infer a passed visual test from build, pixel decoding or native ABI checks. No game was launched for this update.

Ninja's menu SRC format and independent menu parser are documented in the [official injection guide](https://github.com/szapp/Ninja/wiki/Inject-Changes#daedalus-scripts). No foreign menu script or Ikarus/LeGo implementation is copied into this feature.

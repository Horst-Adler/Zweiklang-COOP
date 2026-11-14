# Externe Abhängigkeiten und Herkunft

Keines der hier genannten fremden Laufzeitarchive, SDKs oder Originalspiele wird
im Repository mitgeliefert. Versionsangaben bezeichnen den untersuchten Stand,
nicht automatisch die aktuellste oder allgemein kompatible Veröffentlichung.

## Build

- Microsoft Visual C++ v142 und Windows-10-SDK: externe Entwicklungswerkzeuge.
- Union-/ZenGin-SDK aus der [GothicCoop-Referenz](https://gitlab.com/gothiccoop/gothiccoop),
  Commit `0255381eeb86ea4b888f420fafaa4a10ee4d85e3`.
- `UnionAfx.h`, Union-SDK `v1.0l`, ZenGin G2A-Definitionen, DirectX-7-Header und
  Importbibliotheken werden separat benötigt. Ihre Hinweise bleiben maßgeblich.

Die Windows-Plattformbibliotheken umfassen unter anderem Winsock, BCrypt,
Psapi, Winmm und User32. Zweiklangs Transport verwendet Winsock; ENet und die
Spielimplementierung des Referenzprojekts werden nicht eingebunden.

## Spiel und Laufzeit

- Eigene Installation von Gothic II DNdR, untersuchte EXE `2.6 (fix)`.
- Union-Laufzeit `1.0m`, einschließlich passender Menü-API. Die zugehörige
  `zUnionMenu.d` stammt aus der eigenen Union-Installation; sie ist nicht Teil
  der Veröffentlichung. Referenz zur Laufzeit: [gothic-fix-archive](https://github.com/Gratt-5r2/gothic-fix-archive).
- Zur `/MD`-Kompilierung passende x86-Visual-C++-Laufzeit.
- GD3D11 `17.9.7` ist im lokalen Testprofil installiert, aber keine mitgelieferte
  oder pauschal freigegebene Voraussetzung für beliebige Rendererfassungen.

## Pflichtkern und optionale Koop-Brücken

| Komponente | Untersuchte Fassung | Herkunft |
|---|---|---|
| Ninja | 4.1.16 | [Autorenrelease](https://github.com/szapp/Ninja/releases/tag/v4.1.16) |
| Toolkit | 16.0 | [Autorenrelease](https://github.com/szapp/Toolkit/releases/tag/v16.0) |
| Ninja Quickloot | 1.9.5 | [Autorenrelease](https://github.com/Kirides/ninja-quickloot/releases/tag/v1.9.5) |
| Dauerhaft Rennen / Sprint | 2.4.0 | Separat aus der Autorenveröffentlichung bzw. über Spine beschafft |
| PatchMenu | 1.1.1 | Separat aus der Autorenveröffentlichung bzw. über Spine beschafft |

Nur Ninja 4.1.16 ist für den Kern Pflicht. Die vier Komfortarchive sind optional; bei Quickloot, Sprint oder PatchMenu ist Toolkit 16.0 bedingt erforderlich. Verwendetes Quickloot/Sprint verlangt Brückenrevision 1.
Die Originaldateien werden deshalb mit dem eigenen, veröffentlichten
`Prepare-QolCompatibility.py` **lokal** transformiert. Deren Daten werden nicht
als neue eigene Mod veröffentlicht. Das Werkzeug kennt die Hashes des untersuchten
Originalstands und lehnt andere Dateien ab.

Quickloot nennt eine MIT-Lizenz; weitere Komponenten behalten ihre jeweiligen
eigenen Rechte. Aus einer lokal installierten Datei wird keine pauschale Erlaubnis
zur Weiterverbreitung abgeleitet. Deshalb liegen weder die Archive noch daraus
kopierte komplette Fremdskripte im Repository. Das gilt auch für Union-Menüskript,
Renderer und SDK-Binärdateien.

## Figuren und Spielressourcen

Die Mod referenziert vorhandene Gothic-II-Körper, Morphköpfe und Gesichtstexturen.
Das weibliche Profil wurde anhand der lokal installierten Velaya-Mod untersucht;
deren Welt-, Dialog-, Modell- oder Texturdateien werden hier nicht mitgeliefert.
Die zurückgezogenen Outfit-/Startbildexperimente sind kein Bestandteil der
aktiven Alpha-Veröffentlichung.


Aktuelle Profile und automatische Erkennung: [Union/Ninja](UNION-NINJA.md). Python 3.12 mit zenkit 1.3.0.4 ist eine reine Build-Abhängigkeit für das eigene Ninja-VDF, keine zusätzliche Spiel-Laufzeit.

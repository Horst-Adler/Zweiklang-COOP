# Installation der Entwickler-Alpha.32

Technisch implementiert, gemeinsame native Abnahme offen. Keine Spiele, SDKs oder fremden Plugins enthalten; kein öffentliches Fertig-Release vorausgesetzt.

## Basis und eigener Build

Gothic II DNdR 2.6-fix/LAA (x86), Union 1.0m und passende x86-VC++-Laufzeit separat auf beiden PCs vorbereiten. Das fremde `System/Autorun/Menu/zUnionMenu.d` aus der eigenen Union-Installation behalten oder dem Installer über `-UnionMenuScriptPath` bereitstellen. Vor Änderungen Spiele schließen und Spielstände sichern. Keine unterschiedlichen TCs oder unabhängig fortgeschrittenen Saves mischen.

Nach [BUILDING.md](BUILDING.md) bauen. Eigene Runtime:

```text
System/Autorun/ZweiklangCoop.dll
System/Autorun/ZweiklangCoop_Menu.d
System/Autorun/ZweiklangCoop_Npc.d
Data/ZweiklangCoop_Core.vdf
System/ZweiklangCoop.ini
```

Die Core-VDF enthält eigene Ninja-Skripte, kein Startbild. Das Originalmenü bleibt aktiv. Union ist Hauptkern; Ninja 4.1.16 ist die zusätzliche Pflichtlaufzeit. Toolkit, Quickloot, Sprint und PatchMenu sind für das Minimalprofil nicht erforderlich.

## Minimalprofil

Ninja 4.1.16 separat installieren. Den Ninja-Loader registrieren, falls noch nicht vorhanden:

```powershell
python .\scripts\Configure-Qol.py "D:\Spiele\Gothic II" --loader-only
.\scripts\Install-Alpha.ps1 -GamePath "D:\Spiele\Gothic II" -WhatIf
.\scripts\Install-Alpha.ps1 -GamePath "D:\Spiele\Gothic II"
```

`--loader-only` ergänzt ausschließlich `Ninja.dll**` in der Pluginliste der Spielkonfiguration; es erzeugt keine Komfortoptionen und ändert keine Grafikwerte. Vorhandene persönliche Werte bleiben erhalten. Der Installer installiert standardmäßig keine Komfortarchive. Bereits vorhandene Archive werden erhalten und geprüft, deshalb für ein wirklich minimales Profil alle vier Komfortarchive bei geschlossenem Spiel außerhalb von Data sichern/deaktivieren und den Installer erneut ausführen.

## Optionales Komfortprofil

Unterstützt werden Quickloot 1.9.5 und Sprint 2.4.0 **mit privater Koop-Brücke Revision 1**, Toolkit 16.0 und PatchMenu 1.1.1. Die untersuchten Komfortmodule benötigen unter Ninja 4 Toolkit; es wird bei expliziter Auswahl automatisch in die Dateiauswahl aufgenommen. Unverändertes Upstream-Quickloot/Sprint ist im Koop nicht sicher und wird abgewiesen.

Originaldateien selbst beschaffen. Brücken lokal erzeugen; Beispiel nur für Quickloot, mit benötigtem Toolkit im Eingabeordner:

```powershell
python .\scripts\Prepare-QolCompatibility.py --input .\local\originale --output .\local\qol-kompatibel --modules Quickloot
.\scripts\Install-Alpha.ps1 -GamePath "D:\Spiele\Gothic II" -QolDirectory .\local\qol-kompatibel -OptionalModules Quickloot
```

Für das vollständige bisherige Profil ohne `--modules` vorbereiten und `-OptionalModules Quickloot,Sprint,PatchMenu` verwenden. Alternativ einzelne Module auswählen. Ninja.dll im Eingabeordner ist für das reine Vorbereiten optional; als Spielbasis bleibt Ninja erforderlich. Hashes stehen in `config/Dependencies.json`. Fremde Ausgaben nicht veröffentlichen. Komfort-Voreinstellungen lassen sich bei Bedarf ohne `--loader-only` ergänzen; vorhandene Werte bleiben bestehen.

## Sicherheit, Verbindung und Rückbau

Der Installer prüft die wirksamen Abhängigkeiten vor dem Schreiben, sichert betroffene Runtime-Dateien, installiert die vier eigenen Laufzeitdateien und berechnet die vollständige ContentId. Persönliche Konfiguration wird erhalten; neue Rollen-/Namensparameter überschreiben bestehende Profile nicht. Ingame im Union-Menü Rolle, Name, IP, Port, Figur und Friendly Fire prüfen. F6 Host, F7 beitreten, F8 trennen; zuerst eine Spielwelt laden und sichere Savebindung herstellen.

Beide PCs brauchen denselben Build, Protokoll 11, dieselbe Welt und aktuell **dasselbe optionale Profil**. Unbekannte/geänderte oder nur auf einem PC vorhandene Erweiterungen nicht durch kopierte ContentId übergehen. Automatische Erkennung prüft zusätzlich tatsächlich geladene Skripte und Brückenrevisionen. Der Installer selbst startet kein Spiel und ändert keine Saves, Recovery-Dateien oder Windows-/VM-Einstellungen.

Rückbau: `scripts/Restore-Runtime.ps1 -Manifest <gesichertes-manifest.json> -GamePath <Spielpfad>`. Es werden nur explizit zugelassene Runtime-Dateien wiederhergestellt oder neu hinzugekommene deaktiviert. Keine Save-Rücksetzung. Vor älteren Builds separat die Save-/Formatkompatibilität prüfen.

## Noch im Spiel zu prüfen

Minimalprofil: Parser/Start, Beitritt/Bereitschaft, reguläres Aufheben, Leiche/Kiste, Speichern/Recovery, Händler, Cavalorn-EP. Komfortprofil zusätzlich Quickloot, Sprint und PatchMenu. Gesonderte Alpha.31-Charakterfälle separat abnehmen. Der kurze [Cavalorn-Test](docs/TESTPLAN-CAVALORN.md) ist keine komplette Kampagnenabnahme. [Modulvertrag und Grenzen](docs/UNION-NINJA.md).

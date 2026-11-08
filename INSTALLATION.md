# Installation und Verwendung der Entwickler-Alpha

Diese Anleitung beschreibt die vorhandene **Alpha.26**, kein öffentlich
abgenommenes Fertigpaket. Es wird kein GitHub-Release vorausgesetzt oder erzeugt.
Spiele, Union, SDK und fremde Modarchive sind nicht enthalten.

## 1. Zwei passende Spielinstallationen vorbereiten

1. Gothic II mit **Die Nacht des Raben** separat auf beiden PCs installieren.
2. Den untersuchten Stand **2.6 (fix), x86, Union 1.0m** herstellen.
3. Vor Änderungen beide Spiele regulär beenden und Testspielstände sichern.
4. Keine unterschiedlichen Total Conversions, Skript-/Pluginstände oder
   unabhängig fortgeschrittenen Saves mischen.

Der erste Questadapter ist an ein untersuchtes deutsches DNdR-Skriptprofil
gebunden. Andere Sprach-/Skriptfassungen sind nicht automatisch bestätigt.

## 2. Eigenes Plugin bauen

Quellcode über **Code → Download ZIP** auf der Repositoryseite beziehen oder:

```powershell
git clone https://github.com/Horst-Adler/Zweiklang-COOP.git
Set-Location Zweiklang-COOP
```

[BUILDING.md](BUILDING.md) ausführen. Die benötigte eigene Runtime besteht aus:

```text
System/Autorun/ZweiklangCoop.dll
System/Autorun/ZweiklangCoop_Menu.d
System/Autorun/ZweiklangCoop_Npc.d
System/ZweiklangCoop.ini
```

Die DLL entsteht beim Build; die zwei eigenen `.d`-Dateien liegen im Quellprojekt.
Die Konfiguration wird lokal erzeugt. Die aktuelle Alpha enthält **kein eigenes
Startbild-VDF** und verwendet wieder das originale Gothic-Menü.

Zusätzlich ist `System/Autorun/Menu/zUnionMenu.d` aus der eigenen Union-
Installation erforderlich. Dieses fremde Menüskript wird nicht hier veröffentlicht.
Eine bereits vorhandene passende Datei bleibt nutzbar; andernfalls muss sie
dem Installer über `-UnionMenuScriptPath` separat bereitgestellt werden.

## 3. Erforderliche QoL-Dateien separat beschaffen

Für dieses konkrete Testprofil werden verwendet:

| Komponente | Version | Benötigte Originaldatei |
|---|---|---|
| Ninja | 4.1.16 | `Ninja.dll` |
| Toolkit | 16.0 | `Toolkit.vdf` |
| Quickloot | 1.9.5 | `Quickloot.vdf` |
| Dauerhaft Rennen / Sprint | 2.4.0 | `Sprint.vdf` |
| PatchMenu | 1.1.1 | `PatchMenu.vdf` |

Die Autorenquellen stehen in [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md).
Originale ausschließlich lokal ablegen. Keine Archive in dieses Repository committen.

Die aktuelle DLL verlangt die Marker `NINJA_QUICKLOOT_ZK_BRIDGE_REV = 1`
und `NINJA_SPRINT_ZK_BRIDGE_REV = 1`. Diese fehlen in unveränderten Originalarchiven.
Das folgende eigene Werkzeug fügt die schmalen Kompatibilitätsabzweige lokal ein:

```powershell
# Eingabe: fünf flach nebeneinander liegende, separat beschaffte Originaldateien.
python .\scripts\Prepare-QolCompatibility.py --input .\local\qol-originale --output .\local\qol-kompatibel
```

Das Werkzeug kontrolliert die bekannten Dateihashes, verändert die Originale nicht
und erzeugt `Data/Quickloot.vdf`, `Data/Sprint.vdf`, die übrigen drei unveränderten
Laufzeitdateien sowie einen lokalen Nachweis. Nicht passende Originaldateien werden
abgelehnt. Die erzeugten Archive und privaten Diffs bleiben lokale Arbeitsdaten,
keine veröffentlichten Zweiklang-Ressourcen.

## 4. Lokal installieren

Beispiel mit einer eigenen Testinstallation:

```powershell
# Erst die geplanten Schritte ansehen; keine Dateien werden geändert:
.\scripts\Install-Alpha.ps1 -GamePath "D:\Spiele\Gothic II" -QolDirectory .\local\qol-kompatibel -WhatIf

# Ersteinrichtung auf dem Host:
.\scripts\Install-Alpha.ps1 -GamePath "D:\Spiele\Gothic II" -QolDirectory .\local\qol-kompatibel -Role Host -PlayerName Spieler1 -HostAddress 192.0.2.10
```

**`192.0.2.10` ist eine Dokumentationsadresse und muss durch die tatsächliche
Host-IPv4 ersetzt werden.** Das gilt auch für alle im Folgenden gezeigten Beispielpfade.
Kein Beispiel enthält die Adressen oder Installationsorte einer realen Testumgebung.

Falls das fremde Union-Menüskript noch nicht am Ziel liegt:

```powershell
.\scripts\Install-Alpha.ps1 -GamePath "D:\Spiele\Gothic II" -QolDirectory .\local\qol-kompatibel -UnionMenuScriptPath "D:\EigeneUnionDateien\zUnionMenu.d"
```

Der Installer prüft Voraussetzungen, sichert betroffene Runtime-Dateien, kopiert
die exakt vorgesehenen Dateien und berechnet aus Spiel-/Skript-/Plugininhalten
eine gemeinsame ContentId. Vorhandene persönliche Pluginoptionen werden erhalten;
Rollen-/Namensparameter gelten bei vorhandener Konfiguration nicht als erzwungene
Änderung. Anschließend Einstellungen im Union-Menü prüfen.

Das Werkzeug startet kein Spiel und verändert keine Spielstände oder
Recovery-Dateien. Es ist kein allgemeiner Modmanager und prüft nicht jede
denkbare fremde Plugin-/TC-Kombination. Bestehende eigene Startbildexperimente
aus anderen Arbeitsständen werden von diesem öffentlichen Installer nicht
automatisch entfernt; die saubere Alpha.26-Basis setzt sie nicht voraus.

## 5. Ninja-Loader und QoL-Voreinstellungen

```powershell
python .\scripts\Configure-Qol.py "D:\Spiele\Gothic II"
```

Das Hilfswerkzeug ergänzt fehlende Sprint-/Quickloot-Optionen in `Gothic.ini`
und den Loader `Ninja.dll**` in `[PLUGINS] PluginList` von `SystemPack.ini`
sowie einer vorhandenen `Union.ini`. Andere Pluginlisteneinträge bleiben erhalten.
Vorhandene eigene QoL-Werte und die bestehende Textkodierung werden berücksichtigt;
Grafik- und Tasteneinstellungen werden nicht neu gesetzt.
ANSI und UTF-8 einschließlich BOM werden unterstützt. UTF-16-Laufzeitdateien
werden vor Änderungen abgelehnt, statt sie stillschweigend umzuschreiben.

Ohne vorhandene eigene Werte gelten Sprint-Stärkegrenze 0, dauerhaftes Rennen,
Standardanimation und Quickloot per rechter Maustaste. Das ist die Testbasis,
keine Behauptung, jede optionale Sprintanimation sei bereits im Spiel abgenommen.

## 6. Zweiten PC konfigurieren

Auf dem Gast dieselben gebauten DLL-/Skriptdateien und kompatiblen Archive
verwenden. Dort Rolle `Client`, eigener Spielername und dieselbe Host-IP/Port
eintragen. Der Eintrag **Client-IP** bezeichnet die lokale Gastadresse.
Der Rollenwechsel im Union-Menü kann die primäre lokale IPv4 ermitteln.

Die ContentId muss auf beiden PCs übereinstimmen. Unterschiedliche Original-
oder Plugininhalte werden bewusst nicht durch manuelles Angleichen dieser
Kennung übergangen.

## 7. Start und erfolgreicher Beitritt

1. Beide Gothic normal starten und passende Saves derselben Welt laden.
2. **Einstellungen → Union & Plugins → Zweiklang Coop** öffnen und Werte prüfen.
3. Host mit F6 starten; Gast mit F7 verbinden.
4. Nicht nur `Connected`, sondern vollständige Bereitschaft für Kampf, Loot und
   den unterstützten ersten Questfall abwarten.
5. Namen, Figuren, Bewegung und Diagnose kontrollieren; dann den
   [Cavalorn-Testplan](docs/TESTPLAN-CAVALORN.md) verwenden.
6. F8 trennt regulär. Das Öffnen der Union-Konfiguration kann die
   Sitzung absichtlich beenden.

## Speichern und Wiederherstellung

Beide speichern getrennt in passenden Slots. Nach Laden Sitzung neu
starten und Bereitschaft abwarten. Journale und Save-Checkpoints gehören zusammen.
Alte Host-Saves vor bereits protokollierten Questgaben oder Transfers nicht
ungeprüft erneut als Ausgangspunkt verwenden.

Runtime-Sicherungen des Installers liegen ausschließlich lokal unter
`local/install-backups`. Ein gezielter Rückbau ist möglich:

```powershell
.\scripts\Restore-Runtime.ps1 -Manifest .\local\install-backups\ZEITSTEMPEL\manifest.json -GamePath "D:\Spiele\Gothic II" -WhatIf
```

Danach denselben Befehl ohne `-WhatIf` nur für den bewusst gewählten passenden
Runtime-Stand verwenden. `ZEITSTEMPEL` durch den tatsächlich erzeugten Ordner
ersetzen. Das Skript prüft Zielpfade und Backuphashes; neu hinzugekommene Dateien
werden umbenannt statt gelöscht. Es stellt **keine** gemeinsamen Saves her und
migriert keine neueren Journale zurück in beliebige alte Pluginformate.

## Erneuter Test nach dem Alpha.26-Abbruch

Alte Testslots mit neuem Vorschaubild, aber fehlendem neuen Welt-/Charakterstand nicht als bestätigte Ausgangsbasis verwenden. Beide beginnen einen frischen Testzweig und speichern zunächst getrennt in freie Slots. Danach diese neuen Stände laden, verbinden und vollständige Bereitschaft abwarten. Bei Recovery-/Speicherfehlern hier stoppen und lokale Diagnose sichern. Alpha.27 umgeht keine Inventarsperre und repariert frühere Teil-Saves nicht automatisch.

Ab Alpha.28 erhält reguläres Speichern die Verbindung. Nacheinander und ohne offene Transfers speichern; während der angekündigten Pause warten. [Befunde und Wiederholungstest](docs/ALPHA-28.md).

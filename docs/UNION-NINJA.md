# Entwicklungsgrundlagen und Modulvertrag

Union ist der native Hauptkern. Ninja 4.1.16 lädt die eigene ergänzende Daedalus-Schicht. Das eigene Ninja-Modul benötigt selbst weder Toolkit noch PatchMenu und ersetzt keine globalen Spielinitialisierungen oder fremden Funktionen.

## ABI 1

`Patch_ZweiklangCore_ABI` ist eine Konstante. Union prüft die Funktion `Patch_ZweiklangCore_Dispatch` (keine Parameter) und die skalaren Integer `Command`, `Value`, `Result` mit demselben Präfix. Command 0 ist eine nebenwirkungsfreie Probe, 1 bereitet den Händlerbestand vor, 2 bereinigt das reguläre Runeninventar einer Leiche, 3 gewährt bereits autorisierte lokale Quest-EP. Nur Command 3 akzeptiert Value 1..1.000.000; sonst muss Value null sein. Result wird vor Aufruf auf null gesetzt und bei erfolgreicher Ausführung eins. Union bewahrt die vorherigen Werte und SELF/OTHER/HERO/ITEM/VICTIM-Bindungen. Keine Zeiger werden im Save gespeichert, keine Transaktion entsteht allein durch einen Skriptbefehl.

Die vorhandenen alten Helfernamen bleiben als Kompatibilitätssymbole bestehen; die drei aktuellen Ausführungspfade gehen durch die Ninja-Schnittstelle. Weitere Module bekommen eigene Namen/ABI und geprüfte Befehle; bei einer Vertragsänderung Revision erhöhen und passende Tests ergänzen. Netzwerk und persistente Inventarentscheidungen bleiben im Union-Kern.

## Profile

| Profil | Pflicht | Optional |
|---|---|---|
| Minimal | Unterstütztes Gothic II DNdR 2.6-fix/LAA, Union 1.0m, Ninja 4.1.16, eigene DLL, zwei Union-Skripte, eigene Core-VDF, eigene Konfiguration und vorhandenes Union-Menüskript | Alle vier Komfortarchive entfallen |
| Mit Komfort | Wie Minimal | Quickloot 1.9.5 + Brücke 1, Sprint 2.4.0 + Brücke 1, PatchMenu 1.1.1; für jede dieser Varianten Toolkit 16.0 als bedingte Bibliotheksabhängigkeit |

Toolkit allein ist ebenfalls optional zulässig. Die Brücken akzeptieren ausschließlich die in `config/dependencies.json` und `Extensions.h` dokumentierten Hashes. Unverändertes Upstream-Quickloot/Sprint ist im Koop nicht freigegeben. Eine fehlende Erweiterung ist erlaubt; eine geladene unbekannte Variante wird nicht durch einen Konfigurationsschalter als sicher ausgegeben. Nicht erfasste Mods brauchen eine eigene Kompatibilitätsprüfung.

Beide PCs verwenden denselben Build und dasselbe optionale Profil. Den vollständigen Contentvergleich einschließlich optionaler Archive beibehalten. Zur Profiländerung Spiele schließen, Archive außerhalb des Data-Verzeichnisses sichern/deaktivieren und aktuellen Installer auf beiden PCs erneut verwenden. Keine fremden Savevarianten mischen; erst in getrennten Testslots abnehmen.

## Technische und manuelle Prüfungen

- Codec-, UDP-, Journal-, Save-, Inventar-, Handel-, Quest-, Charakter- und Bereichstests bleiben aktiv.
- Neue Tests für 16 Modulbestückungen, bedingtes Toolkit, unbekannte Hashes/Revisionen, fehlende Ninja-/ABI-/Skriptmerkmale, Befehlsgrenzen und echte Windows-SHA256-Dateilesung.
- Installationsprüfungen mit echten getrennten Dateikopien für Minimal- und Komfortprofile; keine Änderungen an produktiven Komfortarchiven für die Profiltests.
- Noch offen: nativer Start/Parser in Minimal- und Komfortprofil; Beitritt/Bereitschaft, reguläres Aufheben, Leiche/Kiste, Speichern/Recovery, Händler und Cavalorn-EP. Komfortprofil zusätzlich Sprint/Quickloot und PatchMenu-Bedienung. Neue Alpha.31-Sonderfälle separat prüfen.

Die native Prüfung wird nicht durch einen erfolgreichen Build oder einen statischen Skriptcheck ersetzt. Kein Spiel automatisch starten.

## Primärquellen

Ninja dokumentiert die getrennten SRC-Dateien, Namensräume, Initialisierung und dynamischen Versions-/Patchmerkmale: [Inject Changes](https://github.com/szapp/Ninja/wiki/Inject-Changes). Toolkit liefert die für modulare Mods angepassten Ikarus-/LeGo-Bibliotheken: [Toolkit](https://github.com/szapp/Toolkit). Fremde Bibliotheken oder Archive werden nicht mit dem eigenen Quellcode veröffentlicht.

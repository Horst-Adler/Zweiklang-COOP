# Technische Architektur

## Einbindung und Ziel

Das Projekt baut eine Win32-DLL für die untersuchte Gothic-II-DNdR-Engine mit
Union. `UnionEntry.cpp` bindet das Modul ein; `Plugin.cpp` registriert Lebenszyklus-,
Menü-, Eingabe- und native Spieladapter. Native Adressen und ABI-Annahmen sind
versionsgebunden. Andere Engines oder EXE-Stände werden nicht durch eine ähnliche
Dateibezeichnung automatisch unterstützt.

Die normale Spielansicht, Kamera und lokale Eingabe bleiben je Spielinstanz
erhalten. Es gibt keinen Splitscreen, keinen dedizierten Spielserver und keine
Controllerzuordnung für mehrere lokale Figuren.

## Transport und Nachrichten

`Network.cpp` nutzt nicht blockierendes Winsock-UDP. `Poll` wird aus Spiel- und
Menücallbacks bedient; es gibt keinen Enginezugriff aus einem separaten
Netzwerkthread. Bewegungszustände sind von bestätigten, geordneten Ereignissen
getrennt. Sitzungs-/Sequenzprüfungen, ACKs, Wiederholungen und begrenzte Queues
reduzieren die Wirkung verlorener oder doppelter Pakete.

`Protocol.h` und die fachlichen Protokollheader enthalten Längen-/Werteprüfungen,
Versionen und Serialisierung. Die aktive Protokollversion ist 9. Rohe
Speicheradressen werden nicht als netzwerkweite Objektidentitäten verwendet.

## Engine-Adapter

| Adapter | Inhalt |
|---|---|
| `CombatBridge.inl` | Hostgeführte NPC-/Kampfzustände, Treffer, Partner, Aggro und Friendly Fire |
| `LootBridge.inl` | Quellenidentität, exklusive Freigabe, Bestände, Mengen und native Inventarinteraktion |
| `WorldBridge.inl` | Bodenitems, NPC-Handel, Lebenszyklus und zwei Aktivierungsgebiete |
| `RecoveryBridge.inl` | Verbindung zwischen persistenten Entscheidungen und nativen Item-/Gold-/Questwirkungen |
| `SessionBridge.inl` | Bereitschaft, Sperrgründe, Diagnosekontext und Testmarken |
| `CavalornBridge.inl` | Enges Skriptprofil für Cavalorns Banditen und tatsächliche Gaben |
| `QolBridge.inl` | Bestätigter Quickloot-Pfad, Sprint-Overlays und Schutz des ungeprüften Originalpfads |

Die lokale Spielfigur bleibt Eigentümerin ihrer eigenen Spielerzustände.
Der Partner ist eine eigene leere NPC-Instanz; `PC_HERO` wird nicht als
Inventarvorlage geklont oder global neu gebunden. Temporäre Figuren und native
Objektreferenzen benötigen einen definierten Laden-/Disconnect-Lebenszyklus.

## Transaktionen und Persistenz

`Recovery.h` verwaltet persistente vorbereitete und terminale Entscheidungen
sowie versionierte Checkpoints. Quellenentnahme und Empfängergutschrift werden
als nachvollziehbare Effekte abgeglichen. Wiederholte Zustellung soll einen
bereits angewendeten Effekt nicht erneut erzeugen.

Aktuell: Journalformat 4, Checkpointformat 2. Technische Regressionen umfassen
echte synthetische Ausgaben der früheren Alpha.17-/Alpha.18-Serializer.
Das ist keine vollständige gemeinsame Weltspeicherung und keine allgemeine
Umkehr bereits verbuchter Aktionen. Save, Profil-/Zeitlinie und Recovery-Daten
müssen zusammenpassen. Native Speichervorgänge behalten ab Alpha.28 die Sitzung über geordnete zuverlässige Transportkontrollen bei; Spielzustandsverarbeitung und Inventaraktionen warten während der Pause.

## Erster Questadapter

Der Cavalorn-Adapter erkennt ein konkretes untersuchtes Skriptprofil und
bekannte Info-Instanzen. Hostdialoge bleiben maßgeblich. Die Gegenseite erhält
bekannten Journal-/Queststand und tatsächlich gewährte Gaben über eindeutige
Effektkennungen. Bedingungen werden im entsprechenden Lebenszyklus wiederhergestellt.

Dadurch sind weder beliebige allgemeine Quests noch freie Gast-Abgabe,
Fraktionslogik oder perfekte Kampf-EP-Teilung automatisch implementiert.
Modabhängige Inhalte müssen später über eigene geprüfte Adapter angebunden werden.

## QoL-Brücken

Der aktuelle Startpfad verlangt zwei Marker der Brückenrevision 1. Die lokal
transformierten Archive rufen eigene Daedalus-Schutzfunktionen ab. Quickloot
verwendet im Koop einen hostbestätigten Transaktionspfad; der Gast darf nicht
einfach fremde Spiegelinventare in sein eigenes Inventar kopieren. Nach einer
Sitzung bleibt der ungeprüfte Schnellpfad bei Trennung gesperrt; bekannte
Recovery-Fehler sperren ihn ebenfalls.

Die Sprint-Brücke begrenzt Gaständerungen an fremden Begleitern. Bekannte
Overlaykennungen werden mit Spieler-/NPC-Zuständen übertragen. Keine beliebigen
Dateinamen aus dem Netz werden als frei wählbare Modellpfade akzeptiert.

## Menü und Diagnose

Eigene Daedalus-Instanzen registrieren das Union-Untermenü. C++ befüllt den
eigenen Optionsbereich, unterstützt die native Texteingabe und verarbeitet
eigene Aktionen. **Seit Alpha.26 gibt es keine eigene Hauptmenügrafik- oder Transparenzänderung.**

Diagnoselogik erfasst ausgewählte Ereignisse mit Kontext, Zeit-/Sequenzdaten,
Logstufen und Rotation. Testmarken besitzen eine gemeinsame Kennung; das
automatische Bestehen eines Testschritts wird daraus nicht abgeleitet.
Logdateien können lokale Namen, Adressen und Pfade enthalten und bleiben lokal.

## Grenzen der Nachweise

Die 2.250 technischen Assertions testen unter anderem echte UDP-Loopbacks,
Codecs, Fehlergrenzen, Recovery-Matrizen und Eingabe-/Konfigurationshilfen.
Sie ersetzen keine Engine-, Renderer-, Skript- oder Kampagnenabnahme auf zwei PCs.
Die bestätigten früheren Spielbefunde sind in README und Changelog getrennt genannt.

## Alpha.27: Grenzen zwischen nativer Engine und Recovery

Die Save-Vorprüfung läuft vor `CGameManager::Write_Savegame`, bevor die Engine Metadaten oder Skriptdateien verändert. Der Verzeichnispfad kommt aus dem nativen Save-Verzeichnisobjekt; `NativeSave.h` prüft die Slotkomponente, Schreibbarkeit und Fortschreibung von Nutzdateien. Ein nativer Rückruf allein bestätigt keinen vollständigen Save. Direkte Ladepfade werden ebenfalls gebunden. Unverwaltete direkte Save-Aufrufe außerhalb des internen Current-Slots werden abgelehnt.

Ein Gast-Gesprächsversuch gibt die KI nur des angefragten NPCs kurzzeitig frei; bestätigte Gesprächsreferenzen übernehmen danach. Ohne Gespräch läuft die Spiegelung wieder. Die ursprüngliche NPC-Physik wird beim Trennen wiederhergestellt. Ein exakt identischer, eindeutiger Spawnanker darf enge Gruppen auflösen; mehrfach identische Anker bleiben unzugeordnet.

Die Diagnose behält zusätzlich Level-1-Ereignisse in `.events.jsonl` (16 MiB, vier Rotationen) und den ersten MiB der Sitzung in `.begin.jsonl`. Beide verwenden dieselbe Sequenz wie das Hauptlog. Zusammenführungen müssen doppelte Sequenzen derselben Sitzung verwerfen. Diese Dateien und die native Spielabnahme sind in [Alpha.27](ALPHA-27.md) beschrieben.

Alpha.28 verwendet Protokoll 10 und interne geordnete Savekontrollen (Payload 63). Die Speicherpause ist auf 120 Sekunden begrenzt; ACK-Fristen werden beim Fortsetzen neu angesetzt. Kollisionsausschluss nach Weltaufnahme und gezielter KI-Paarfilter ergänzen sich.


## Alpha.32: Union-/Ninja-Modularisierung

Technisch umgesetzt: Union-Hauptkern, eigener Ninja-Befehlsvertrag ABI 1 und automatische optionale Modulprofile. Pflichtkern ohne Komfortmods; bedingte Toolkit-Abhängigkeiten und vollständiger Contentvergleich erhalten. Native Abnahme beider Profile noch offen. [Architektur und Testmatrix](UNION-NINJA.md).

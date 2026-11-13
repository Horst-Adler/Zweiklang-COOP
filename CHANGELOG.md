# Versionshistorie

## 0.1.0-alpha.31 – 2026-11-14

Persönliche Straftat-/NPC-Zustände mit gesicherter Charakterdatei; lokale Fortschritts- und Formschutzregeln; besondere Bewegungsprioritäten und Overlays; Gestaltübertragung und besitzergebundene Standardbeschwörungen. Protokoll 11, 3.413 technische Assertions, native Abnahme offen. [Details](docs/ALPHA-31.md).

## 0.1.0-alpha.30 – 2026-11-13

Bestätigte Speicherpause vor nativem Schreiben, Partner-Inventarsperren, erreichbare Trennung und richtige Statusmeldungen während Savepausen; Bodenitem-Zuordnung und doppelte Drop-Tags korrigiert; Saveguard-Bereinigung geprüft; Logrotation bleibt bei Dateisperren begrenzt und Ereignisse unabhängig. 2.391 Netzwerktest-Assertions plus Diagnoseprüfungen. Native Abnahme offen. [Rundumcheck](docs/ALPHA-30.md).

## 0.1.0-alpha.29 – 2026-11-13

Sichere Haupt-/Pending-Journalwiederherstellung mit Beweiserhalt und Konfliktsperre, Schutz gegen erfundene Aborts bei bestätigten Client-Commits, idempotente dauerhafte Schreibvorgänge, aktive Zeitlinien vor lokalem Rücksetzen prüfen und genaue Persistenz-/Itemdiagnosen. 2.342 technische Assertions. Native Abnahme offen; keine VM-/Treiberänderung. [Details](docs/ALPHA-29.md).

## 0.1.0-alpha.28 – 2026-11-08

Neue Inventar-Basis bei neuem Spiel, Startschutz bei ungeklärter Bindung, zuverlässige Speicherpause mit Sitzungserhalt und begrenzter Timeout-Verlängerung, Partnerkollision nach Weltaufnahme und gezielter Paarfilter. Versionsanzeige/Diagnose ergänzt. Protokoll 10; 2.250 technische Assertions. Native Abnahme offen. Details: [Alpha.28](docs/ALPHA-28.md).

Belegbare Entwicklungsstände, keine nachträglichen öffentlichen Releases.
Die aktuelle Quellversion ist **0.1.0-alpha.27**, Stand 7. November 2026.
Die lokale Entwicklungsnummer wird für diese Veröffentlichung nicht erhöht.

| Stand | Änderungen | Nachweis / Einschränkung |
|---|---|---|
| Frühe Alpha-Basis | Projektstruktur, eigener Netzwerkkern, gemeinsames Rollenplugin, Union-Menü, Eingabe und Figurenauswahl | Nicht jeder frühe Zwischenstand lässt sich separat nummerieren; keine erfundenen Einzelreleases. |
| Alpha.8 | Ladbare Basis nach Modell-/Outfitversuchen wiederhergestellt | Laden vom Nutzer bestätigt. |
| Alpha.9–10 | Outfitversuche | Zurückgezogen nach fehlender Darstellung bzw. Textur-/Ladefehlern; Kleidung in Beta. |
| Alpha.11 | Erweiterte Diagnose, Morphkopf-/Körperdarstellung und Bewegungsanimationen | Köpfe/Animationen und Reconnect bestätigt; Gesichter als Folgefehler. |
| Alpha.12 | Eigene Kopftexturauswahl für Originalheld und Velaya | Nutzer bestätigte korrigierte Gesichter. |
| Alpha.13 | Gemeinsame Kampfzustände, zuverlässige Treffer, eigene Spieler-HP und Friendly Fire | Verbindungsversuch verursachte Abstürze; nicht als stabile Abnahme gewertet. |
| Alpha.14 | Zugriff auf inaktive Animationskanäle abgesichert | Verbindung wieder möglich; Bewegung/Gasttreffer/Aggro noch fehlerhaft. |
| Alpha.15 | Bewegung, Trefferreichweite und Aggro korrigiert | Bewegung ohne Reconnect, Gastschaden, Gast-Aggro und gleiche Gegner-HP bestätigt. |
| Alpha.16 | Erste hostbestätigte Loot-Entnahme vorbereitet | 808 technische Prüfungen; damals nur vorbereitet. |
| Alpha.17 | Einlagern, Schlösser, persistente Transferentscheidungen und Save-Checkpoints | 1.067 technische Prüfungen; gemeinsame native Abnahme offen. |
| Alpha.18 | Bodenitems, NPC-Handel, Lebenszyklus, Aktivierungsgebiete und Recovery erweitert | 1.426 Assertions; neuere Spielabläufe noch abzunehmen. |
| Alpha.19 | Bereitschaftsanzeige, Testmarken und enger Cavalorn-Questadapter | Final 1.964 Assertions; Hostdialoge, tatsächliche volle Gaben; keine allgemeine Questlösung. |
| Alpha.20 | Kollisionsfreier Darstellungsschlüssel und Recovery-Zuordnung bewegter NPCs | 1.975 Assertions; technische Befundkorrekturen. |
| Alpha.21 | Sprint-/Quickloot-Brücken und Abhängigkeiten | 2.170 Assertions; Protokoll 9 und Journalformat 4. |
| Alpha.22 | Quickloot bei geladenem Recovery-Fehler gesperrt; temporäre Item-Referenzen abgesichert | 2.180 Assertions; native Abnahme offen. |
| Alpha.23 | Partnernamen bei weggesteckter Waffe anvisierbar; FF-Schadensschutz getrennt erhalten | 2.196 Assertions; Fokusänderung im Spiel abzunehmen. |
| Alpha.24 | Eigener Startbild-Einbau | Nutzer meldete Blaustich und doppelte Grafik. |
| Alpha.25 | DXT1-/Transparenzversuch | Nutzer meldete schwarzen Hintergrund und Renderglitches; zurückgezogen. |
| **Alpha.26** | **Originalmenü wiederhergestellt; Startbild in Beta verschoben** | Menücode wie vor dem Grafikeinbau, eigene Grafik-VDF deaktiviert. Übriger Koop-Kern erhalten; 2.196 Assertions bestanden. |

## Veröffentlichung dieses Quellstands

- Eigenständige Quellen und synthetische Recovery-Fixtures getrennt zusammengestellt.
- Deutsche README, Architektur-, Build-, Installations-, Roadmap- und Beitragsdokumentation.
- Portable Entwicklungs-/Installationswerkzeuge und lokale QoL-Archivtransformation.
- Explizite Publikationspositivliste und vollständige Indexprüfung.
- Originalspiele, Website, fremde SDK-/Laufzeitarchive, private Konfigurationen,
  Diagnosearchive und Buildprodukte ausgeschlossen.

Es wird kein GitHub-Release allein aufgrund dieses Entwicklungsstands erzeugt.

## Alpha.27 – Befundkorrekturen nach dem ersten Durchlauf

Der Alpha.26-Test endete vor dem Questabschluss: zwei als Fehler markierte Schritte, weitere ungeklärte Punkte und ein zusätzlicher Kollisionsbefund. Speicher-/Checkpointfehler blockierten Inventaraktionen; ein separater Infrastrukturabbruch beendet den Durchlauf, ohne einen Spielabsturz zu beweisen.

Native Savepfade, frühe Schreibvorprüfung, Prüfung der eigentlichen Save-Dateien und eindeutige Abschlussmeldungen korrigiert. Gastdialogübergang freigegeben, Partnerkollision entfernt, NPC-Physik beim Trennen wiederhergestellt, Sprint-Partyguard auf beide PCs erweitert, identische eindeutige Spawnanker berücksichtigt. Wesentliche Ereignisse und Sitzungsanfang haben eigene Logaufbewahrung; Protokollanzeige korrigiert. Testplan durch konkrete Marker-Anleitung ergänzt.

**2.228 technische Assertions und Win32-Build bestanden. Native Abnahme der Korrekturen offen.** Protokoll 9, Journal 4, Checkpointformat 2 unverändert; kein Fertig-Release. [Details](docs/ALPHA-27.md).

### Alpha.30 – Installationsnachprüfung

Inhaltskennung über PowerShell-/Windows-Kollationsgrenzen vereinheitlicht; doppelte Inhaltsdateien abgewiesen; 20 Kultur-/Reihenfolgeregressionen. Expliziter Headless-Testlauf für Windows-Dienstsitzungen. Technischer Zwei-PC-Abgleich erfolgreich, native Spielabnahme offen.

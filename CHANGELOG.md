# Versionshistorie

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

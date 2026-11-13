# Alpha.30 – vollständiger Plugin-Check und Befundkorrekturen

Stand: 13. November 2026. Protokoll 10, Journal 4, Save-Checkpoint 2. Technisch geprüft; gemeinsame native Spielabnahme offen. Keine Spiele gestartet und keine VM-, RAM-, Grafik- oder Treibereinstellungen geändert.

## Korrigierte Befunde

| Befund | Änderung | Technischer Nachweis |
|---|---|---|
| Einzelnes Bodenitem nahe dem Suchradius wird als mehrdeutig verworfen | Abstand zum zweiten echten Kandidaten statt zu einem künstlichen Radius-Platzhalter prüfen; Radius bleibt unverändert | Einzelkandidat bei 45/49 cm, Außenbereich und nahe Konkurrenz geprüft |
| Doppelte eindeutige Drop-Namen werden nach Listenreihenfolge ausgewählt | Auch bei Drop-Tags genau einen Kandidaten verlangen | Gleicher Tag zweimal wird abgewiesen |
| Speicherpause nur gesendet, aber vor nativer Speicherung noch nicht bestätigt | Vor nativer Speicherung höchstens eine Sekunde auf bestätigte Pause und vorherige zuverlässige Nachrichten warten; andernfalls vor nativem Schreiben abbrechen | Erstes UDP-Paket verworfen, Wiederholung, verspätete Vorgänger-ACKs und gleichzeitige Saveankündigungen geprüft |
| Speichern berücksichtigt offene Inventarvorgänge des Partners unvollständig | Geöffnete Gastinventare und Öffnungsanfragen in Bereitschaft einbeziehen; lokalen und entfernten Blocker getrennt anzeigen | Offline-Erstsave, fehlende Partnerbereitschaft, lokale/entfernte Transfers und Partnerpause geprüft |
| F8 während Partner-Speichern nicht erreichbar | Trennung vor dem frühen Warten-Rücksprung verarbeiten | Kontrollfluss geprüft; Transporttrennung/Pausenbereinigung automatisiert geprüft; native Taste noch im Spiel abnehmen |
| Veralteter Text nach Partner-Speicherpause | Beginn/Ende protokollieren und Anzeige nach Ende zurücksetzen | Source-/Buildprüfung; Sichtprüfung im Spiel offen |
| Erfolgsmeldung behauptet trotz Trennung eine aktive Sitzung | Tatsächlichen Verbindungszustand nach Saveprüfung anzeigen und loggen | Kontrollfluss/Build; nativer Save noch abnehmen |
| Abgebrochene Savevorprüfung entfernt Schutzmarker ungeprüft | Entfernen prüfen; Windowsfehler sichtbar sperren statt späteren Ladevorgang unbemerkt zu blockieren | Fehlender Marker und echte Sharingverletzung in isoliertem Verzeichnis geprüft |
| Blockierte Logrotation kann Größenlimit umgehen | Appendposition korrekt ermitteln, begrenzte Wiederholungen und unabhängiges Ereignisprotokoll bei blockierter Hauptrotation | Fehler vor Korrektur reproduziert; danach Archiv gesperrt, Größenlimit und Ereigniserhalt erfolgreich geprüft |
| Plugininstallation kann bei fehlender Laufzeit zu spät scheitern | Vor jeder lokalen Dateikopie Union, benötigte Abhängigkeiten und 4-GB/LAA-Basis prüfen | Beide lokalen Zielinstallationen geprüft |

## Durchgeführter Rundumcheck

- 2.391 Netzwerktest-Assertions plus Diagnose-/Konfigurationsprüfungen erfolgreich. Win32/v142-Build und Menüstruktur bestehen.
- Bestehende Journal-, Saveformat-, Transaktions-, Loot-, Handels-, Quest-, Bewegungs-, Fokus-, Schadens-, Reconnect- und Bereichsregressionen mit ausgeführt. Das ist technische Prüfung, keine vollständige native Spielabnahme.
- Die echten unterbrochenen Journalpaare erneut in isolierten Kopien erfolgreich geprüft; Originale erhalten.
- Native G2-ABI-/Hookeinträge, NPC-Template, Körper-/Kopf-/Textur- und Animationsressourcen, Sprint-/Quicklootbrücken und Cavalorn-Bytecodeprofil geprüft.
- 89 Dateien gegen die eigene GOG-Hashdatenbank geprüft: keine fehlt. Einzige erwartete Abweichung ist die unterstützte 2.6-fix-/LAA-EXE. Die ursprüngliche GOG-EXE ist durch die vorhandene Sicherung belegt; gegen die bekannte 2.6-fix-Basis unterscheiden sich nur LAA-Schalter und PE-Prüfsumme.
- Installierte Basis bleibt erhalten. Persönliche Optionen, Save-/State-/Loginhalte werden beim lokalen Update gesichert und gegen Hashes geprüft.
- Alpha.29-Recoverykorrekturen und bestätigte Kollisionskorrektur bleiben enthalten. Keine neuen VM-Zugriffe; Client-VM bleibt auf ihrem bisherigen Stand.

## Grenzen und nächster Test

Die Änderung behauptet keine Behebung von Windows-Bluescreens oder des Unraid-Neustarts. Es wurden keine Spiele geöffnet. Der kurze Bestätigungs-Wartepfad läuft auf dem Engine-Thread, pollt nur den Transport und startet bei fehlender Bestätigung keinen nativen Save. Seine konkrete Menü-/Savebedienung muss noch im Spiel abgenommen werden.

Vor gemeinsamem Test beide echten Installationen auf identischen Stand bringen. Zuerst C09: einzelne gewöhnliche Bodenfunde nacheinander. Danach Quickloot separat sowie C06/C07. A07: nur eine Person speichert, dann bewegen und erneut aufnehmen; Rollen tauschen. Während einer Partnerpause einmal F8 prüfen und anschließend regulär wieder verbinden. Nicht gleichzeitig speichern. Bei fehlender Bestätigung Anzeige notieren und später erneut versuchen. Keine absichtlichen Systemabstürze für die Recoveryprüfung provozieren.

Laden und Weltwechsel trennen weiterhin; allgemeines gemeinsames Speichern/Laden mit einem Klick ist nicht freigegeben. Andere Questadapter und eine vollständige Kampagne bleiben außerhalb der aktuellen Abnahme.

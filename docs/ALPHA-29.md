# Alpha.29 – Korrekturen am Plugin nach dem Aufhebetest

Stand: 13. November 2026. Protokoll 10, Journalformat 4, Checkpointformat 2. Technische Prüfung bestanden; erneute gemeinsame Spielabnahme offen.

## Behoben

- Unterbrochene Journalveröffentlichung: Hauptdatei und `.pending` werden beim Pluginstart getrennt validiert. Nur gleiche Profile, unveränderte Transaktionsdaten/Ordnung und ein zulässiger einzelner Fortschritt werden akzeptiert. Ein vollständiger neuer Commit wird veröffentlicht; identische oder sicher ältere temporäre Stände werden archiviert. Vorhandene Belege bleiben in `.interrupted-<ID>.main/.pending` erhalten. Beschädigte, widersprüchliche oder nicht eindeutig zuordenbare Stände sperren den Inventarabgleich ohne Überschreiben der Originale. Ein fehlender Hauptstand mit nichtleerem Pending erfordert weiterhin gezielte Klärung.
- Fehlerhafte Ablehnung nach Verlust eines Hosteintrags: Eine Clientrückfrage über einen bereits bestätigten Commit wird vor jeder Bodenitem-/Ablegebehandlung abgefangen. Es wird kein neuer Abort erfunden. Ein Terminalzustand in einer neuen Request-Nachricht wird ebenfalls abgewiesen.
- Doppelte Bestätigungen schreiben nicht mehr unverändert erneut das gesamte Journal. Die Idempotenz bleibt erhalten; unnötige dauerhafte Schreibvorgänge entfallen.
- Nach einem Schreibfehler bleibt der Store für weitere Entscheidungen geschlossen, bis er neu geprüft wurde. Fehler nennen Operation und Windows-Code; ungültige Journale nennen die betroffene Datei.
- Beim Zurücksetzen unterbrochener lokaler Vorgänge wird zuerst die aktive Speicherstand-Zeitlinie geprüft. Ein neues Spiel verändert keine alten fremden lokalen Vorbereitungen mehr.
- Bodenitems protokollieren jetzt den tatsächlichen Sperrgrund statt pauschal einen fehlenden Checkpoint zu melden.
- Die veraltete Initialisierungsmeldung Alpha.27 wurde zusammen mit Ressourcen- und Overlayversion auf Alpha.29 korrigiert.

## Diagnose ergänzt

`recovery_store_begin/end`, `recovery_write_begin/end`, `recovery_flush_begin/end`, `recovery_replace_begin/end` und `recovery_io_error` grenzen Persistenzschritte ein. Transaktions-ID, Rolle, Entscheidung, Pfad, Dauer und Fehlercode werden je nach Schritt erfasst. `ground_request_begin/queued`, `ground_decision_begin`, `ground_effect_begin/end` und `recovery_result_begin/applied` unterscheiden Antrag, Hostentscheidung, Weltänderung und Clientanwendung. `recovery_store_open`, `recovery_journal_pending/recovered/conflict/invalid` zeigen Wiederherstellungen und Sperren.

Die Protokolle bleiben begrenzt; es wurde kein zusätzlicher synchroner Hardware-Flush pro Paket eingeführt. Der erforderliche dauerhafte Journal-Flush bleibt bestehen. Ein harter Gesamtausfall kann weiterhin letzte Logzeilen verlieren.

## Prüfung und Grenzen

- 2.342 technische Assertions bestanden, einschließlich unterbrochener Veröffentlichung, falscher Profile, CRC-/Teil-Dateien, Konflikten, Wiederholungen, Reihenfolge der Schreibmarken und echter Windows-Sharingverletzung.
- Die beiden gesicherten Original-Journalpaare aus dem ausgefallenen Durchgang wurden zusätzlich in isolierten lokalen Kopien geprüft. Host-Commit und bereits bestätigter Client-Commit bleiben erhalten; ein zweiter Start ist sauber. Originalbelege unverändert.
- Win32/v142-Build, Menüstruktur und zusätzliche native ABI-/QoL-/Cavalorn-Prüfungen bestanden. Keine Spiele gestartet.
- Die vom Nutzer bestätigte Kollisionskorrektur bleibt erhalten. Diese Version beansprucht keine Behebung von Windows-Bluescreens oder des Unraid-Neustarts. Keine RAM-, VM-, Grafik- oder Treiberkonfiguration wird verändert.
- Der aktuelle Auftrag lässt die VM außen vor. Host und lokale Clientkopie werden aktualisiert; die tatsächliche Clientinstallation muss später auf denselben Stand gebracht werden. Alte und neue Inhaltskennungen dürfen nicht gemischt werden.

## Nächster kurzer Spieltest

Erst nach identischem Versions-/Inhaltsstand auf beiden PCs. Zunächst volle Bereitschaft abwarten, C09 markieren und je einen gewöhnlichen Bodenfund nacheinander aufnehmen. Der Fund muss einmal verschwinden und dem richtigen Inventar gutgeschrieben werden. Danach Quickloot separat, anschließend C06/C07 für Ablegen/Übergeben. Nach A07 jeweils nur eine Person speichern lassen und ohne Reconnect erneut aufnehmen. Unterbrochene Transfers zunächst technisch, nicht durch absichtlich provozierten Rechnerabsturz prüfen. Bei einer Recovery-Sperre keine Journaldateien bereinigen: Uhrzeit/Marker notieren und alle Logs einschließlich `.events`, `.begin` und Journalbelegen sichern.

Die Wiederherstellung eines Journals ist keine Wiederherstellung ungespeicherten Spielverlaufs. Aktiver Save und gespeicherte Effekte bestimmen weiterhin, was in der Spielwelt angewendet werden darf.

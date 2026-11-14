# Alpha.28 – Befundkorrekturen aus Cavalorn Take 2

Stand: 8. November 2026. Technisch geprüft; gemeinsame native Spielabnahme offen.

## Befund und Änderung

- Acht Aufhebeversuche wurden vor jeder Transaktion wegen fehlender Speicherstandbindung blockiert. Ein wirklich neues Spiel bekommt jetzt nach Abschluss des Weltladens eine eigene leere Inventar-Zeitlinie. Diese wird beim regulären Speichern als Checkpoint dauerhaft gebunden. Ungeklärte oder fehlerhafte Ladebindungen werden nicht freigeschaltet: Der Koop-Start wird mit Diagnosehinweis abgewiesen. Ungespeicherter Fortschritt bleibt bis zum Speichern flüchtig.
- Speichern ruft nicht mehr den allgemeinen Sitzungsabbau auf. Geordnete zuverlässige Transportmeldungen kündigen die Speicherpause und deren Ende an. Auf der Gegenseite wartet die Verarbeitung der Spielzustände; Inventaraktionen sind während der Pause gesperrt. Nach dem Speichern wird derselbe Verbindungszustand fortgesetzt.
- Während einer angekündigten Speicherpause gelten höchstens 120 Sekunden statt des normalen kurzen Netzwerk-Timeouts. Beim Fortsetzen werden Bestätigungsfristen offener zuverlässiger Nachrichten neu angesetzt. Eine verlassene Pause endet weiterhin mit einer klaren Trennung; der Schutz gilt nicht unbegrenzt. Sendekapazität für die Fortsetzung wird reserviert.
- Speichern während offener Transfers, offener Gastcontainer oder einer empfangenen Partnerspeicherpause wird mit Hinweis abgewiesen. In ruhiger Situation nacheinander speichern; keine gleichzeitigen Speicheraktionen erzwingen. Bereits laufende native Schreibvorgänge behalten die Prüfsummen-, Dateifortschritts- und Checkpointprüfung aus Alpha.27.
- Gothic reaktiviert die Kollisionsflags beim Einfügen eines NPCs in die Welt. Der Partner wird deshalb nach dem Einfügen und vor jeder Netzwerkplatzierung wieder kollisionsfrei gesetzt. Zusätzlich lehnt die menschliche KI Kollisionen ausschließlich zwischen lokalem Spieler und Partner ab. Namensfokus und Kampfregeln bleiben separat erhalten.
- Partner-Zustandslogs enthalten jetzt die tatsächlichen statischen/dynamischen Kollisionsflags. Frische Spielinitialisierung, gesperrter Sitzungsstart und Beginn/Ende der Speicherpause werden explizit protokolliert.
- Die veraltete feste Alpha.26-Anzeige ist auf Alpha.28 korrigiert. Protokoll 10 verhindert gemischte Sitzungen mit dem alten Speicherverhalten. Journalformat 4 und Checkpointformat 2 bleiben erhalten.

## Nachweise und Grenzen

2.250 technische Assertions bestehen, darunter echte UDP-Tests für eine 20 Sekunden blockierte Spielinstanz beim Speichern, Nachrichtenaufbewahrung ohne Duplikate, Fortsetzen ohne Reconnect, beide Rollen, Disconnect während Pause und die 120-Sekunden-Grenze. Menüstruktur und Win32/v142-Build bestehen. Kein Spiel wurde gestartet.

Take 2 bestätigte Yunas Xardas-Gespräch. Nur Yuna speicherte; ihr Slot und Checkpoint waren gültig. Daraus folgt keine Bestätigung eines Host-Speichervorgangs oder erfolgreicher Loottransaktionen. Nach dem Save fehlen im erhaltenen Log Reconnect und weitere erfasste Aufnahmeversuche.

Die Fehlerkorrekturen sind implementiert. Kollisionsfreiheit, Itemaufnahme und Speicherung mit Erhalt der Verbindung müssen nun nativ mit beiden Spielern bestätigt werden. Keine pauschale Abnahme von Handel, Questabschluss, Höhle oder Crash-Recovery.

## Kurzer Wiederholungstest vor der Route

1. Auf beiden PCs Alpha.28 starten. Frischen Testzweig oben bei Xardas oder zusammengehörige Teststände verwenden. Freie Slots und reguläre Sicherungen beibehalten.
2. Verbinden, volle Bereitschaft abwarten. Bei einer Start-/Recovery-Sperre Ursache dokumentieren, keine Schutzdateien löschen.
3. A05 markieren: In beide Richtungen durch den Partner laufen, auch auf der Treppe und beim Springen. Namen und Gegnerfokus kontrollieren.
4. C01/C02 passend markieren: Nacheinander je einen Bodenfund aufnehmen. Genau ein Item muss vom Boden verschwinden und im richtigen Inventar erscheinen. Dann Quickloot getrennt testen. Bei einer Sperre nicht weiter zur Quest laufen.
5. A07 markieren: Nur Yuna speichert in einen freien Slot, während Horst nichts ausführt. Kein Reconnect; danach beide bewegen und je einen weiteren Fund aufnehmen. Anschließend nur Horst speichern lassen und wiederholen. Keine offenen Inventarfenster oder laufenden Transfers.
6. Erst nach diesen Prüfungen die verbleibende Cavalorn-Checkliste fortsetzen. Ergebnisse und Uhrzeit je Spieler festhalten. Bei jeder Auffälligkeit gleiche Schrittmarke erneut setzen.

Laden eines Speicherstands und Wechsel der Welt trennen die Sitzung weiterhin absichtlich. Die Änderung betrifft reguläres Speichern, nicht allgemeines gemeinsames Laden oder weltübergreifendes Spielen.

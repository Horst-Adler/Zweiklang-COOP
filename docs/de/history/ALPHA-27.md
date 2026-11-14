# Alpha.27 – Korrekturen nach dem ersten Cavalorn-Test

Alpha.26: 16 Haken bestanden, zwei Fehler, 13 unklar, 34 offen; zusätzlich Kollision unter einem grünen Haken. Der Durchgang endete vor dem Questabschluss. Ein externer Unraid-Abbruch ist kein nachgewiesener Spielabsturz.

## Änderungen

- Native Save-Verzeichnisse über `zCOption::GetDir(...)->GetFullPath()` statt über den veränderlichen Prozess-Arbeitsordner auflösen. Pfad, Slot und Windows-Fehler werden geloggt. Sichere Slotkomponente prüfen und fehlende Verzeichnisse vorbereiten.
- Vorprüfung mit eigenem Schreibprobe-File vor dem GameManager-Speichervorgang. Ein Vorprüfungsfehler lässt die Engine keine Metadaten/Spielstandteile schreiben. Der Probe-File wird sofort wieder entfernt.
- Wiederherstellungsmarker vor dem nativen Schreiben setzen. Abschluss erst nach Rückkehr des nativen Schreibers und geänderten, nicht leeren Charakter-/Weltdateien sowie neuem Save-Header bestätigen. Fehler nicht mehr als „Save completed“ melden. Unverwaltete direkte Native-Saves werden außerhalb des internen Current-Slots sicher abgelehnt.
- Direkte native Ladevorgänge zusätzlich erfassen; verschachtelte Load-Hooks erzeugen keine doppelte Bindung.
- Konkrete lokale/entfernte Recovery-Sperre hat Vorrang vor der allgemeinen Aufforderung zu speichern. Die Schutzsperre wird nicht umgangen.
- Bodenitemblockaden mit Item und Ursache protokollieren. Aufhebeversuch und tatsächliches natives Ergebnis unterscheiden; der alte feste `result=0`-Platzhalter entfällt.
- Partnerdarstellung ohne physische statische/dynamische Kollision. Namensfokus, Sichtbarkeit, Schadenserfassung und Gegnerangriffe im Spiel erneut prüfen.
- Gastdialog: kurze, auf den angefragten NPC begrenzte KI-Freigabe beim nativen Gesprächsversuch; Gegenrichtung der Gesprächsreferenz berücksichtigen. Während des Gesprächs überschreiben Hostzustände dessen native Bewegung nicht. Abgelehnte Versuche und Disconnect löschen die Freigabe. Dialoge werden geloggt. Keine allgemeine gemeinsame Dialogwiedergabe.
- Gespiegelte NPCs erhalten ihre ursprüngliche Physikeinstellung beim Trennen zurück. Neu erzeugte Welt-NPCs behalten ihre Dialogwahrnehmung; autonome KI bleibt durch die Spiegel-Hooks begrenzt.
- Sprint-Partyiteration auf beiden PCs während der Koop-Schutzphase unterdrücken. Der lokale Held behält Sprint, Partner-Overlays werden vom Plugin übertragen. Normale NPC-Begleiter bekommen in dieser Phase keinen eigenen automatischen Party-Sprint.
- NPCs mit eindeutig identischem Spawnanker auch bei engen gleichartigen Gruppen zuordnen. Mehrere identische Anker bleiben gesperrt; keine geratene Bindung. Unaufgelöste Fälle loggen Instanz und Kandidatenzahl.
- Wesentliche Level-1-Ereignisse separat in `session-*.events.jsonl` mit eigener begrenzter Rotation sichern. Zusätzlich bewahrt `.begin.jsonl` den ersten MiB der Sitzung. Paket-Debugrotation verdrängt diese Belege nicht. Gemeinsam mit Hauptlog und `.faults.jsonl` exportieren; doppelte Einträge über Sitzungsdatei und Sequenz erkennen.
- Diagnose zeigt die tatsächliche Protokollkonstante 9.
- Cavalorn-Einleitung durch Marker-Anleitung ersetzt; 65 IDs und alte Ergebnisse erhalten. A02/A07 verlangen bestätigte neue Testslots; A05 enthält expliziten Kollisionscheck, C09 zuerst Einzelaufnahmen, danach Konkurrenz.

## Prüfung und Grenzen

Win32/v142-Build und 2.228 technische Assertions einschließlich echter Loopback-UDP-Prüfungen bestanden. Neue Prüfungen decken wechselnden Arbeitsordner, neue Save-Verzeichnisse, ungültige Slots, Dateikonflikte, Metadaten ohne neue Nutzdaten, leere Nutzdaten, priorisierte Sperrgründe, Dialogfreigabe/Timeout, eindeutige bzw. identische Spawnanker, nichtphysische Partnerpositionierung und die getrennte Logaufbewahrung ab.

Diese Prüfungen sind keine native Spielabnahme. Xardas-Gastdialog, physisches Durchlaufen, Gasttreffer/Aggro, komplette Loot-Wege und ein echter Speicher-/Ladezyklus müssen auf beiden PCs geprüft werden. Ein im Save abgebrochener Engineprozess bleibt über den vorhandenen Recovery-Marker blockiert; keine automatische Behauptung einer reparierten Speicherung.

## Wiederholung

Die Save-Slots des Alpha.26-Abbruchs wurden nur gesichert, nicht umgeschrieben oder als repariert eingestuft. Für den nächsten Durchgang beide einen frischen Testzweig mit freien Slots verwenden. Zuerst Speichern/Laden, Bereitschaft und einzelne Bodenitems oben bei Xardas prüfen. Bei einer Save-/Recovery-Sperre nicht bis zur Höhle weiterlaufen.

Marken: Eine Person wählt vor jedem Spielschritt die ID mit Strg+Umschalt+N/B und setzt sie mit M. M wählt danach automatisch die nächste ID. Bei Fehlern dieselbe ID erneut auswählen und markieren, Uhrzeit/PC/Aktion notieren. D zeigt die Diagnose. Website-Haken werden weiter manuell gespeichert.

Der eigene Startbildschirm bleibt Beta. Protokoll 9, Journal 4 und Checkpointformat 2 bleiben unverändert. Beide PCs benötigen dieselbe Alpha.27-Runtime und ContentId. Kein Spiel durch Codex gestartet.

## Nachrangige, noch nicht zugeordnete Enginewarnungen

Die Alpha.26-Logs enthielten zusätzlich fehlende Soundkennungen, den Waypoint `OC_RAMP_07` und eine Cutscene ohne Player. Eine sichtbare Fehlfunktion oder Verbindung zum Saveblocker ist dafür nicht belegt. Originalskripte und Spielressourcen wurden deshalb nicht auf Verdacht ersetzt. Nach dem gezielten Wiederholungstest anhand der neuen Marker erneut einordnen.

Der Infrastrukturabbruch wurde nicht repariert oder als Spielproblem behandelt. Die neuen Diagnose-Dateien reduzieren den Verlust früher wichtiger Meldungen; bei einem abrupten Prozessende kann die letzte ungeflushte Debugzeile weiterhin fehlen.

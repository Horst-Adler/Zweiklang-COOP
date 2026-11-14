# Alpha.31 – persönliche Charaktere und besondere Spielzustände

Technischer Stand: Netzwerkprotokoll 11, maximal 1.024 Bytes pro UDP-Datagramm, Journal 4, Recovery-Checkpoint 2 und zusätzliche Charakterdatei Format 1. Gemeinsame native Abnahme offen. Kein Gothic gestartet. Der anschließende Auftrag zu optionalen Erweiterungen ist noch nicht umgesetzt.

## Die vier Alpha-Punkte

| Punkt | Im Quellcode umgesetzt | Noch im Spiel nachzuweisen |
|---|---|---|
| Verantwortlichkeit bei Straftaten | Getrennte NPC-Einstellungen, temporäre Einstellungen, Kennen-/Absolutionszustände und 29 persönliche Straf-/Buchhaltungszähler und zehn NPC-Erinnerungsfelder. Host bewertet persönliche Wahrnehmungsereignisse; Client meldet eigene Dialogänderungen. Verschachtelte Host-/Gastreaktionen erhalten getrennte Daten. Aufheben prüft die native Eigentümerwahrnehmung beim tatsächlichen Akteur. | Sichtbare Reaktion von Zeugen/Wachen, Strafen und Begnadigung, friedlicher unbeteiligter Partner, Dialoge bei verschiedenen Fraktionen. Kein pauschaler Nachweis aller Story-/Fraktionsskripte. |
| Lehrer und permanente Boni | Aktueller Held und zurückbehaltene Spielerformen sind von NPC-Spiegelung ausgeschlossen. Talente, Lernpunkte, Level und Erfahrung werden nur auf die unabhängige Partnerfigur geschrieben. Echte Gilde getrennt von aktueller Gestalt/Gilde. Gezielte lokale Fortschrittslogs. | Einzelnes Training und einzelner permanenter Bonus je Spieler; Kosten und Änderung nur beim ausführenden Charakter, einschließlich nach Rückverwandlung und Laden. |
| Besondere Bewegungen | Klettern-/Leiter-, Schwimm-, Tauch-, Sprung- und Fallanimationen haben Vorrang vor einer gleichzeitig laufenden Laufanimation. Begrenzter Abgleich der benötigten Bewegungs-Overlays. Positionshoheit bleibt beim tatsächlichen Spieler; eigener Umweltschaden bleibt lokal. | Leitern, unterschiedliche Kletterhöhen, Wasser/Tauchen, Sprünge, Stürze und Übergänge auf beiden Ansichten. |
| Verwandlungen und Beschwörungen | Tiermodell/Körper/Skalierung übertragen; menschliches Aussehen nicht auf Tierformen schreiben. Formwechsel vor NPC-Registrierung schützen und ausgediente Formen freigeben. Acht reguläre Beschwörungsvarianten über eigene Spawn-Brücke, Host-KI, Besitzer-/Charakterkennung, eindeutige Gruppenmitglieder und Team-Schutz bei Friendly Fire aus. Kein zweites Bezahlen von Mana/Schriftrolle. | Alle unterstützten Formen und Rückverwandlung, je ein eigener und gegnerischer Schaden, Beschwörungen/Gruppen, Folgen/Aggro/Tod/Lebensdauer/Disconnect und Speichern/Laden. Eigene Zauber anderer Mods sind nicht automatisch unterstützt. |

## Sicherheit und Persistenz

- Zusätzliche Anwendungsschlange bei voller zuverlässiger Transportwarteschlange; kein künstlicher Timeout nur durch kurzfristigen Rückstau. Grenzen bleiben aktiv.
- Persönliche Daten an Charakter-/Speicherprofil und nativen Save-Fingerprint gebunden. `ZWEIKLANG_ACTORS.BIN` wird atomar und mit Prüfsumme geschrieben, nachdem die nativen Welt-/Charakterdateien überprüft wurden und bevor der Recovery-Checkpoint abgeschlossen wird.
- Fehlende Charakterdatei bei älteren Alpha-Saves bedeutet ein bisher noch nicht getrenntes Beziehungsprofil. Frühere persönliche NPC-Historien lassen sich daraus nicht zuverlässig rekonstruieren; für die Abnahme einen neuen Testzweig verwenden. Vorhandene Saves werden bei der Installation nicht umgeschrieben.
- Ausstehende persönliche Änderungen, Dialogbestätigungen und Zählerabgleiche verhindern einen voreiligen Save. Laden und Weltwechsel trennen weiterhin; ein vollständiges gemeinsames Kampagnenspeichern ist damit nicht nachgewiesen.
- Charakterdateien werden im Diagnoseexport mitgesichert. Originale, Logs und persönliche Konfiguration beim Update erhalten.
- Wiederhergestellte Gastbeschwörungen warten auf den Besitzer; ein anderer Charakter übernimmt sie nicht. Beim Disconnect werden zugehörige temporäre Gastwesen über den nativen Spawnmanager entfernt. Hosteigene Wesen bleiben beim Host. Native Lifecycle-Abnahme offen.

## Technische Prüfung

Die bestehende vollständige Testsuite und neue Charakter-/Archiv-/Codec-/Eigentümerregressionen ausführen. Zusätzlich `scripts/Check-CharacterNative.py`: zehn native Hookeinträge, SDK-Signaturen, acht Beschwörungsinstanzen, korrekte Float-Radien der Originalskripte und Schreibschutz des lokalen Helden. Ein technischer Check ersetzt weder den Union-Skriptparser beim Spielstart noch die Zwei-PC-Spielabnahme.

Die Paketgrenze wurde wegen zusätzlicher Gestalt-, Talent- und Eigentümerdaten auf 1.024 Bytes angehoben. Beide PCs benötigen denselben Build und Protokollstand. Der Transport selbst wird weiterverwendet.

## Reihenfolge der Abnahme

Zuerst den bisherigen Turm-/Cavalorn-Test wiederholen. Neue Sonderfälle danach in einem eigenen Testzweig prüfen; fehlende Zauber oder permanente Gegenstände nicht als bestandene Tests werten. Details: [Charakter-Testliste](../../TEST-ALPHA31-CHARAKTERE.md).

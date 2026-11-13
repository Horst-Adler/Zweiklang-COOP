# Zweiklang Coop – Test von Xardas bis Cavalorn

Diese Liste gilt für Alpha.27 / Protokoll 9 nach bestätigter Installation auf beiden PCs. Alpha.26 wurde wegen Inventarproblemen und eines externen Unraid-Abbruchs nicht abgeschlossen. Alpha.27 enthält Korrekturen; die erneute Spielabnahme steht aus. Während des Tests steuert ihr beide die Spiele selbst. Zuerst den Turm-Teil prüfen, anschließend nur bis Cavalorn und zur Banditenhöhle gehen.

## Logmarkierungen: wann und wie setzen?

Eine Marke ist ein Lesezeichen in unseren Spielprotokollen. Sie verbindet eine Aktion mit der Schritt-ID dieser Checkliste, damit Codex später beide PCs vergleichen kann. Sie verändert keine Items, Quests oder Spielstände und hakt hier nichts automatisch ab.

| Tastenkombination | Was passiert? |
|---|---|
| `Strg` + `Umschalt` + `D` | Zusätzliche Diagnosezeilen ein-/ausblenden. Dort steht auch die nächste Marker-ID. |
| `Strg` + `Umschalt` + `N` | Die nächste Schritt-ID auswählen, ohne eine Marke zu schreiben. |
| `Strg` + `Umschalt` + `B` | Eine Schritt-ID zurückgehen, ohne eine Marke zu schreiben. |
| `Strg` + `Umschalt` + `M` | Die ausgewählte ID jetzt markieren. Danach wird automatisch die nächste ID ausgewählt. |

### So macht ihr es bei jedem Spielschritt

- **Normalerweise markiert nur Host für beide.** Beide müssen verbunden sein. Vor der Aktion zeigt Host die Diagnose mit Strg + Umschalt + D an und prüft „nächste Marke“.

- Die passende ID mit N oder B wählen. Beispiel: Vor dem Xardas-Gespräch von Gast muss dort **B05** stehen.

- **Direkt vor Beginn** einmal Strg + Umschalt + M drücken und alle Tasten wieder loslassen. Nicht gedrückt halten. Bei erreichbarem Partner erhält Gast dieselbe Markierung mit gemeinsamer Kennung.

- Jetzt führt die genannte Person den Schritt aus. Beispiel: Nach der Marke B05 spricht Gast Xardas an. Host beobachtet.

- Ergebnis in der eigenen Testverwaltung eintragen: bestanden, Fehler, unklar oder ausgelassen. Bei Fehlern Uhrzeit, PC, Item/NPC und tatsächliches Verhalten notieren. Danach „Ergebnisse speichern“ drücken.

### Wenn ein Fehler auftritt oder ihr etwas wiederholt

Bei einem Fehler sofort noch einmal dieselbe ID markieren: Nach der ersten Marke steht bereits die nächste ID da, daher mit B zurückgehen, dann M drücken. Beispiel: B05 vor dem Gespräch; bei Fehlschlag wieder B05 markieren. So liegt die problematische Aktion zwischen zwei Marken. Bei einer Wiederholung ebenfalls vorher dieselbe ID auswählen und markieren. Nicht versehentlich den nächsten Testpunkt markieren.

Wenn M im Dialog, Inventar oder Menü nicht reagiert, keine Tastenfolge erzwingen: Aktion regulär beenden, dieselbe ID markieren und die beobachtete Uhrzeit notieren. Vor einem Reconnect zuerst den Fehler markieren. Nach dem Reconnect beide wieder volle Bereitschaft abwarten lassen und dieselbe ID erneut markieren. Ohne Verbindung bleibt eine Marke lokal; Gast kann dann dieselbe ID selbst setzen und das in der Notiz vermerken. Die Uhrzeit allein ersetzt keine gemeinsame Marker-Kennung.

### Wann braucht ihr keine Marke?

Für Vorbereitungen bei geschlossenem Spiel (A01/A02) keine Marke setzen. Für A03 nach erfolgreichem Beitritt markieren und die Verbindungszeit notieren. Für A04 bis I04 jeweils vor der eigentlichen Spielaktion markieren. Bei einem Punkt mit mehreren Teilaktionen reicht normalerweise die Startmarke; bei einer auffälligen Teilaktion dieselbe ID zusätzlich markieren und den Teil benennen. Bei I04 weiterhin keinen Absturz absichtlich auslösen.

**Speichern trennt die Sitzung:** Vor A07, E01 und H01 markieren, beide speichern lassen, dann Host starten/Gast verbinden. Erst bei „Bereit: Kampf, Loot und Cavalorn-Quest“ weitermachen. Ein neues Vorschaubild allein bestätigt keinen erfolgreichen Save. Bei einer Speicher-/Recovery-Sperre abbrechen, Uhrzeit und ID festhalten und die Logs sichern.

**Neuer Durchgang mit Alpha.27:** Dieser öffentliche Testplan enthält keine gespeicherten privaten Ergebnisse. Einen eigenen Durchgang mit neuen Ergebnissen anlegen. Oben Datum, Build und Zeiten ergänzen. Die alten Abbruchsaves sind kein bestätigter neuer Ausgangsstand; für die Wiederholung neue Testslots verwenden. Nach Ende auf beiden PCs regulär beenden und die Diagnose exportieren, damit alle Logdateien einschließlich Ereignis- und Anfangsprotokoll erhalten bleiben.

## A – Vorbereitung, beide oben in Xardas’ Turm

- [ ] A01 Beide haben Alpha.27 / Protokoll 9 samt denselben QoL-Dateien. Debug-Logging aktiv. Vor dem Verbinden die Sprint-Einstellungen prüfen: Stärkegrenze/Perma/Animation notieren. Quickloottaste prüfen (Standard RMB). Spiele zum Installationsabgleich zunächst geschlossen.
- [ ] A02 Vorhandene Spielstände sind gesichert. Für Alpha.27 beide einen frischen Testzweig oben bei Xardas beginnen und vor dem Verbinden jeweils in einem neuen, freien Slot speichern. Danach die neuen Teststände laden. Abbruchsaves aus Alpha.26 nicht als bestätigten Ausgangsstand verwenden. Keine unabhängig fortgeschrittenen Saves mischen.
- [ ] A03 Host startet und lädt zuerst; Gast startet, lädt und verbindet sich anschließend. Uhrzeit der Verbindung notieren: __________.
- [ ] A04 Beide sehen „Connected“ und anschließend „Bereit: Kampf, Loot und Cavalorn-Quest“, den richtigen Namen und die erwarteten Figuren. Diagnose mit Strg+Umschalt+D umschalten; ID A04 auswählen und mit M markieren. Keine Fehlfunktion der ursprünglichen M/B-Tasten durch die Kombinationen.
- [ ] A05 Beide bleiben 20 Sekunden stehen. Keine doppelten Partner, verschwundenen Köpfe oder dauerhaft laufenden Animationen. Danach vorsichtig aneinander vorbeilaufen und einmal gegeneinander springen: kein Schieben und kein Versinken im Boden; Gegnerkontakt später separat prüfen.
- [ ] A06 Ausgangswerte notieren: Host HP/Gold/EP __________; Gast HP/Gold/EP __________. Bei späteren Zahlen immer den aktuellen Stand unmittelbar vor dem jeweiligen Test verwenden.
- [ ] A07 Zwei getrennte Ausgangssaves in neuen Slots anlegen, etwa `TEST_START_H` und `TEST_START_C`. Jeweils regulär speichern, nicht während einer Inventaraktion. Nach der vorgesehenen Trennung erneut verbinden und vollständige Bereitschaft prüfen. Bei Speicherfehler oder Recovery-Sperre hier abbrechen; ein neues Vorschaubild genügt nicht.

## B – Bewegung, Friendly Fire und Gespräch bei Xardas

- [ ] B01 Host läuft 30 Sekunden, dreht sich und bleibt mehrmals stehen; Gast beobachtet. Danach führt Gast den ersten Teil aus und Host beobachtet. Kein Festhängen, Nachlaufen auf der Stelle oder Teleportieren.
- Zusatz B01: Auf sicherem Platz normales Laufen und dauerhaftes Sprinten vergleichen; mindestens zweimal starten/stehen bleiben, dann führt Gast den ersten Teil aus und Host beobachtet. Passende Sprintanimation auf beiden PCs? __________. Falls ihr die Animation im Patch-Menü ändert, auch diese Variante prüfen. Normales Gehen und Steuerung müssen weiterhin funktionieren.
- [ ] B02 Beide gleichzeitig durch den Raum und die Treppe bewegen. Partner bleibt sichtbar und verschwindet nicht an Türen oder Treppen. Zusätzlich auf BEIDEN PCs: ohne gezogene Waffe den Partner ansehen/anvisieren und korrekten Namen prüfen. Bei Friendly Fire AUS anschließend Waffe, Fäuste und Magie (soweit verfügbar) ziehen: Partner soll nicht als Kampfziel gewählt werden, normale Gegner weiterhin schon. Danach Waffe wieder wegstecken und Namensfokus erneut prüfen.
- [ ] B03 Friendly Fire ist aus. Jeder versucht einmal, den Partner mit gezogener Nahkampfwaffe anzuvisieren und einmal zu treffen. Erwartet: kein Partner als Kampfziel und keine verlorenen Partner-HP. Nicht in Xardas hinein schlagen.
- [ ] B04 Host spricht mit Xardas, Gast beobachtet und geht einige Schritte weg. Danach Gespräch regulär beenden. Xardas erscheint nur einmal; das Gespräch blockiert keine Bewegung des anderen Spielers.
- [ ] B05 Gast spricht mit Xardas; Host beobachtet. Danach regulär beenden. Kein Einfrieren und keine dauerhaft blockierten Dialoge. Fehlende identische Dialogoptionen separat als Quest-/Dialoglücke notieren.
- [ ] B06 Während Gast mit Xardas spricht, Gast über die normale Koop-Option trennen. Falls das Menü im Dialog nicht erreichbar ist: Dialog beenden, trennen und diesen Spezialfall als ausgelassen markieren. Keine erzwungene Prozessbeendigung.
- [ ] B07 Host beobachtet 20 Sekunden: Xardas bleibt ansprechbar und führt passende Animationen aus. Bewegung prüfen, falls er tatsächlich losläuft; ein absichtlich stehender NPC beweist keine behobene Bewegungspause. Gast verbindet sich wieder; keine zweite Xardas-Figur. Zeitpunkt: __________.

## C – Behälter und Bodenitems im Turm

Wählt gewöhnliche, stapelbare Gegenstände ohne Questfunktion. Testitems und Anfangsmengen: __________. Behälter: __________. Für das Gitter im Labor zunächst nur Host den Kamin-Knopf bedienen lassen; eine abweichende Gitterstellung bei Gast gesondert als Weltobjekt-/Schalterlücke erfassen.

- [ ] C01 Host öffnet einen erreichbaren Behälter, notiert Inhalt und schließt ihn. Gast öffnet denselben Behälter: gleiche Restmengen.
- [ ] C02 Gast nimmt genau ein Testitem und schließt. Host öffnet erneut: Item fehlt dort; Gast besitzt genau ein zusätzliches Exemplar.
- [ ] C03 Gast legt ein gewöhnliches Item ein und schließt. Host findet es, entnimmt es und schließt. Gast sieht anschließend den leeren Platz beziehungsweise die richtige Restmenge.
- [ ] C04 Beide versuchen denselben einfachen Behälter gleichzeitig per Quickloot zu leeren; anschließend regulär öffnen und Gesamtmengen vergleichen. Danach einmal einer regulär, einer per Quickloot. Erwartet: geregelter Zugriff oder nachvollziehbare Sperre, keine doppelte Beute. Bei skriptgesteuerter Kiste regulären Weg nutzen und Schnelltest an einfacher Kiste nachholen.
- [ ] C05 Falls ein passendes Schloss samt regulärem Schlüssel vorhanden ist: Gast öffnet es auf normalem Weg, schließt; Host prüft den Schlosszustand. Ohne passende Voraussetzungen auslassen, keine Cheats verwenden.
- Zusatz C02–C05: Mindestens eine bekannte Quelle mit mehreren gewöhnlichen Stapeln durch Gast schnell leeren, danach Rollen tauschen. Namen/Mengen der eingeblendeten Gaben gegen Inventar und Restbestand prüfen. Gast darf nacheinander bestätigte Stapel erhalten; Host hält die Taste für mehrere Stapel. Keine stehen gebliebene Bereitschaftssperre oder unerwarteten Inventarfenster.
- [ ] C06 Host lässt ein gewöhnliches Item fallen. Gast sieht genau ein Weltobjekt und hebt es auf. Auf beiden PCs verschwindet es; Host hat es nicht mehr, Gast hat es einmal.
- [ ] C07 Rollen tauschen: Gast lässt ein Item fallen, Host hebt es auf. Gleiche Prüfung.
- [ ] C08 Einen kleinen Stapel fallen lassen und aufheben; beide prüfen die tatsächlich abgelegte Menge. Die Gesamtmenge bleibt erhalten. Die Darstellung desselben Weltstapels auf zwei PCs zählt dabei nur als ein gemeinsamer Stapel.
- [ ] C09 Zuerst hebt jeder allein ein gewöhnliches Bodenitem auf: Animation, Verschwinden des Items und Inventarzuwachs müssen passen. Erst danach versuchen beide gleichzeitig, dasselbe weitere Bodenitem aufzuheben. Genau einer erhält es; danach bleibt keine erneut aufhebbare Kopie zurück.
- Zusatz C06–C09: Aufnahmen jeweils auch per Quickloot ausführen, einmal im Sprint. Im Waffenmodus darf RMB weiterhin dem Kampf dienen. Keine Aufnahme aus Menüs, Dialogen oder bei Bewusstlosigkeit. Bei Sperre Uhrzeit und angezeigten Grund erfassen.
- [ ] C10 Ein Item ablegen und zunächst liegen lassen. Gast trennt sich und verbindet sich wieder. Item liegt weiterhin genau einmal dort; anschließend aufnehmen.
- [ ] C11 Kontrollierte Wiederherstellung: Gast entnimmt ein Testitem und trennt sich unmittelbar danach regulär. Zeitpunkt und sichtbaren Zustand notieren. Nach Wiederverbindung beide Inventare und Quelle prüfen: weder Verlust noch Verdopplung.
- [ ] C12 C11 mit Einlagern wiederholen. Ob tatsächlich eine noch offene Transaktion getroffen wurde, wird später im Log geprüft; allein ein schneller Reconnect beweist das nicht.
- Zusatz C11/C12: Einmal während einer Schnellloot-Folge regulär trennen. In der Trennung keine Items aufnehmen; auch bei gehaltener Quickloottaste darf der ungeprüfte Kopierpfad nicht weiterlaufen. Nach Reconnect Restbestände und erhaltene Mengen vollständig prüfen, anschließend gegebenenfalls erneut looten. Markierungen in beiden Logs vergleichen.

## D – Getrennt den Turm verlassen

- [ ] D01 Host bleibt zunächst im Turm. Gast geht allein auf dem normalen Weg Richtung Cavalorn, vermeidet vorerst Kämpfe und meldet seine Position. Kein Reconnect zum Beheben von Stottern verwenden.
- [ ] D02 Gast beobachtet unterwegs erreichbare NPCs/Gegner: sichtbar, normale Animationen, keine Doppelgänger. Nicht absichtlich mehrere Gegnergruppen aktivieren.
- [ ] D03 Host verlässt den Turm erst nach etwa einer Minute und folgt. Bei Wiedersehen stimmen Partnerposition und Animationen wieder überein.
- [ ] D04 Auf einem sicheren Abschnitt Rollen tauschen: Gast wartet, Host geht voraus. Beide Gebiete bleiben bedienbar. Uhrzeit der größten Trennung notieren: __________.
- [ ] D05 Falls auf dem Weg ein schwacher, sicher erreichbarer Gegner vorhanden ist: Host hält Abstand, Gast nähert sich allein. Gegner reagiert auf Gast und verfolgt ihn. Andernfalls beim Banditenkampf prüfen; nicht dafür vom Weg abweichen.

Diese Strecke ist ein erster Test zweier Aktivierungsgebiete. Sie ersetzt später keinen Test mit einem Spieler in der Stadt und dem anderen weit außerhalb.

## E – Cavalorn und Questannahme

- [ ] E01 Vor dem Gespräch beide HP, Gold und EP notieren: Host __________; Gast __________. Zwei neue Saves `VOR_CAVALORN_H/C` erstellen, keine älteren Slots überschreiben.
- [ ] E02 Beide sehen genau einen Cavalorn am passenden Ort. Darstellung und Position vergleichen.
- [ ] E03 Host führt zuerst das Gespräch und nimmt die Aufgabe gegen die Banditen an, soweit regulär verfügbar. Gast greift noch nichts an und führt kein paralleles Gespräch.
- [ ] E04 Beide prüfen ihr Tagebuch. Erwartet: „Cavalorns Banditen“ läuft auf beiden PCs. Host vorhanden? __________; Gast vorhanden? __________. Beide Tagebücher wieder schließen, bevor Host fortfährt.
- [ ] E05 Gast öffnet optional Cavalorns Gespräch und beendet es wieder. Questdialoge sind in dieser Alpha bei Host; Gast kann keine zweite Annahme oder Belohnung auslösen. Kein Hängenbleiben oder dauerhaft blockierter NPC bei Host. Ergebnis: __________.
- [ ] E06 Host fragt vor dem Aufbruch optional nach Ausrüstung. Beide erhalten gegenüber ihrem vorherigen Bestand je ein Messer und zwei Heiltränke. Deltas Host __________; Gast __________. Danach keine zweite Gabe durch Reconnect/erneutes Gespräch.
- Zusatz zu E06: Nach dem Gabenabgleich beide neue Saves `VOR_BANDITEN_H/C` anlegen und erneut verbinden. Ein erneuter Kampfversuch mit diesen Saves ist nur vorgesehen, solange danach kein Questabschluss und keine weiteren dauerhaften Loot-/Transfer-Vorgänge protokolliert wurden. Während des Kampfs zunächst nicht plündern; den älteren Stand vor den Gaben nicht zum Neustart verwenden.
- [ ] E07 Wenn Cavalorn mitkommt: Beide beobachten, ob er auf beiden PCs dieselbe Route nimmt und dieselben Gegner angreift. Falls er nicht mitkommt, genaue letzte Dialogoption notieren und weiter prüfen, soweit der Kampf sicher möglich ist.

## F – Banditenkampf und Höhle

Vorher feste Rollen vereinbaren: Host beginnt beim ersten regulär erreichbaren Banditen; Gast beginnt beim nächsten. Keine Cheats oder zusätzlichen Gegner zum Test holen. Bei drohendem Tod Sicherheit vor Testvollständigkeit.

- [ ] F01 Host trifft zuerst einen Banditen, Gast beobachtet: beide sehen denselben Gegner und einen übereinstimmenden HP-Rückgang.
- [ ] F02 Gast trifft einen noch lebenden Banditen, Host beobachtet: Treffer verursacht Schaden auf beiden PCs. Uhrzeit: __________.
- [ ] F03 Ein Bandit reagiert auf Gast, verfolgt und trifft sie. Nur des Gasts HP sinken durch diesen Treffer; des Hosts HP bleiben unverändert, sofern er nicht selbst getroffen wird.
- [ ] F04 Wenn sicher möglich: Gegner trifft Host. Nur dessen eigene HP sinken. Keine weitergeleiteten Verletzungen an den Partner.
- [ ] F05 Beide greifen denselben Banditen an. Schäden werden zusammengeführt; kein wechselndes Zurückspringen der HP. Friendly Fire bleibt aus.
- [ ] F06 Cavalorns Angriffe, falls er beteiligt ist, wirken auf denselben Gegnerbestand. Keine unabhängig lebende Banditenkopie auf dem anderen PC.
- [ ] F07 Bei jedem Tod prüfen: auf beiden PCs tot, keine weiter kämpfende Figur und kein erneutes Aufstehen ohne regulären Spielgrund. Falls ein NPC nur bewusstlos ist, dies getrennt notieren.
- [ ] F08 Beide gehen in den Höhlenbereich und prüfen alle bekämpften Banditen. Gleiche Anzahl besiegter Gegner und keine zusätzliche aktive Kopie.
- [ ] F09 Host plündert eine Leiche teilweise und schließt. Gast öffnet dieselbe Leiche: nur Restbestand. Danach plündert Gast eine weitere Leiche und Host prüft deren Restbestand, falls eine weitere Leiche verfügbar ist.
- [ ] F10 Beide versuchen dieselbe Leiche gleichzeitig zu plündern. Keine doppelte Beute; eine Zugriffssperre darf verständlich eingreifen.
- [ ] F11 Falls in der Höhle ein zugänglicher Behälter oder Bodenitem vorhanden ist: Gast entnimmt zuerst, Host prüft den Rest. Andernfalls als ausgelassen markieren.
- [ ] F12 Kampf-EP und übrige Änderungen notieren: Host __________; Gast __________. Fehlende gemeinsame EP gesondert erfassen; perfekte EP-Teilung ist noch keine zugesagte Funktion dieses Builds.

## G – Questabschluss und Wiederkehr

- [ ] G01 Host schließt die Aufgabe mit Cavalorn regulär ab. Der Abschlussdialog kann bereits automatisch nach dem Kampf beginnen. Zeitpunkt/Option __________. Falls bereits abgeschlossen, keine zweite Abgabe erzwingen. Fehlt der Abschluss, beide Bereitschaft und tatsächlichen Zustand aller Banditen notieren.
- [ ] G02 Beide Tagebücher zeigen den Abschluss. Beide erhalten je 100 Quest-EP; eine native Gabe von 50 Gold an Host wird auch Gast vollständig gegeben. Fehlt diese bedingte Goldgabe bei Host, wird sie nicht erfunden. Deltas Host __________; Gast __________. Kampf-EP separat betrachten; die exakten Questgaben stehen in den Quest-Logs.
- [ ] G03 Gast kann keine zweite Questabgabe auslösen. Wiederverbindung und ein erneutes Gespräch zwischen Host und Cavalorn erzeugen keine weitere Questbelohnung. Bestände vor/nach __________. Die begonnene Vatras-Folgequest ist noch kein weiterer gemeinsamer Testfall.
- [ ] G04 Beide verlassen kurz den unmittelbaren Höhlenbereich und kehren zurück, ohne neue Gegner anzulocken. Tote Banditen bleiben tot; geplünderte Quellen bleiben entsprechend leer. Eine größere Entfernung ist später separat zu testen.
- [ ] G05 Gast trennt sich und verbindet sich wieder. Partner, Cavalorn, Banditenzustände, verbliebene Beute und Inventarmengen erneut prüfen. Keine wiederbelebten Gegner oder doppelte Beute.

## H – Speichern, kompletter Neustart und Ende

- [ ] H01 Außerhalb von Dialog, Kampf und Inventaraktion auf beiden PCs neue Abschlusssaves erstellen, etwa `NACH_HOEHLE_H/C`. Beide Speicherungen abwarten; Zeitpunkt __________.
- [ ] H02 Gast regulär trennen; beide Spiele regulär beenden. Keine Prozessbeendigung erzwingen.
- [ ] H03 Host startet, lädt seinen Abschlusssave; Gast startet, lädt ihren dazugehörigen Abschlusssave und verbindet sich. Keine Ausgangs- und Abschlusssaves mischen.
- [ ] H04 Inventare, Gold und persönliche HP passen zum jeweiligen Save. Bereits entnommene Items werden nicht erneut gutgeschrieben; abgelegte Restitems existieren weiterhin genau einmal.
- [ ] H05 Banditen bleiben tot und Leichen/Behälter haben den gespeicherten Restinhalt. Cavalorn wird nicht dupliziert. Queststand jedes PCs mit seinem zuvor notierten Stand vergleichen; fehlende gemeinsame Questpersistenz separat festhalten.
- [ ] H06 Beide regulär beenden. Danach Diagnoseexport auf des Hosts und des Gasts PC ausführen, soweit vorhanden. Alle rotierenden Logs mitnehmen, nicht nur die letzte Datei. Erst nach Sicherung der Logs einen weiteren langen Versuch beginnen.

## I – Bewusst noch offen für einen separaten Test

- [ ] I01 Händlerkauf: Gast kauft ein Stück, Gold wird einmal abgezogen, Item einmal gegeben, gemeinsamer Händlerbestand sinkt. Danach Kauf durch Host gegenprüfen.
- [ ] I02 Händlerverkauf: Gast verkauft ein gewöhnliches Item; Item verschwindet einmal, Gold kommt einmal hinzu, Host sieht aktualisierten Händlerbestand. Gegebenenfalls Dialog neu öffnen.
- Zusatz zu I01/I02: Einen unbezahlbaren Kauf versuchen. Erwartet: verständliche Ablehnung; anschließend sind ein bezahlbarer Kauf und Looten weiterhin möglich. Bei Bedarf neben I01 notieren.
- [ ] I03 Gleichzeitiger Händlerzugriff sowie Reconnect nach Kauf/Verkauf, danach Gold und Warenbestand vergleichen. Ohne regulär erreichbaren Händler auf dieser Route auslassen.
- [ ] I04 Alter Speicherstand, unterbrochene Transfers und echte Absturz-Wiederherstellung: technische Vorprüfung durch Codex; kontrollierter Ingame-Test später mit separaten Sicherungen. Für diesen Durchgang keinen Absturz erzwingen und nicht versuchsweise alte Saves über den neuen Endstand laden.

## Rückmeldung zum Kopieren

```
Datum / Build Host / Build Gast:
Startzeit / Endzeit:
Bestanden: A..., B..., ...
Fehler: Nummer | Uhrzeit | Host/Gast/beide | erwartet | tatsächlich
Ausgelassen oder unklar: Nummer | Grund
Absturz/Freeze: Uhrzeit | letzter Schritt | auf welchem PC
Reconnect nötig: Uhrzeit | warum | danach besser oder unverändert
Quest Host: angenommen / abgeschlossen / Belohnungsänderung
Quest Gast: angenommen / abgeschlossen / Belohnungsänderung
Inventar-/Goldabweichungen:
Logs/Diagnosearchive und Screenshots:
```

Bei einem Freeze, Absturz, dauerhaften HP-Widerspruch oder Itemverlust den aktuellen Abschnitt abbrechen, Zeitpunkt festhalten und Logs sichern. Bis zur Klärung keine guten Ausgangssaves überschreiben. Normale Quest-/Belohnungslücken ohne blockiertes Spiel könnt ihr dokumentieren und den restlichen Ablauf fortsetzen.


## Vorrangiger Wiederholungstest Alpha.28

Vor der Route Partnerkollision, einzelne Bodenaufnahmen und anschließend ausschließlich Gast speichern, danach erneut aufnehmen; dann ausschließlich Host speichern und wiederholen. Verbindung soll jeweils erhalten bleiben. Offene Inventarvorgänge vor dem Speichern beenden. Laden trennt weiterhin. Weitere Details: [Alpha.28](ALPHA-28.md).


## Alpha.29: Vorrangiger Plugin-Test

Nach Versionsabgleich zuerst einzelne Bodenaufnahmen, Quickloot separat und Itemübergaben prüfen. Journalwiederherstellung wird in isolierten technischen Tests geprüft; keinen Rechnerabsturz erzwingen. Bei Sperren Logs und Journalpaare erhalten. [Änderungen und Grenzen](ALPHA-29.md).


## Alpha.30 vor dem Routentest

Einzelne Bodenfunde und Quickloot getrennt; anschließend nacheinander speichern und weiter aufnehmen. Fehlende Partnerbestätigung muss vor nativem Schreiben abbrechen. F8 während Partnerpause prüfen. Keine Systemabstürze erzwingen. [Befunde und Grenzen](ALPHA-30.md).

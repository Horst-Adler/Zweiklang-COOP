# Zwei-PC-Test: Xardas bis Cavalorn

Alpha.26. Die 65 IDs entsprechen den Diagnosemarken. Dies ist eine Testanleitung, kein bestandener Testbericht. Host und Gast testen mit gesicherten, passenden Spielständen derselben Welt. Der Host führt die Cavalorn-Questdialoge.

Speichern/Laden trennt die Sitzung. Beide speichern manuell, starten danach Host/Gast erneut und warten auf vollständige Bereitschaft. Nicht auf alte Saves vor bereits gebuchten Transfers oder Questgaben zurückspringen.

A: Vorbereitung; B: Bewegung/Dialog/FF; C: Beute; D: Abstand; E: Cavalorn; F: Banditen; G: Abschluss; H: Speichern/Neustart; I: separate Sondertests.

- [ ] A01 Beide haben Alpha.26 / Protokoll 9 samt denselben QoL-Dateien. Debug-Logging aktiv. Vor dem Verbinden die Sprint-Einstellungen prüfen: Stärkegrenze/Perma/Animation notieren. Quickloottaste prüfen (Standard RMB). Spiele zum Installationsabgleich zunächst geschlossen.

- [ ] A02 Vorhandene Spielstände sind gesichert. Beide verwenden einen vereinbarten, zusammenpassenden Ausgangsstand oben bei Xardas. Keine unabhängig fortgeschrittenen Saves mischen.

- [ ] A03 Host startet und lädt zuerst; Gast startet, lädt und verbindet sich anschließend. Uhrzeit der Verbindung notieren: __________.

- [ ] A04 Beide sehen „Connected“ und anschließend „Bereit: Kampf, Loot und Cavalorn-Quest“, den richtigen Namen und die erwarteten Figuren. Diagnose mit Strg+Umschalt+D umschalten; ID A04 auswählen und mit M markieren. Keine Fehlfunktion der ursprünglichen M/B-Tasten durch die Kombinationen.

- [ ] A05 Beide bleiben 20 Sekunden stehen. Keine doppelten Partner, verschwundenen Köpfe oder dauerhaft laufenden Animationen.

- [ ] A06 Ausgangswerte notieren: Host HP/Gold/EP __________; Gast HP/Gold/EP __________. Bei späteren Zahlen immer den aktuellen Stand unmittelbar vor dem jeweiligen Test verwenden.

- [ ] A07 Zwei getrennte Ausgangssaves anlegen, etwa `TEST_START_H` und `TEST_START_C`. Jeweils regulär speichern, nicht während einer Inventaraktion.

- [ ] B01 Host läuft 30 Sekunden, dreht sich und bleibt mehrmals stehen; Gast beobachtet. Danach führt Gast den ersten Teil aus und Host beobachtet. Kein Festhängen, Nachlaufen auf der Stelle oder Teleportieren.

- [ ] B02 Beide gleichzeitig durch den Raum und die Treppe bewegen. Partner bleibt sichtbar und verschwindet nicht an Türen oder Treppen. Zusätzlich auf BEIDEN PCs: ohne gezogene Waffe den Partner ansehen/anvisieren und korrekten Namen prüfen. Bei Friendly Fire AUS anschließend Waffe, Fäuste und Magie (soweit verfügbar) ziehen: Partner soll nicht als Kampfziel gewählt werden, normale Gegner weiterhin schon. Danach Waffe wieder wegstecken und Namensfokus erneut prüfen.

- [ ] B03 Friendly Fire ist aus. Jeder versucht einmal, den Partner mit gezogener Nahkampfwaffe anzuvisieren und einmal zu treffen. Erwartet: kein Partner als Kampfziel und keine verlorenen Partner-HP. Nicht in Xardas hinein schlagen.

- [ ] B04 Host spricht mit Xardas, Gast beobachtet und geht einige Schritte weg. Danach Gespräch regulär beenden. Xardas erscheint nur einmal; das Gespräch blockiert keine Bewegung des anderen Spielers.

- [ ] B05 Gast spricht mit Xardas; Host beobachtet. Danach regulär beenden. Kein Einfrieren und keine dauerhaft blockierten Dialoge. Fehlende identische Dialogoptionen separat als Quest-/Dialoglücke notieren.

- [ ] B06 Während Gast mit Xardas spricht, Gast über die normale Koop-Option trennen. Falls das Menü im Dialog nicht erreichbar ist: Dialog beenden, trennen und diesen Spezialfall als ausgelassen markieren. Keine erzwungene Prozessbeendigung.

- [ ] B07 Host beobachtet 20 Sekunden: Xardas bleibt ansprechbar und führt passende Animationen aus. Bewegung prüfen, falls er tatsächlich losläuft; ein absichtlich stehender NPC beweist keine behobene Bewegungspause. Gast verbindet sich wieder; keine zweite Xardas-Figur. Zeitpunkt: __________.

- [ ] C01 Host öffnet einen erreichbaren Behälter, notiert Inhalt und schließt ihn. Gast öffnet denselben Behälter: gleiche Restmengen.

- [ ] C02 Gast nimmt genau ein Testitem und schließt. Host öffnet erneut: Item fehlt dort; Gast besitzt genau ein zusätzliches Exemplar.

- [ ] C03 Gast legt ein gewöhnliches Item ein und schließt. Host findet es, entnimmt es und schließt. Gast sieht anschließend den leeren Platz beziehungsweise die richtige Restmenge.

- [ ] C04 Beide versuchen denselben einfachen Behälter gleichzeitig per Quickloot zu leeren; anschließend regulär öffnen und Gesamtmengen vergleichen. Danach einmal einer regulär, einer per Quickloot. Erwartet: geregelter Zugriff oder nachvollziehbare Sperre, keine doppelte Beute. Bei skriptgesteuerter Kiste regulären Weg nutzen und Schnelltest an einfacher Kiste nachholen.

- [ ] C05 Falls ein passendes Schloss samt regulärem Schlüssel vorhanden ist: Gast öffnet es auf normalem Weg, schließt; Host prüft den Schlosszustand. Ohne passende Voraussetzungen auslassen, keine Cheats verwenden.

- [ ] C06 Host lässt ein gewöhnliches Item fallen. Gast sieht genau ein Weltobjekt und hebt es auf. Auf beiden PCs verschwindet es; Host hat es nicht mehr, Gast hat es einmal.

- [ ] C07 Rollen tauschen: Gast lässt ein Item fallen, Host hebt es auf. Gleiche Prüfung.

- [ ] C08 Einen kleinen Stapel fallen lassen und aufheben; beide prüfen die tatsächlich abgelegte Menge. Die Gesamtmenge bleibt erhalten. Die Darstellung desselben Weltstapels auf zwei PCs zählt dabei nur als ein gemeinsamer Stapel.

- [ ] C09 Beide versuchen gleichzeitig, dasselbe Bodenitem aufzuheben. Genau einer erhält es; danach bleibt keine erneut aufhebbare Kopie zurück.

- [ ] C10 Ein Item ablegen und zunächst liegen lassen. Gast trennt sich und verbindet sich wieder. Item liegt weiterhin genau einmal dort; anschließend aufnehmen.

- [ ] C11 Kontrollierte Wiederherstellung: Gast entnimmt ein Testitem und trennt sich unmittelbar danach regulär. Zeitpunkt und sichtbaren Zustand notieren. Nach Wiederverbindung beide Inventare und Quelle prüfen: weder Verlust noch Verdopplung.

- [ ] C12 C11 mit Einlagern wiederholen. Ob tatsächlich eine noch offene Transaktion getroffen wurde, wird später im Log geprüft; allein ein schneller Reconnect beweist das nicht.

- [ ] D01 Host bleibt zunächst im Turm. Gast geht allein auf dem normalen Weg Richtung Cavalorn, vermeidet vorerst Kämpfe und meldet seine Position. Kein Reconnect zum Beheben von Stottern verwenden.

- [ ] D02 Gast beobachtet unterwegs erreichbare NPCs/Gegner: sichtbar, normale Animationen, keine Doppelgänger. Nicht absichtlich mehrere Gegnergruppen aktivieren.

- [ ] D03 Host verlässt den Turm erst nach etwa einer Minute und folgt. Bei Wiedersehen stimmen Partnerposition und Animationen wieder überein.

- [ ] D04 Auf einem sicheren Abschnitt Rollen tauschen: Gast wartet, Host geht voraus. Beide Gebiete bleiben bedienbar. Uhrzeit der größten Trennung notieren: __________.

- [ ] D05 Falls auf dem Weg ein schwacher, sicher erreichbarer Gegner vorhanden ist: Host hält Abstand, Gast nähert sich allein. Gegner reagiert auf Gast und verfolgt ihn. Andernfalls beim Banditenkampf prüfen; nicht dafür vom Weg abweichen.

- [ ] E01 Vor dem Gespräch beide HP, Gold und EP notieren: Host __________; Gast __________. Zwei neue Saves `VOR_CAVALORN_H/C` erstellen, keine älteren Slots überschreiben.

- [ ] E02 Beide sehen genau einen Cavalorn am passenden Ort. Darstellung und Position vergleichen.

- [ ] E03 Host führt zuerst das Gespräch und nimmt die Aufgabe gegen die Banditen an, soweit regulär verfügbar. Gast greift noch nichts an und führt kein paralleles Gespräch.

- [ ] E04 Beide prüfen ihr Tagebuch. Erwartet: „Cavalorns Banditen“ läuft auf beiden PCs. Host vorhanden? __________; Gast vorhanden? __________. Beide Tagebücher wieder schließen, bevor Host fortfährt.

- [ ] E05 Gast öffnet optional Cavalorns Gespräch und beendet es wieder. Questdialoge sind in dieser Alpha bei Host; Gast kann keine zweite Annahme oder Belohnung auslösen. Kein Hängenbleiben oder dauerhaft blockierter NPC bei Host. Ergebnis: __________.

- [ ] E06 Host fragt vor dem Aufbruch optional nach Ausrüstung. Beide erhalten gegenüber ihrem vorherigen Bestand je ein Messer und zwei Heiltränke. Deltas Host __________; Gast __________. Danach keine zweite Gabe durch Reconnect/erneutes Gespräch.

- [ ] E07 Wenn Cavalorn mitkommt: Beide beobachten, ob er auf beiden PCs dieselbe Route nimmt und dieselben Gegner angreift. Falls er nicht mitkommt, genaue letzte Dialogoption notieren und weiter prüfen, soweit der Kampf sicher möglich ist.

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

- [ ] G01 Host schließt die Aufgabe mit Cavalorn regulär ab. Der Abschlussdialog kann bereits automatisch nach dem Kampf beginnen. Zeitpunkt/Option __________. Falls bereits abgeschlossen, keine zweite Abgabe erzwingen. Fehlt der Abschluss, beide Bereitschaft und tatsächlichen Zustand aller Banditen notieren.

- [ ] G02 Beide Tagebücher zeigen den Abschluss. Beide erhalten je 100 Quest-EP; eine native Gabe von 50 Gold an Host wird auch Gast vollständig gegeben. Fehlt diese bedingte Goldgabe bei Host, wird sie nicht erfunden. Deltas Host __________; Gast __________. Kampf-EP separat betrachten; die exakten Questgaben stehen in den Quest-Logs.

- [ ] G03 Gast kann keine zweite Questabgabe auslösen. Wiederverbindung und ein erneutes Gespräch zwischen Host und Cavalorn erzeugen keine weitere Questbelohnung. Bestände vor/nach __________. Die begonnene Vatras-Folgequest ist noch kein weiterer gemeinsamer Testfall.

- [ ] G04 Beide verlassen kurz den unmittelbaren Höhlenbereich und kehren zurück, ohne neue Gegner anzulocken. Tote Banditen bleiben tot; geplünderte Quellen bleiben entsprechend leer. Eine größere Entfernung ist später separat zu testen.

- [ ] G05 Gast trennt sich und verbindet sich wieder. Partner, Cavalorn, Banditenzustände, verbliebene Beute und Inventarmengen erneut prüfen. Keine wiederbelebten Gegner oder doppelte Beute.

- [ ] H01 Außerhalb von Dialog, Kampf und Inventaraktion auf beiden PCs neue Abschlusssaves erstellen, etwa `NACH_HOEHLE_H/C`. Beide Speicherungen abwarten; Zeitpunkt __________.

- [ ] H02 Gast regulär trennen; beide Spiele regulär beenden. Keine Prozessbeendigung erzwingen.

- [ ] H03 Host startet, lädt seinen Abschlusssave; Gast startet, lädt ihren dazugehörigen Abschlusssave und verbindet sich. Keine Ausgangs- und Abschlusssaves mischen.

- [ ] H04 Inventare, Gold und persönliche HP passen zum jeweiligen Save. Bereits entnommene Items werden nicht erneut gutgeschrieben; abgelegte Restitems existieren weiterhin genau einmal.

- [ ] H05 Banditen bleiben tot und Leichen/Behälter haben den gespeicherten Restinhalt. Cavalorn wird nicht dupliziert. Queststand jedes PCs mit seinem zuvor notierten Stand vergleichen; fehlende gemeinsame Questpersistenz separat festhalten.

- [ ] H06 Beide regulär beenden. Danach Diagnoseexport auf des Hosts und des Gasts PC ausführen, soweit vorhanden. Alle rotierenden Logs mitnehmen, nicht nur die letzte Datei. Erst nach Sicherung der Logs einen weiteren langen Versuch beginnen.

- [ ] I01 Händlerkauf: Gast kauft ein Stück, Gold wird einmal abgezogen, Item einmal gegeben, gemeinsamer Händlerbestand sinkt. Danach Kauf durch Host gegenprüfen.

- [ ] I02 Händlerverkauf: Gast verkauft ein gewöhnliches Item; Item verschwindet einmal, Gold kommt einmal hinzu, Host sieht aktualisierten Händlerbestand. Gegebenenfalls Dialog neu öffnen.

- [ ] I03 Gleichzeitiger Händlerzugriff sowie Reconnect nach Kauf/Verkauf, danach Gold und Warenbestand vergleichen. Ohne regulär erreichbaren Händler auf dieser Route auslassen.

- [ ] I04 Alter Speicherstand, unterbrochene Transfers und echte Absturz-Wiederherstellung: technische Vorprüfung durch Codex; kontrollierter Ingame-Test später mit separaten Sicherungen. Für diesen Durchgang keinen Absturz erzwingen und nicht versuchsweise alte Saves über den neuen Endstand laden.

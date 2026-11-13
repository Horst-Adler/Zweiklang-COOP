# Roadmap

Der Versionsplan bezieht sich auf **Gothic II DNdR mit Union für zwei Spieler**.
Eine technische Implementierung ist nicht gleichbedeutend mit einer erfolgreichen
Spielabnahme. Es gibt keine feste Termin- oder Kampagnenkompatibilitätszusage.

## Bereits belegt

- Eigene Quellcode-/Buildbasis und gemeinsames Host-/Clientplugin.
- Union-Menü und korrigierte Eingabe einschließlich deutschem Tastaturlayout.
- Namenloser Held und Velaya-Profil; Partnergesichter und Animationen korrigiert.
- Verbindung und Reconnect im LAN in früheren Tests bestätigt.
- Alpha.15: Bewegung ohne Reconnect, Gasttreffer, Gast-Aggro und gleiche Gegner-HP bestätigt.
- 2.228 technische Assertions im aktuellen Alpha.27-Code und im separaten
  Veröffentlichungscheckout erfolgreich ausgeführt.

## Alpha – gemeinsamer Kern

### Aktuelle Abnahme

- Zwei-PC-Regression von Xardas bis Cavalorn und Ende der Banditenhöhle.
- Partnerfokus/Friendly Fire und getrennte Spieler-HP prüfen.
- Leichen/Kisten, Einlagern, Schlösser, Bodenstapel und Quickloot prüfen.
- Quellenkonflikte, Abbruch, Wiederverbindung und Recovery überprüfen.
- Hostgeführten Cavalorn-Questadapter mit tatsächlich vollen Gaben abnehmen.
- NPC-Lebenszyklus und zwei Aktivierungsgebiete derselben Welt prüfen.
- NPC-Handel bei einem echten Händler und große Entfernung separat testen.

### Noch zu vervollständigende Grundsysteme

- Allgemeine gemeinsame Quests und berechtigte Questabgabe durch beide Spieler.
- Allgemeine Belohnungsregeln: berechtigte Spieler erhalten je 100 % statt Halbierung.
- Fraktionsquests bei unterschiedlichen Fraktionen getrennt; gemeinsame Fortschritte
  und volle Gaben nur bei gleicher Fraktion. Fraktionswechsel/Berechtigungszeitpunkt definieren.
- Allgemeine Kampf-EP-Vergabe; Reichweite, Abwesenheit und Bewusstlosigkeit klären.
- Quest-/Spezialitems, Zugang zu Schlüsselgegenständen und relevante Skriptfolgen.
- Türen, Gitter, Schalter und geskriptete Interaktionen koordinieren.
- Dialogkonflikte, Begleiter, Zeit/Pause/Schlafen und Niederlagenregeln.
- Koordinierte Welt-/Kapitelwechsel und robuste gemeinsame Speicherkoordination.

**Alpha-Abschluss:** Die vereinbarten Grundabläufe funktionieren reproduzierbar
auf beiden PCs, einschließlich Konsistenz nach Wiederbeitritt und Fortsetzen.
Kritische Abstürze, Duplikate, Verluste und Fortschrittsblockaden werden nach
Befund bearbeitet; automatische Tests allein erfüllen dieses Kriterium nicht.

## Beta – längere Tests und Komfort

- Längere gemeinsame Kampagnenabschnitte und unabhängige Wege/Charakterentwicklung.
- Latenz, Jitter, Verlust, doppelte/vertauschte Pakete, dichte NPC-Gebiete und Last.
- Mehrstündige Sitzungen; Ressourcenverbrauch und mögliche Lecks messen.
- Versionswechsel, unterstützte Formatmigrationen und Rückfallprozeduren.
- **Partner ansprechen und direkt handeln:** eigenes Partner-Dialogmenü mit
  „Handeln“, Items/Gold anbieten, beidseitige Bestätigung und atomare Inventaränderung;
  sichere Behandlung von Dialogabbruch/Disconnect/Entfernung. Geplant, noch nicht implementiert.
- **Zwei männliche und zwei weibliche Aussehen**, inklusive bestehender Held-/Velaya-Basis.
- Braunes Leder-/Stoffoutfit und Startrüstung mit zwei Formen in Xardas’ Laborkiste
  hinter dem Gitter; einmalige Vergabe ohne Wiederholungsduplikate.
- **Eigenes Startbild „Zweiklang COOP“** neu untersuchen. Die Alpha.24/25-Einbauten
  sind nach Darstellungsfehlern zurückgezogen. Native Renderreihenfolge zuerst klären.
- Definierte Adapter-Schnittstellen für spätere Folgeversionen vorbereiten.

Gegenseitige Wiederbelebung, Nachvergütung bei Abwesenheit oder gemeinsame
Story-Abstimmungen bleiben zunächst Regel-/Architekturentscheidungen.
Die optionale Idee weiblicher Spielerdialoge ist keine verbindliche Freigabevoraussetzung.

**Beta-Abschluss:** Stabile längere Spielsitzungen und erweiterte Kampagnentests;
keine offenen kritischen Stabilitäts-, Duplikat-, Verlust- oder Persistenzfehler.

## Vollversion – Freigabe der Gothic-2-Basis

- Unterstützte Spiel-/Union-/Pluginstände ausdrücklich benennen.
- Zwei-PC-Kampagnen-, Kapitel-/Weltwechsel-, Speicher- und Wiederbeitrittsnachweise.
- Reproduzierbare Pakete, eindeutige Versionen/Prüfsummen, Installation und Rückbau.
- Bedienhilfe, Diagnose und bekannte Einschränkungen abschließen.
- Herkunft und zulässigen Verteilungsumfang aller tatsächlich ausgelieferten Dateien prüfen.

### Geplant: eigene Koop-Achievements

- Erster Gegner als Team besiegt, erste gemeinsame Quest abgeschlossen und beide Spieler in Khorinis angekommen.
- Bestätigte Kampf-/Quest-/Gebietsereignisse, Hostabgleich und dauerhafter Fortschritt je Koopprofil; eindeutige Vergaben trotz Reconnect oder Laden.
- Eigenes Erfolgsmenü und optionale Benachrichtigungen. Beteiligung, Fraktionsquests und nachträgliche Vergabe vor Umsetzung definieren; noch nicht implementiert.

**Freigabe:** Nur die ausdrücklich abgenommenen Kombinationen werden unterstützt.
Die Versionsnummer allein ist kein Nachweis einer fertigen gemeinsamen Kampagne.

## Eigene spätere Erweiterungen und Versionen

### Odyssee-Erweiterung

Separate Folgeversion auf dem stabilen G2-Kern: konkrete TC-Version und
Abhängigkeiten erfassen, eigenes Inhaltsprofil sowie Quest-/Welt-/Persistenzadapter
entwickeln und kapitelweise gemeinsam abnehmen. Eigenes Paket und eigene
Kompatibilitätsmatrix; keine derzeitige pauschale Unterstützung.

### Gothic-1-Portierung

Eigener Versionszweig mit passender Engine-/Unionbasis, Build, nativen Schnittstellen,
Figuren, Kampf, Items, Quests und Saveprofilen. G2-Adressen nicht übernehmen.
Separate Zwei-PC-Kampagnen- und Stabilitätsabnahme vor einer eigenen Freigabe.

**Odyssee und Gothic 1 sind keine Abschlusskriterien für die Gothic-2-Vollversion.**

## Unmittelbare Wiederholungsprüfung nach Alpha.26

Zuerst die Alpha.27-Korrekturen für Speicherung, Bereitschaft, Einzelaufnahmen, Gastdialoge und Spielerkollision oben im Turm abnehmen. Dann den unterbrochenen Cavalorn-Ablauf fortsetzen. Frühere grüne Haken ersetzen diese Regression nicht; eine technische Korrektur ist noch keine bestätigte gemeinsame Spielabnahme.


Aktueller Charakterstand: [Alpha.31](docs/ALPHA-31.md), Protokoll 11. Persönliche NPC-Zustände, Schutz lokaler Fortschritte, besondere Bewegungen, Verwandlungen und reguläre Beschwörungen sind technisch implementiert; native Zwei-PC-Abnahme bleibt offen. Zusätzliche [Charakter-Testliste](docs/TEST-ALPHA31-CHARAKTERE.md).


## Bestätigte weitere Ideen

Beta: Partner-Pings und gemeinsame Kartenmarkierungen/Notizen. Vollversion: ausgewählte NPC-Reaktionen auf das Duo und gemeinsames Reisetagebuch. Diese Ideen sind geplant, noch nicht implementiert. Die vier Charakter-Alpha-Punkte sind technisch in Alpha.31 enthalten, native Abnahme offen.

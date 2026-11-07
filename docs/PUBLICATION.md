# Sichere Veröffentlichung

## Umfang dieses Repositorys

Veröffentlicht werden nur eigene Modquellen, eigene Helferskripte,
Projektdateien, synthetische Recovery-Fixtures, Beispielkonfiguration und
neu zusammengestellte deutsche Projektdokumentation.

Bewusst ausgeschlossen:

- vollständige Originalspiel- und Clientinstallationen sowie einzelne Originalspiel-Dateien;
- private Website einschließlich Quellcode, Daten, Builds und Grafiken;
- SDK-Checkouts, fremde Menüskripte, Importbibliotheken und Laufzeitarchive;
- Velaya-/andere TC-Dateien, Renderer und lokal transformierte QoL-Archive;
- lokale Konfigurationen, Save-/Recovery-Dateien und Diagnosearchive;
- Zugangsdaten, Schlüssel, persönliche Pfade/Adressen und Kontaktinformationen;
- temporäre Arbeitsdaten, Caches und Buildprodukte;
- die in die Beta verschobenen Grafikexperimente.

Die ursprünglichen lokalen Arbeitsverzeichnisse werden zur Veröffentlichung
weder gelöscht noch verschoben. Ein separater Checkout vermeidet Änderungen an
den funktionierenden Spiel-/Websiteinstallationen.

## Schutzschichten

1. `.gitignore` schließt Originalspiele, Website, lokale Daten und typische
   Spiel-/Binärformate aus.
2. `publication-allowlist.json` nennt **jeden** ausdrücklich freigegebenen Pfad.
   Die vier synthetischen BIN-Dateien werden zusätzlich anhand ihres Hashes geprüft.
3. `scripts/Audit-Publication.py` prüft Pfade, Größen, Binärsignaturen, erwartete
   Textkodierung und typische Schlüssel-/Privatdatenmuster.
4. `--staged` kontrolliert den **vollständigen** Git-Index, Dateimodi, Konflikte
   und die Übereinstimmung mit den vorher geprüften Arbeitsdateien.
5. Vor dem Push werden vollständiger Diff und vorhandene Historie geprüft;
   nach Veröffentlichung wird der Remote-Baum mit der Positivliste verglichen.

Automatische Mustererkennung ist keine allgemeine rechtliche oder inhaltliche
Freigabe. Herkunft und Inhalt jeder aufgenommenen Datei müssen zusätzlich
bekannt sein. Unklare Ressourcen werden nicht veröffentlicht.

## Kontrollierter Ablauf

```powershell
python .\scripts\Audit-Publication.py
# Danach nur die explizit geprüften Pfade stagen, kein pauschales git add .
python .\scripts\Audit-Publication.py --staged
git diff --cached --check
git diff --cached --stat
```

Die bestehende Remote-Historie bleibt erhalten. Kein Force-Push und kein
heimliches Bereinigen bereits veröffentlichter Inhalte. Falls ausgeschlossene
Dateien in einem bestehenden Commit gefunden werden, Veröffentlichung stoppen,
den Befund dokumentieren und eine gesonderte Freigabe für Historienänderungen abwarten.

## Datenschutz der Diagnose

Die Mod erzeugt zur Fehlersuche lokal Namen, Adressen, Pfade und Ereignisdaten.
Das ist kein Anlass, diese Dateien automatisch hochzuladen. Für öffentliche
Fehlerberichte nur bewusst ausgewählte, bereinigte Ausschnitte verwenden.
Die Beispielkonfiguration und der veröffentlichte Testplan enthalten keine
persönlichen Daten oder gespeicherten Testergebnisse.

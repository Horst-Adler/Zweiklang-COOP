# Mitwirken

Beiträge sind besonders hilfreich, wenn sie konkrete native Befunde mit einem
kleinen nachvollziehbaren Eingriff verbinden. Die Alpha ist noch keine fertig
abgenommene Kampagne; automatische Tests und Spieltests getrennt benennen.

## Fehler melden

Ein [Issue](https://github.com/Horst-Adler/Zweiklang-COOP/issues/new/choose) sollte enthalten:

1. Pluginversion bzw. Commit, Spiel-/Union-/Pluginstände und betroffene Rolle.
2. Ausgangssituation: passende Saves, geladene Welt, Sitzung bereits bereit?
3. Genaue reproduzierbare Schritte und erwartetes/tatsächliches Verhalten.
4. Uhrzeit und Testschritt-ID; relevante bereinigte Logauszüge beider PCs.
5. Ob der Fehler nach Reconnect/Laden verschwindet oder erneut auftritt.

Keine Zugangsdaten, echten IPs, privaten Benutzernamen oder vollständigen privaten
Diagnosearchive ungeprüft veröffentlichen. Originalspielarchive und Spielstände
gehören nicht in öffentliche Issues oder Pull Requests.

## Codebeiträge

- Einen begrenzten Zweck pro Änderung beschreiben.
- Enginezugriffe im Spielthread halten; keine rohen Spielzeiger übertragen.
- Paketlängen, Grenzen, Sitzungen und Wiederholung berücksichtigen.
- Inventar-/Gold-/Queständerungen nicht am Transaktions-/Recovery-Pfad vorbei ausführen.
- Die normale lokale Spielfigur nicht als fremden NPC pausieren oder überschreiben.
- Speicher-/Protokolländerungen versionieren und bestehende Leser/Fixtures beachten.
- Keine neuen Menü-/Renderer-/Modkombinationen ohne passenden Spielnachweis als unterstützt erklären.
- Fremdcode und Ressourcenherkunft vor Aufnahme nachvollziehbar prüfen.

## Vor einem Pull Request

```powershell
.\scripts\Build-Alpha.ps1 -TestsOnly
python .\src\ZweiklangCoop\tests\CheckUnionMenu.py
python .\scripts\Audit-Publication.py
git diff --check
```

Bei nativen Änderungen zusätzlich mit dem separat vorhandenen SDK bauen und den
betroffenen Ablauf auf zwei PCs testen. Falls kein nativer Test möglich war,
genau das angeben. Kein erzwungener Absturz an ungesicherten Spielständen.

Neue veröffentlichbare Dateien bewusst in `publication-allowlist.json` aufnehmen.
Nach explizitem Staging den gesamten Index prüfen:

```powershell
python .\scripts\Audit-Publication.py --staged
```

Die Positivliste ist keine nachträgliche Erlaubnis, fremde SDKs, Originalspiele,
Website-Dateien oder persönliche Daten zu veröffentlichen. Verwendete Abhängigkeiten
lokal beziehen und dokumentieren; unklare Ressourcen nicht hochladen.

Pull Requests über die [Projektseite](https://github.com/Horst-Adler/Zweiklang-COOP/pulls)
öffnen und Problem, resultierendes Verhalten, relevante Tests und verbleibende
Grenzen beschreiben. Keine unbewiesenen Freigabe- oder Kompatibilitätsbehauptungen.

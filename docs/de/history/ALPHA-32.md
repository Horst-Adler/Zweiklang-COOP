# Alpha.32 – Union, Ninja und optionale Komfortmodule

14. November 2026. Technisch implementiert; native Zwei-PC-Abnahme offen. Protokoll 11, Journal 4, Checkpoint 2 und Charakterdatei 1 bleiben erhalten. Kein Gothic gestartet.

- Union bleibt Hauptkern für Netzwerk, Hostautorität, Inventartransaktionen, Recovery, Figuren und Diagnose.
- Eigenes Ninja-Modul `Data/ZweiklangCoop_Core.vdf`: versionierte ABI 1 mit geprüften Befehlen für Händlerbestand, Leichen-/Runeninventar und lokale Quest-EP. Nur Namen auflösen, keine Symbolindizes speichern. Typen, Parameterzahl, Pflichtversion, Archivhash und erfolgreicher Probeaufruf werden geprüft.
- Quickloot 1.9.5 und Sprint 2.4.0 werden nur mit den bereits dokumentierten privaten Koop-Brücken aktiviert. Automatische Erkennung kombiniert Dateihash mit geladenem Ninja-Patch und Brückenrevision. Fehlen beide, läuft der reguläre Aufheben-/Behälter-/Kampfpfad weiter; der eigene Quickloot-Tastaturpfad und Sprint-Overlays bleiben aus.
- Toolkit 16.0 und PatchMenu 1.1.1 sind für den Kern optional. Die untersuchten Komfortmodule brauchen unter Ninja 4 aber Toolkit. Diese bedingte Abhängigkeit bleibt geprüft, statt sie ersatzlos zu entfernen.
- Geladene umbenannte, beschädigte, ungepatchte oder unbekannte Varianten der erfassten Komfortmodule sperren den Koop-Start mit Dateinamen/Grund. Andere fremde Mods sind dadurch nicht automatisch kompatibel.
- Vollständige ContentId bleibt einschließlich optionaler Archive erhalten. Beide PCs brauchen aktuell dasselbe optionale Profil; gemischte Modulbestückung ist nicht freigegeben. Keine Abschwächung des Inhaltsabgleichs.
- Installer installiert standardmäßig nur eigene Runtime und erforderliches Ninja. Komfortmodule ausdrücklich mit `-OptionalModules` auswählen; vorhandene Module werden erhalten und geprüft. Eigene Core-VDF wird gesichert und bei Fehlern zurückgerollt. Nach Profiländerungen ContentId mit dem aktuellen Installer neu berechnen.
- Händler-Skriptbindungen werden zusätzlich bei Fehlerausstieg wiederhergestellt. Der Ninja-Aufruf initialisiert sich auch beim Recovery vor dem ersten Verbindungsstart; Befehlszustand wird bei Rückkehr und Ausnahmen zurückgesetzt, rekursive Aufrufe werden abgewiesen.

3.671 technische Assertions im vollständigen Testlauf, dazu 18 echte Dateiprofilfälle und 20 Inhaltskennungsprüfungen. Das eigene VDF wird deterministisch gebaut, unabhängig gemountet und byteweise gegen die zwei eigenen Quelldateien geprüft. Vorhandene Saves werden bei der Installation nicht umgeschrieben.

Siehe [Architektur und Profile](UNION-NINJA.md). Der anschließende Rundumcheck und Installationsabgleich werden separat dokumentiert. Kein Nachweis, dass der native Ninja-/Union-Parser oder alle Gameplaypfade bereits auf zwei PCs abgenommen wurden.

# Drittanbieter, Herkunft und Lizenzumfang

## Eigenständiger Code

Netzwerktransport, Nachrichtenformate, Engine-Adapter, Recovery-Logik, eigene
Daedalus-Helfer und Projektdateien wurden für Zweiklang COOP neu erstellt.
Die eigene Quellbasis stand vor dieser Veröffentlichung unter **AGPL-3.0-only**;
der vorhandene Lizenztext wird unverändert in [LICENSE](LICENSE) übernommen.
Diese Veröffentlichung legt keine neue Lizenz für fremde Bestandteile fest.

## Externes SDK und Union

Die Kompilierung benötigt Union-/ZenGin-Definitionen und die generische
`UnionAfx.h`-Integration aus einem separat beschafften Referenzcheckout:

- <https://gitlab.com/gothiccoop/gothiccoop>
- Commit `0255381eeb86ea4b888f420fafaa4a10ee4d85e3`
- Weitere historische Referenz: <https://github.com/Gothic-Workshop/GothicCoop>

Der fremde Koop-Spielcode wird nicht als Teil der eigenen Implementierung
kompiliert. SDK-Header, API-Definitionen, Union-/DirectX-Importbibliotheken und
deren jeweilige Rechte bleiben extern. Das gesamte SDK und das fremde
Union-Menüskript werden nicht in diesem Repository weiterverbreitet.

Die Laufzeit wurde aus der Union-Autorenquelle untersucht:
<https://github.com/Gratt-5r2/gothic-fix-archive>.

## Originalspiele und Figuren

Eine eigene Gothic-II-DNdR-Installation ist erforderlich. Zweiklang referenziert
vorhandene Körper, Morphköpfe und Texturen, liefert diese aber nicht mit.
Das weibliche Profil wurde anhand einer lokal installierten Velaya-Fassung
untersucht. Weder die Velaya-TC noch deren Skripte, Welten oder Grafiken werden
hier veröffentlicht. Originalspiele und ihre geschützten Inhalte gehören den
jeweiligen Rechteinhabern.

## Weitere Mods und Laufzeiten

Ninja, Toolkit, Quickloot, Sprint, PatchMenu und GD3D11 sind separat zu beschaffende
Fremdkomponenten. Der öffentliche Quellstand enthält deren Archive oder Binärdateien
nicht. Autorenquellen und untersuchte Versionen: [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md).

Das eigene QoL-Werkzeug veröffentlicht nur die Transformationslogik und eigenen
Kompatibilitätsabzweige. Es liest lokal bereitgestellte Originalarchive und erzeugt
lokale Ausgaben. Die Rechte an darin enthaltenen Fremdskripten und Ressourcen
werden dadurch nicht verändert. Eine pauschale Weiterverbreitungserlaubnis wird
aus dem Besitz einer installierten Datei nicht abgeleitet.

Das Plugin zNoFriendlyFire wurde zur Einordnung der angefragten vorhandenen
Union-Lösung konsultiert: <https://github.com/Gratt-5r2/zNoFriendlyFire>.
Weder seine Implementierung noch seine Binärdatei wird mitgeliefert; Zweiklangs
Partnerschadensschutz ist Bestandteil des eigenen Adapters.

## Plattform und Testdateien

Microsoft Windows SDK, MSVC, Visual-C++-Laufzeit und Windows-Systembibliotheken
sind externe Plattformabhängigkeiten unter ihren eigenen Bedingungen.

Die vier kleinen BIN-Dateien unter `src/ZweiklangCoop/tests/fixtures` sind
synthetische Ausgaben eigener früherer Recovery-Serializer. Sie enthalten keine
Originalspielressourcen und keine privaten Spielstände. Ihre Hashes sind in
der Publikationspositivliste festgehalten.

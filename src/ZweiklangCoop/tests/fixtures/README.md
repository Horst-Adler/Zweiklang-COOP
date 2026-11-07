# Synthetische Recovery-Testdateien

Die vier BIN-Dateien enthalten künstliche Transaktionen des eigenen Serializers,
keine Gothic-Spielstände und keine Originalspielressourcen.

- Recovery17Store.bin / Recovery17Checkpoint.bin: vom unveränderten Alpha.17-Serializer erzeugte Commit-Transaktion; die aktuellen Tests prüfen Lesen und Migration.
- Recovery18Store.bin / Recovery18Checkpoint.bin: vom unveränderten Alpha.18-Serializer erzeugter Händlerverkauf mit Preis-/Währungseffekten.

Die historischen Erzeuger wurden lokal archiviert; die vier festen Eingabedateien
sind Bestandteil der Regressionstests. Eine native Spielstandmigration ist damit
nicht automatisch bewiesen.

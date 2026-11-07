# Selbst kompilieren

## Nachgewiesene Werkzeugkette

- Windows, 32-Bit-Ziel `Win32`.
- Visual Studio 2019 Build Tools bzw. Visual Studio mit **MSVC v142** und Windows-10-SDK.
- C++17, Release-Konfiguration und dynamische Visual-C++-Laufzeit `/MD`.
- MSBuild; `config/buildtools2019.vsconfig` nennt die verwendeten Installationskomponenten einschließlich SDK 19041.
- Python 3 für die Archiv-/Konfigurations-/Publikationswerkzeuge. Der Plugin-Build selbst benötigt Python nicht.

Die Projektdateien wählen `WindowsTargetPlatformVersion=10.0`. Eine andere Compiler-/SDK-Kombination ist damit nicht automatisch als kompatibel bestätigt.

## Externe Union-/ZenGin-Abhängigkeit

Das Repository enthält **kein SDK und keine fremden Importbibliotheken**.
Der dokumentierte Referenzstand ist:

- Repository: <https://gitlab.com/gothiccoop/gothiccoop.git>
- Commit: `0255381eeb86ea4b888f420fafaa4a10ee4d85e3`

Benötigt werden daraus die SDK-Definitionen und -Bibliotheken, insbesondere:

```text
UnionAfx.h
.build_settings.inl
ZenGin/Gothic_II_Addon/
ZenGin/Gothic_UserAPI/
SDK/Union/v1.0l/Include/
SDK/Union/v1.0l/Lib/Shw32.lib
SDK/Union/v1.0l/Lib/Vdfs32g.lib
SDK/DirectX7/include/
SDK/DirectX7/Lib/
```

Der Union-SDK-Stand `v1.0l` im Build und die untersuchte Union-Laufzeit `1.0m`
sind unterschiedliche Angaben und kein erfundener Versionssprung. Die generische
`UnionAfx.h`-Integration und die drei G2A-SDK-Definitionseinheiten werden mitgebaut;
die fremde Koop-Spielimplementierung wird nicht kompiliert.

Ein separat vorhandener Checkout kann direkt über `-SdkRoot` genutzt werden.
Wer die SDK-Referenz beschafft, muss deren eigene Rechte und Voraussetzungen
beachten. Fremde Dateien nicht in den Git-Index dieses Projekts aufnehmen.
Ein geeigneter lokaler, ignorierter Ablageort ist `local/sdk/GothicCoop`.

## Build aus dem Repository-Hauptverzeichnis

```powershell
# Eigenständige Tests ohne Union-/Gothic-SDK:
.\scripts\Build-Alpha.ps1 -TestsOnly

# DLL und Tests mit eigenem, separat beschafftem SDK:
.\scripts\Build-Alpha.ps1 -SdkRoot .\local\sdk\GothicCoop

# Falls MSBuild nicht automatisch gefunden wird:
.\scripts\Build-Alpha.ps1 -SdkRoot .\local\sdk\GothicCoop -MSBuildPath "C:\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
```

Das letzte Beispiel bezeichnet einen selbst gewählten Werkzeugpfad, keine mitgelieferte Datei.

Direkter Projektaufruf als Alternative:

```powershell
MSBuild .\src\ZweiklangCoop\ZweiklangCoop.vcxproj /m /p:Configuration=Release /p:Platform=Win32 /p:Game=G2NotR /p:SdkRoot="D:\SDK\GothicCoop\"
MSBuild .\src\ZweiklangCoop\NetworkTests.vcxproj /m /p:Configuration=Release /p:Platform=Win32
.\build\alpha\tests\ZweiklangNetworkTests.exe
```

Die Test-EXE aus dem Repository-Hauptverzeichnis ausführen, damit die synthetischen
Dateien unter `src/ZweiklangCoop/tests/fixtures` gefunden werden.

## Ausgabe und Prüfungen

| Datei | Bedeutung |
|---|---|
| `build/alpha/G2NotR/ZweiklangCoop.dll` | Eigene Win32-Union-DLL |
| `build/alpha/tests/ZweiklangNetworkTests.exe` | Eigenständige technische Tests |
| `build/logs/plugin.log` | Plugin-Buildprotokoll |
| `build/logs/tests-build.log` | Test-Buildprotokoll |
| `build/logs/tests.log` | Testausgabe |

Die aktuelle Veröffentlichungsvorbereitung wurde mit der genannten lokalen
Werkzeugkette neu gebaut. **2.228 Assertions bestanden**; Menü-Strukturprüfung
und der zusätzliche Python-Check wurden ebenfalls ausgeführt. Die Tests prüfen
unter anderem echte Loopback-UDP-Verbindungen, begrenzte Codecs, Wiederholungen,
Recovery-Entscheidungen, Altformatdateien, Tastatureingaben und Konfiguration.

Optionaler zusätzlicher Strukturcheck:

```powershell
python .\src\ZweiklangCoop\tests\CheckUnionMenu.py
```

Die Tests starten kein Gothic. Für native Engineabläufe bleibt ein gemeinsamer
Zwei-PC-Spieltest erforderlich. Das Quellprojekt enthält zwar historische G1-
Propertygruppen; die aktive Buildvalidierung erlaubt ausdrücklich nur `G2NotR`.
Das ist keine Gothic-1-Freigabe.

## Nach dem Build

Nach [INSTALLATION.md](INSTALLATION.md) installieren. **Einmal bauen und genau
dieselben DLL-/Skriptdateien auf beiden PCs verwenden.** Zwei getrennte Builds
können trotz gleicher Versionsnummer unterschiedliche Binärhashes besitzen;
die Inhaltsprüfung darf dann den Beitritt ablehnen. Buildprodukte und Protokolle
bleiben lokal und sind durch `.gitignore` sowie die Publikationspositivliste ausgeschlossen.

[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [Parameter(Mandatory=$true)][string]$GamePath,
    [string]$QolDirectory,
    [ValidateSet("Quickloot","Sprint","Toolkit","PatchMenu")][string[]]$OptionalModules=@(),
    [string]$UnionMenuScriptPath,
    [ValidateSet('Host','Client')][string]$Role='Host',
    [ValidateLength(1,63)][ValidatePattern('^[^\r\n\x00=]+$')][string]$PlayerName='Spieler',
    [string]$HostAddress='127.0.0.1',
    [ValidateRange(1,65535)][int]$Port=12345
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'ContentIdentity.ps1')
. (Join-Path $PSScriptRoot 'Check-Dependencies.ps1')
$game=[IO.Path]::GetFullPath($GamePath).TrimEnd('\')
$system=Join-Path $game 'System'
$dll=Join-Path $root 'build\alpha\G2NotR\ZweiklangCoop.dll'
foreach($path in @($dll,(Join-Path $system 'Gothic2.exe'),(Join-Path $system 'Shw32.dll'),(Join-Path $system 'Vdfs32g.dll'),(Join-Path $game 'Data\Union.vdf'),(Join-Path $game '_work\Data\Scripts\_compiled\GOTHIC.DAT'))){if(-not(Test-Path -LiteralPath $path -PathType Leaf)){throw ('Voraussetzung fehlt: '+$path)}}
if((Get-Item -LiteralPath (Join-Path $system 'Shw32.dll')).VersionInfo.ProductName -ne 'Union'){throw 'Diese Installation hat keine erkannte Union-Laufzeit.'}
$engineBytes=[IO.File]::ReadAllBytes((Join-Path $system 'Gothic2.exe'))
if($engineBytes.Length -lt 64){throw 'Gothic2.exe ist keine unterstuetzte Spielbasis.'}
$enginePe=[BitConverter]::ToInt32($engineBytes,60)
if($enginePe -lt 0 -or $enginePe+24 -gt $engineBytes.Length -or ([BitConverter]::ToUInt16($engineBytes,$enginePe+22) -band 32) -eq 0){throw 'Der fuer diese Alpha erwartete 4-GB/LAA-Schalter fehlt. Spielbasis vor der Installation vorbereiten.'}
if(Get-Process Gothic2 -ErrorAction SilentlyContinue){throw 'Gothic II muss vor der Installation geschlossen sein.'}
$installed=Join-Path $system 'Autorun\ZweiklangCoop.dll'
if(Test-Path -LiteralPath $installed){$probe=[IO.File]::Open($installed,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);$probe.Close()}
$parsed=$null
if(-not [Net.IPAddress]::TryParse($HostAddress,[ref]$parsed) -or $parsed.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork -or $HostAddress -eq '0.0.0.0'){throw 'HostAddress muss eine konkrete IPv4-Adresse sein.'}
$core=Join-Path $root 'build\alpha\ninja\ZweiklangCoop_Core.vdf'
$coreManifest=Get-Content -LiteralPath (Join-Path $root 'build\alpha\ninja\manifest.json') -Raw|ConvertFrom-Json
if(-not(Test-Path -LiteralPath $core) -or (Get-FileHash -LiteralPath $core).Hash -ne $coreManifest.SHA256 -or $coreManifest.ABI -ne 1){throw 'Eigenes Ninja-Core-Modul fehlt oder wurde nach Build veraendert.'}
$files=@(
    @{Source=$core;Relative='Data\ZweiklangCoop_Core.vdf'},
    @{Source=$dll;Relative='System\Autorun\ZweiklangCoop.dll'},
    @{Source=(Join-Path $root 'src\ZweiklangCoop\scripts\ZweiklangCoop_Menu.d');Relative='System\Autorun\ZweiklangCoop_Menu.d'},
    @{Source=(Join-Path $root 'src\ZweiklangCoop\scripts\ZweiklangCoop_Npc.d');Relative='System\Autorun\ZweiklangCoop_Npc.d'}
)
$menuTarget=Join-Path $game 'System\Autorun\Menu\zUnionMenu.d'
if($UnionMenuScriptPath){
    if(-not(Test-Path -LiteralPath $UnionMenuScriptPath -PathType Leaf)){throw 'Separat bereitgestelltes zUnionMenu.d fehlt.'}
    $files+=@{Source=[IO.Path]::GetFullPath($UnionMenuScriptPath);Relative='System\Autorun\Menu\zUnionMenu.d'}
}elseif(-not(Test-Path -LiteralPath $menuTarget)){throw 'zUnionMenu.d aus der eigenen Union-Installation separat über -UnionMenuScriptPath bereitstellen. Fremdes Skript ist nicht im Repository enthalten.'}
$catalog=Get-Content -LiteralPath (Join-Path $root 'config\dependencies.json') -Raw|ConvertFrom-Json
$selection=@($OptionalModules)
if(@($selection|Where-Object {$_ -in 'Quickloot','Sprint','PatchMenu'}).Count -and 'Toolkit' -notin $selection){$selection+='Toolkit'}
foreach($module in $catalog.Modules){
    if($module.Id -in $selection){
        if(-not $QolDirectory){throw 'Fuer gewaehlte Komfortmodule -QolDirectory mit separat beschafften, vorbereiteten Dateien angeben.'}
        $path=Join-Path $QolDirectory $module.File
        if(-not(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-FileHash -LiteralPath $path).Hash -ne $module.SHA256){throw ('Unbekannte optionale Quelldatei: '+$module.File)}
        $files+=@{Source=$path;Relative=$module.File.Replace('/','\')}
    }
}
if($QolDirectory -and (Test-Path -LiteralPath (Join-Path $QolDirectory $catalog.Ninja.File))){
    $ninja=Join-Path $QolDirectory $catalog.Ninja.File
    if((Get-FileHash -LiteralPath $ninja).Hash -ne $catalog.Ninja.SHA256){throw 'Ninja-Quelldatei nicht unterstuetzt.'}
    $files+=@{Source=$ninja;Relative='System\Ninja.dll'}
}else{
    $ninja=Join-Path $game $catalog.Ninja.File
    if(-not(Test-Path -LiteralPath $ninja) -or (Get-FileHash -LiteralPath $ninja).Hash -ne $catalog.Ninja.SHA256){throw 'Ninja 4.1.16 separat installieren oder -QolDirectory bereitstellen.'}
}
# Validate the effective profile before writing, including retained optional files.
$effective=@()
foreach($module in $catalog.Modules){
    $incoming=@($files|Where-Object {$_.Relative -eq $module.File.Replace('/','\')})
    $path=if($incoming.Count){$incoming[0].Source}else{Join-Path $game $module.File}
    if(Test-Path -LiteralPath $path){if((Get-FileHash -LiteralPath $path).Hash -ne $module.SHA256){throw ('Nicht unterstuetztes beibehaltenes Archiv: '+$module.File)};$effective+=$module}
}
if(@($effective|Where-Object RequiresToolkit).Count -and 'Toolkit' -notin @($effective.Id)){throw 'Aktive Komfortmodule benoetigen Toolkit 16.0.'}
if(-not $PSCmdlet.ShouldProcess($game,'Eigenes Alpha-Plugin und separat bereitgestellte Abhängigkeiten sichern/installieren; ContentId aktualisieren')){return}
$backup=Join-Path $root ('local\install-backups\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $backup -Force|Out-Null
$records=@();$iniPath=Join-Path $system 'ZweiklangCoop.ini'
foreach($relative in (@($files|ForEach-Object Relative)+'System\ZweiklangCoop.ini'|Select-Object -Unique)){
    $target=Join-Path $game $relative;$existed=Test-Path -LiteralPath $target;$saved=Join-Path $backup $relative
    if($existed){New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force|Out-Null;Copy-Item -LiteralPath $target -Destination $saved}
    $records+=[ordered]@{Target=$target;Backup=$saved;Existed=$existed;SHA256=$(if($existed){(Get-FileHash -LiteralPath $saved).Hash}else{$null})}
}
$manifest=Join-Path $backup 'manifest.json';$records|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $manifest -Encoding UTF8
try{
    foreach($entry in $files){
        $target=Join-Path $game $entry.Relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force|Out-Null
        if([IO.Path]::GetFullPath($entry.Source) -ne [IO.Path]::GetFullPath($target)){Copy-Item -LiteralPath $entry.Source -Destination $target -Force}
        if((Get-FileHash -LiteralPath $entry.Source).Hash -ne (Get-FileHash -LiteralPath $target).Hash){throw 'Dateikopie fehlgeschlagen.'}
    }
    $profile=Test-CoopDependencies -GamePath $game -CatalogPath (Join-Path $root "config\dependencies.json")
    $contentFiles=@(
        Get-ChildItem -LiteralPath (Join-Path $game 'Data') -File|Where-Object {$_.Extension -in '.vdf','.mod'}
        Get-ChildItem -LiteralPath (Join-Path $game '_work\Data\Scripts\_compiled') -File
        Get-Item -LiteralPath (Join-Path $system 'Gothic2.exe'),(Join-Path $system 'Shw32.dll'),(Join-Path $system 'Vdfs32g.dll'),(Join-Path $system 'Ninja.dll')
        Get-ChildItem -LiteralPath (Join-Path $system 'Autorun') -File -Recurse|Where-Object {$_.Extension -in '.dll','.d'}
    )|Sort-Object FullName -Unique
    $contentRows=@($contentFiles | ForEach-Object {[ordered]@{File=$_.FullName.Substring($game.Length+1);Bytes=$_.Length;SHA256=(Get-FileHash -LiteralPath $_.FullName).Hash}})
    $contentId=Get-CoopContentIdentity -Rows $contentRows
    if(Test-Path -LiteralPath $iniPath){
        $raw=[IO.File]::ReadAllBytes($iniPath)
        $skip=0
        if($raw.Length -ge 2 -and $raw[0] -eq 255 -and $raw[1] -eq 254){$encoding=[Text.Encoding]::Unicode;$skip=2}
        elseif($raw.Length -ge 3 -and $raw[0] -eq 239 -and $raw[1] -eq 187 -and $raw[2] -eq 191){$encoding=[Text.UTF8Encoding]::new($true,$true);$skip=3}
        else{try{$encoding=[Text.UTF8Encoding]::new($false,$true);$null=$encoding.GetString($raw)}catch{$encoding=[Text.Encoding]::GetEncoding(1252)}}
        $text=$encoding.GetString($raw,$skip,$raw.Length-$skip)
        if($text -notmatch '(?m)^ContentId='){throw 'Vorhandene Konfiguration ohne ContentId: vor Installation manuell prüfen.'}
        $text=[regex]::Replace($text,'(?m)^ContentId=[^\r\n]*','ContentId='+$contentId)
        [IO.File]::WriteAllText($iniPath,$text,$encoding)
    }else{
        $text="[Network]`r`nHost=$HostAddress`r`nClient=0.0.0.0`r`nRole=$Role`r`nPort=$Port`r`nContentId=$contentId`r`n`r`n[Player]`r`nName=$PlayerName`r`n`r`n[Logging]`r`nLevel=2`r`nEngineMessages=1`r`n`r`n[Controls]`r`nHostKey=F6`r`nJoinKey=F7`r`nLeaveKey=F8`r`n`r`n[Combat]`r`nFriendlyFire=0`r`n`r`n[Avatar]`r`nGender=0`r`n`r`n[Diagnostics]`r`nOverlay=1`r`nNextTestStep=1`r`n"
        # Das Plugin liest neue Profile über die Windows-ANSI-Profilfunktionen.
        $ansi=[Text.Encoding]::GetEncoding([Globalization.CultureInfo]::CurrentCulture.TextInfo.ANSICodePage,[Text.EncoderFallback]::ExceptionFallback,[Text.DecoderFallback]::ExceptionFallback)
        [IO.File]::WriteAllText($iniPath,$text,$ansi)
    }
    Write-Output ('Optionales Profil: '+($profile.OptionalModules -join ', '))
    Write-Output ('Installiert. Bestehende persönliche Einstellungen erhalten; neue Einstellungen im Union-Menü prüfen. ContentId='+$contentId)
    Write-Output ('Runtime-Sicherung: '+$manifest)
    Write-Output 'Ninja-Loader muss in SystemPack.ini/Union.ini als Ninja.dll** registriert sein. INSTALLATION.md beachten. Kein Spiel gestartet.'
}catch{
    & (Join-Path $PSScriptRoot 'Restore-Runtime.ps1') -Manifest $manifest -GamePath $game
    throw
}

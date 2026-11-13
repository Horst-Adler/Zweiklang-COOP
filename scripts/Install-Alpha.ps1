[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [Parameter(Mandatory=$true)][string]$GamePath,
    [Parameter(Mandatory=$true)][string]$QolDirectory,
    [string]$UnionMenuScriptPath,
    [ValidateSet('Host','Client')][string]$Role='Host',
    [ValidateLength(1,63)][ValidatePattern('^[^\r\n\x00=]+$')][string]$PlayerName='Spieler',
    [string]$HostAddress='127.0.0.1',
    [ValidateRange(1,65535)][int]$Port=12345
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
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
$files=@(
    @{Source=$dll;Relative='System\Autorun\ZweiklangCoop.dll'},
    @{Source=(Join-Path $root 'src\ZweiklangCoop\scripts\ZweiklangCoop_Menu.d');Relative='System\Autorun\ZweiklangCoop_Menu.d'},
    @{Source=(Join-Path $root 'src\ZweiklangCoop\scripts\ZweiklangCoop_Npc.d');Relative='System\Autorun\ZweiklangCoop_Npc.d'}
)
$menuTarget=Join-Path $game 'System\Autorun\Menu\zUnionMenu.d'
if($UnionMenuScriptPath){
    if(-not(Test-Path -LiteralPath $UnionMenuScriptPath -PathType Leaf)){throw 'Separat bereitgestelltes zUnionMenu.d fehlt.'}
    $files+=@{Source=[IO.Path]::GetFullPath($UnionMenuScriptPath);Relative='System\Autorun\Menu\zUnionMenu.d'}
}elseif(-not(Test-Path -LiteralPath $menuTarget)){throw 'zUnionMenu.d aus der eigenen Union-Installation separat über -UnionMenuScriptPath bereitstellen. Fremdes Skript ist nicht im Repository enthalten.'}
$expected=@{
    'Data\Quickloot.vdf'='0352BC03B6E9C53CB2D3D4DC89426073C7E45AEE9272A994C7C9E178108B772F'
    'Data\Sprint.vdf'='9D06C29735810D9D07F551F9409ADE23494E983FF6F560C157855B82B8C77967'
    'Data\Toolkit.vdf'='08A8873043AE3E95C808E0AD519E00F302239BF58A561F3CA22EA6D00F5DC260'
    'Data\PatchMenu.vdf'='8DA288D5FCFF42EDDC2CBA99548A9C03A464E146DD9C5CFE39F7DAD2FFD08F7A'
    'System\Ninja.dll'='38CE2836A74227BEAF08C6972CD263389BC3D6F591BFF35AE812C83C6241CA8B'
}
foreach($relative in $expected.Keys){
    $path=Join-Path $QolDirectory $relative
    if(-not(Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path).Hash -ne $expected[$relative]){throw ('Vorbereitete QoL-Datei fehlt oder weicht ab: '+$relative)}
    $files+=@{Source=$path;Relative=$relative}
}
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
    $contentFiles=@(
        Get-ChildItem -LiteralPath (Join-Path $game 'Data') -File|Where-Object {$_.Extension -in '.vdf','.mod'}
        Get-ChildItem -LiteralPath (Join-Path $game '_work\Data\Scripts\_compiled') -File
        Get-Item -LiteralPath (Join-Path $system 'Gothic2.exe'),(Join-Path $system 'Shw32.dll'),(Join-Path $system 'Vdfs32g.dll'),(Join-Path $system 'Ninja.dll')
        Get-ChildItem -LiteralPath (Join-Path $system 'Autorun') -File -Recurse|Where-Object {$_.Extension -in '.dll','.d'}
    )|Sort-Object FullName -Unique
    $canonical=($contentFiles|ForEach-Object {$_.FullName.Substring($game.Length+1).ToLowerInvariant()+'|'+$_.Length+'|'+(Get-FileHash -LiteralPath $_.FullName).Hash}) -join "`n"
    $hasher=[Security.Cryptography.SHA256]::Create()
    try{$contentId=([BitConverter]::ToString($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($canonical)))).Replace('-','')}finally{$hasher.Dispose()}
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
    Write-Output ('Installiert. Bestehende persönliche Einstellungen erhalten; neue Einstellungen im Union-Menü prüfen. ContentId='+$contentId)
    Write-Output ('Runtime-Sicherung: '+$manifest)
    Write-Output 'Ninja-Loader muss in SystemPack.ini/Union.ini als Ninja.dll** registriert sein. INSTALLATION.md beachten. Kein Spiel gestartet.'
}catch{
    & (Join-Path $PSScriptRoot 'Restore-Runtime.ps1') -Manifest $manifest -GamePath $game
    throw
}

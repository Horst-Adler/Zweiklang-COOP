[CmdletBinding(SupportsShouldProcess=$true)]
param([Parameter(Mandatory=$true)][string]$Manifest,[Parameter(Mandatory=$true)][string]$GamePath)
$ErrorActionPreference='Stop'
$game=[IO.Path]::GetFullPath($GamePath).TrimEnd('\')
$allowed=@('System\Autorun\ZweiklangCoop.dll','System\Autorun\ZweiklangCoop_Menu.d','System\Autorun\ZweiklangCoop_Npc.d','System\Autorun\Menu\zUnionMenu.d','System\ZweiklangCoop.ini','System\Ninja.dll','Data\Quickloot.vdf','Data\Sprint.vdf','Data\Toolkit.vdf','Data\PatchMenu.vdf')
if(Get-Process Gothic2 -ErrorAction SilentlyContinue){throw 'Gothic II muss geschlossen sein.'}
$entries=Get-Content -LiteralPath $Manifest -Raw|ConvertFrom-Json
foreach($entry in $entries){
    $target=[IO.Path]::GetFullPath($entry.Target)
    if($target -notin @($allowed|ForEach-Object {[IO.Path]::GetFullPath((Join-Path $game $_))})){throw 'Manifest enthält ein unzulässiges Ziel.'}
    if($entry.Existed -and (-not(Test-Path -LiteralPath $entry.Backup) -or (Get-FileHash -LiteralPath $entry.Backup).Hash -ne $entry.SHA256)){throw 'Sicherung fehlt oder Hash stimmt nicht.'}
    if($target.EndsWith('.dll') -and (Test-Path -LiteralPath $target)){$probe=[IO.File]::Open($target,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);$probe.Close()}
}
if(-not $PSCmdlet.ShouldProcess($game,'Nur gesicherte Runtime wiederherstellen; keine Saves oder Recovery-Dateien zurücksetzen')){return}
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss-fff'
foreach($entry in $entries){
    if($entry.Existed){Copy-Item -LiteralPath $entry.Backup -Destination $entry.Target -Force}
    elseif(Test-Path -LiteralPath $entry.Target){Move-Item -LiteralPath $entry.Target -Destination ($entry.Target+'.disabled-'+$stamp)}
}
Write-Output 'Runtime wiederhergestellt. Vor einem älteren Plugin auch die Kompatibilität der separat gesicherten Spielstände/Recovery-Daten prüfen.'

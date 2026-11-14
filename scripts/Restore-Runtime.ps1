[CmdletBinding(SupportsShouldProcess=$true)]
param([Parameter(Mandatory=$true)][string]$Manifest,[Parameter(Mandatory=$true)][string]$GamePath)
$ErrorActionPreference='Stop'
$game=[IO.Path]::GetFullPath($GamePath).TrimEnd('\')
$allowed=@('Data\OreboundCoop_Menu.vdf','System\Autorun\OreboundCoop.dll','System\Autorun\OreboundCoop_Menu.d','System\Autorun\OreboundCoop_Npc.d','Data\OreboundCoop_Core.vdf','System\OreboundCoop.ini','System\Autorun\ZweiklangCoop.dll','System\Autorun\ZweiklangCoop_Menu.d','System\Autorun\ZweiklangCoop_Npc.d','Data\ZweiklangCoop_Core.vdf','System\ZweiklangCoop.ini','System\Autorun\Menu\zUnionMenu.d','System\Ninja.dll','Data\Quickloot.vdf','Data\Sprint.vdf','Data\Toolkit.vdf','Data\PatchMenu.vdf')
if(Get-Process Gothic2 -ErrorAction SilentlyContinue){throw 'Close Gothic II before restoring runtime files.'}
$targets=@($allowed|ForEach-Object {[IO.Path]::GetFullPath((Join-Path $game $_))})
$entries=@(Get-Content -LiteralPath $Manifest -Raw|ConvertFrom-Json)
if(-not $entries.Count){throw 'Empty runtime manifest.'}
foreach($entry in $entries){$target=[IO.Path]::GetFullPath($entry.Target);if($target -notin $targets){throw 'Manifest target is not an approved runtime file.'};if($entry.Existed -and (-not(Test-Path -LiteralPath $entry.Backup -PathType Leaf) -or (Get-FileHash -LiteralPath $entry.Backup).Hash -ne $entry.SHA256)){throw 'Missing or modified backup.'}}
if(-not $PSCmdlet.ShouldProcess($game,'Restore only approved runtime files; preserve saves and recovery data')){return}
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss-fff'
foreach($entry in $entries){if($entry.Existed){Copy-Item -LiteralPath $entry.Backup -Destination $entry.Target -Force}elseif(Test-Path -LiteralPath $entry.Target){Move-Item -LiteralPath $entry.Target -Destination ($entry.Target+'.disabled-'+$stamp)}}
Write-Output 'Runtime restored. Check separately backed-up save/recovery compatibility before using an older build.'

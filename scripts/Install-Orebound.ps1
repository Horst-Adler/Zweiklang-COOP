[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [Parameter(Mandatory=$true)][string]$GamePath,
    [string]$SourceRoot=(Split-Path -Parent $PSScriptRoot),
    [string]$QolDirectory,
    [ValidateSet('Quickloot','Sprint','Toolkit','PatchMenu')][string[]]$OptionalModules=@(),
    [string]$UnionMenuScriptPath,
    [ValidateSet('Host','Client')][string]$Role='Host',
    [ValidateLength(1,32)][ValidatePattern('^[^\r\n\x00=]+$')][string]$PlayerName='Player',
    [string]$HostAddress='127.0.0.1',
    [ValidateRange(1024,65535)][int]$Port=12345,
    [ValidateSet('','en','de')][string]$Language='',
    [string]$ExpectedContentId=''
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($SourceRoot).TrimEnd('\')
$game=[IO.Path]::GetFullPath($GamePath).TrimEnd('\');$system=Join-Path $game 'System'
. (Join-Path $PSScriptRoot 'ContentIdentity.ps1')
. (Join-Path $PSScriptRoot 'Check-Dependencies.ps1')
if(Get-Process Gothic2 -ErrorAction SilentlyContinue){throw 'Close Gothic II before installing.'}
foreach($rel in @('System\Gothic2.exe','System\Shw32.dll','System\Vdfs32g.dll','Data\Union.vdf','_work\Data\Scripts\_compiled\GOTHIC.DAT')){if(-not(Test-Path -LiteralPath (Join-Path $game $rel) -PathType Leaf)){throw ('Missing game prerequisite: '+$rel)}}
if((Get-Item -LiteralPath ($system+'\Shw32.dll')).VersionInfo.ProductName -ne 'Union'){throw 'The selected installation does not have Union.'}
$exe=[IO.File]::ReadAllBytes(($system+'\Gothic2.exe'));if($exe.Length -lt 64){throw 'Invalid Gothic II executable.'};$pe=[BitConverter]::ToInt32($exe,60)
if($pe -lt 0 -or $pe+24 -gt $exe.Length -or ([BitConverter]::ToUInt16($exe,$pe+22) -band 32) -eq 0){throw 'The supported executable requires the LAA flag. Prepare the game separately.'}
$parsed=$null;if(-not[Net.IPAddress]::TryParse($HostAddress,[ref]$parsed) -or $parsed.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork -or $HostAddress -eq '0.0.0.0'){throw 'HostAddress must be a concrete IPv4 address.'}
$files=@(
 @{Source=Join-Path $root 'build\alpha\G2NotR\OreboundCoop.dll';Relative='System\Autorun\OreboundCoop.dll'},
 @{Source=Join-Path $root 'src\OreboundCoop\scripts\OreboundCoop_Menu.d';Relative='System\Autorun\OreboundCoop_Menu.d'},
 @{Source=Join-Path $root 'src\OreboundCoop\scripts\OreboundCoop_Npc.d';Relative='System\Autorun\OreboundCoop_Npc.d'},
 @{Source=Join-Path $root 'build\alpha\ninja\OreboundCoop_Core.vdf';Relative='Data\OreboundCoop_Core.vdf'}
)
foreach($file in $files){if(-not(Test-Path -LiteralPath $file.Source -PathType Leaf)){throw ('Build output missing: '+$file.Source)}}
if((Get-Item -LiteralPath $files[0].Source).VersionInfo.FileVersion -ne '0.1.0-alpha.33'){throw 'Build Alpha.33 before installing.'}
$core=Get-Content -LiteralPath (Join-Path $root 'build\alpha\ninja\manifest.json') -Raw|ConvertFrom-Json
if($core.ABI -ne 2 -or (Get-FileHash -LiteralPath $files[3].Source).Hash -ne $core.SHA256){throw 'Own Ninja module hash/ABI does not match the build manifest.'}
$menuTarget=$system+'\Autorun\Menu\zUnionMenu.d'
if($UnionMenuScriptPath){if(-not(Test-Path -LiteralPath $UnionMenuScriptPath -PathType Leaf)){throw 'Separately supplied Union menu script is missing.'};$files+=@{Source=[IO.Path]::GetFullPath($UnionMenuScriptPath);Relative='System\Autorun\Menu\zUnionMenu.d'}}elseif(-not(Test-Path -LiteralPath $menuTarget)){throw 'Supply your own Union zUnionMenu.d with -UnionMenuScriptPath.'}
$catalogPath=Join-Path $root 'config\dependencies.json';$catalog=Get-Content -LiteralPath $catalogPath -Raw|ConvertFrom-Json
$selection=@($OptionalModules);if(@($selection|Where-Object {$_ -in 'Quickloot','Sprint','PatchMenu'}).Count -and 'Toolkit' -notin $selection){$selection+='Toolkit'}
foreach($module in $catalog.Modules){if($module.Id -in $selection){if(-not $QolDirectory){throw 'Use -QolDirectory to supply selected optional modules.'};$path=Join-Path $QolDirectory $module.File;if(-not(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-FileHash -LiteralPath $path).Hash -ne $module.SHA256){throw ('Unsupported optional source: '+$module.File)};$files+=@{Source=$path;Relative=$module.File.Replace('/','\')}}}
if($QolDirectory -and (Test-Path -LiteralPath (Join-Path $QolDirectory $catalog.Ninja.File))){$path=Join-Path $QolDirectory $catalog.Ninja.File;if((Get-FileHash -LiteralPath $path).Hash -ne $catalog.Ninja.SHA256){throw 'Unsupported Ninja source.'};$files+=@{Source=$path;Relative='System\Ninja.dll'}}else{$path=Join-Path $game $catalog.Ninja.File;if(-not(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-FileHash -LiteralPath $path).Hash -ne $catalog.Ninja.SHA256){throw 'Ninja 4.1.16 is required.'}}
$effective=@();foreach($module in $catalog.Modules){$incoming=@($files|Where-Object {$_.Relative -eq $module.File.Replace('/','\')});$path=if($incoming.Count){$incoming[0].Source}else{Join-Path $game $module.File};if(Test-Path -LiteralPath $path){if((Get-FileHash -LiteralPath $path).Hash -ne $module.SHA256){throw ('Unsupported retained archive: '+$module.File)};$effective+=$module}}
if(@($effective|Where-Object RequiresToolkit).Count -and 'Toolkit' -notin @($effective.Id)){throw 'Selected or retained comfort modules require Toolkit 16.0.'}
$legacy=@('System\Autorun\ZweiklangCoop.dll','System\Autorun\ZweiklangCoop_Menu.d','System\Autorun\ZweiklangCoop_Npc.d','Data\ZweiklangCoop_Core.vdf','System\ZweiklangCoop.ini')
$iniRelative='System\OreboundCoop.ini';$iniPath=Join-Path $game $iniRelative;$legacyIni=Join-Path $game $legacy[4]
$oldIni=if(Test-Path -LiteralPath $iniPath){$iniPath}elseif(Test-Path -LiteralPath $legacyIni){$legacyIni}else{$null}
$text=$null;$encoding=[Text.UTF8Encoding]::new($false,$true)
if($oldIni){$raw=[IO.File]::ReadAllBytes($oldIni);$skip=0;if($raw.Length -ge 2 -and $raw[0] -eq 255 -and $raw[1] -eq 254){$encoding=[Text.Encoding]::Unicode;$skip=2}elseif($raw.Length -ge 3 -and $raw[0] -eq 239 -and $raw[1] -eq 187 -and $raw[2] -eq 191){$encoding=[Text.UTF8Encoding]::new($true,$true);$skip=3}else{try{$null=$encoding.GetString($raw)}catch{$encoding=[Text.Encoding]::GetEncoding(1252)}};$text=$encoding.GetString($raw,$skip,$raw.Length-$skip);if([regex]::Matches($text,'(?m)^ContentId=[^\r\n]*').Count -ne 1){throw 'Existing configuration must contain one unambiguous ContentId.'}}
$selectedLanguage=if($Language){$Language}elseif($oldIni -eq $legacyIni){'de'}else{'en'}
foreach($rel in @($legacy[0],$files[0].Relative)){if(Test-Path -LiteralPath (Join-Path $game $rel)){$probe=[IO.File]::Open((Join-Path $game $rel),[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);$probe.Close()}}
if(-not $PSCmdlet.ShouldProcess($game,'Back up and install Orebound Co-op; migrate legacy files and preserve saves/settings')){return}
$backup=Join-Path $root ('local\install-backups\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'));New-Item -ItemType Directory -Path $backup -Force|Out-Null
$records=@();$state=@();$other=@()
foreach($rel in (@($files|ForEach-Object Relative)+$legacy+$iniRelative|Select-Object -Unique)){$target=Join-Path $game $rel;$saved=Join-Path $backup $rel;$exists=Test-Path -LiteralPath $target;$hash=$null;if($exists){New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force|Out-Null;Copy-Item -LiteralPath $target -Destination $saved;$hash=(Get-FileHash -LiteralPath $target).Hash;if((Get-FileHash -LiteralPath $saved).Hash -ne $hash){throw 'Backup hash mismatch.'}};$records+=[ordered]@{Target=$target;Backup=$saved;Existed=$exists;SHA256=$hash}}
$folders=@('System\ZweiklangCoopState','System\ZweiklangLogs','System\OreboundCoopState','System\OreboundLogs')+@(Get-ChildItem -LiteralPath $game -Directory|Where-Object {$_.Name -match '^saves($|_)'}|ForEach-Object Name)
foreach($folder in $folders){$path=Join-Path $game $folder;if(-not(Test-Path -LiteralPath $path)){continue};foreach($file in Get-ChildItem -LiteralPath $path -File -Recurse){$rel=$file.FullName.Substring($game.Length+1);$saved=Join-Path ($backup+'\State') $rel;New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force|Out-Null;Copy-Item -LiteralPath $file.FullName -Destination $saved;$hash=(Get-FileHash -LiteralPath $file.FullName).Hash;if((Get-FileHash -LiteralPath $saved).Hash -ne $hash){throw 'Save/state/log backup mismatch.'};$state+=[ordered]@{File=$rel;SHA256=$hash}}}
foreach($rel in @('System\Gothic.ini','System\SystemPack.ini','System\Union.ini','System\ddraw.dll','System\Ninja.dll','Data\Quickloot.vdf','Data\Sprint.vdf','Data\Toolkit.vdf','Data\PatchMenu.vdf')){$path=Join-Path $game $rel;if(Test-Path -LiteralPath $path -PathType Leaf){$other+=[ordered]@{File=$rel;SHA256=(Get-FileHash -LiteralPath $path).Hash}}}
$manifest=Join-Path $backup 'manifest.json';ConvertTo-Json -InputObject @($records) -Depth 5|Set-Content -LiteralPath $manifest -Encoding UTF8
try{
 foreach($file in $files){$target=Join-Path $game $file.Relative;New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force|Out-Null;if([IO.Path]::GetFullPath($file.Source) -ne [IO.Path]::GetFullPath($target)){Copy-Item -LiteralPath $file.Source -Destination $target -Force};if((Get-FileHash -LiteralPath $target).Hash -ne (Get-FileHash -LiteralPath $file.Source).Hash){throw 'Runtime copy hash mismatch.'}}
 # These exact owned files are already backed up. Remove them from loader paths
 # so Union/Ninja cannot load both names. Never remove a state/log/save directory.
 foreach($rel in $legacy){$target=Join-Path $game $rel;if(Test-Path -LiteralPath $target){Remove-Item -LiteralPath $target -Force}}
 $profile=Test-CoopDependencies -GamePath $game -CatalogPath $catalogPath
 $contentFiles=@(Get-ChildItem -LiteralPath ($game+'\Data') -File|Where-Object {$_.Extension -in '.vdf','.mod'};Get-ChildItem -LiteralPath ($game+'\_work\Data\Scripts\_compiled') -File;Get-Item -LiteralPath ($system+'\Gothic2.exe'),($system+'\Shw32.dll'),($system+'\Vdfs32g.dll'),($system+'\Ninja.dll');Get-ChildItem -LiteralPath ($system+'\Autorun') -File -Recurse|Where-Object {$_.Extension -in '.dll','.d'})|Sort-Object FullName -Unique
 $rows=@($contentFiles|ForEach-Object {[ordered]@{File=$_.FullName.Substring($game.Length+1).ToLowerInvariant();Bytes=$_.Length;SHA256=(Get-FileHash -LiteralPath $_.FullName).Hash}});$id=Get-CoopContentIdentity -Rows $rows
 if($ExpectedContentId -and $id -ne $ExpectedContentId){throw 'Final content does not match the expected peer installation.'}
 if($null -eq $text){$text="[Network]`r`nHost=$HostAddress`r`nClient=0.0.0.0`r`nRole=$Role`r`nPort=$Port`r`nContentId=$id`r`n`r`n[Player]`r`nName=$PlayerName`r`n`r`n[Logging]`r`nLevel=2`r`nEngineMessages=1`r`n`r`n[Controls]`r`nHostKey=F6`r`nJoinKey=F7`r`nLeaveKey=F8`r`n`r`n[Combat]`r`nFriendlyFire=0`r`n`r`n[Avatar]`r`nGender=0`r`n`r`n[Diagnostics]`r`nOverlay=1`r`nNextTestStep=1`r`n"}else{$text=[regex]::Replace($text,'(?m)^ContentId=[^\r\n]*','ContentId='+$id);$text=[regex]::Replace($text,'(?m)^; Zweiklang Coop [^\r\n]*','; Orebound Co-op 0.1.0-alpha.33')}
 $section=[regex]::Match($text,'(?ms)^\[Interface\][ \t]*\r?\n(.*?)(?=^\[|\z)')
 if($section.Success){$lang=[regex]::Match($section.Value,'(?m)^Language=[^\r\n]*');if($lang.Success){if($Language){$changed=[regex]::Replace($section.Value,'(?m)^Language=[^\r\n]*','Language='+$Language);$text=$text.Substring(0,$section.Index)+$changed+$text.Substring($section.Index+$section.Length)}}else{$text=$text.Insert($section.Index+$section.Length,"`r`nLanguage=$selectedLanguage`r`n")}}
 else{$text+="`r`n[Interface]`r`nLanguage=$selectedLanguage`r`n"}
 [IO.File]::WriteAllText($iniPath,$text,$encoding)
 foreach($row in $state){if((Get-FileHash -LiteralPath (Join-Path $game $row.File)).Hash -ne $row.SHA256){throw ('Save/state/log changed: '+$row.File)}}
 foreach($row in $other){if($row.File -notin @($files|ForEach-Object Relative) -and (Get-FileHash -LiteralPath (Join-Path $game $row.File)).Hash -ne $row.SHA256){throw ('Unrelated game configuration/module changed: '+$row.File)}}
 foreach($rel in $legacy){if(Test-Path -LiteralPath (Join-Path $game $rel)){throw 'Legacy loader file still present after migration.'}}
 $result=[ordered]@{Version='0.1.0-alpha.33';Protocol=11;ScriptABI=2;GamePath=$game;ContentId=$id;Content=$rows;StateFilesUnchanged=$state.Count;PersonalConfigurationPreserved=$true;LegacyLoaderFilesRemoved=$true;OptionalModules=@($profile.OptionalModules);BackupManifest=$manifest;GamesStarted=$false;VerifiedAt=(Get-Date -Format o)}
 $result|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $backup 'verification.json') -Encoding UTF8
 $result|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $root 'local\last-installation.json') -Encoding UTF8
 Write-Output ('Orebound Co-op Alpha.33 installed. ContentId='+$id);Write-Output ('Backup: '+$manifest);Write-Output ('Unchanged save/state/log files: '+$state.Count);Write-Output 'No game started. Native acceptance remains open.'
}catch{
 foreach($entry in $records){if($entry.Existed){Copy-Item -LiteralPath $entry.Backup -Destination $entry.Target -Force}elseif(Test-Path -LiteralPath $entry.Target){Remove-Item -LiteralPath $entry.Target -Force}}
 throw
}

[CmdletBinding()]
param([string]$GamePath,[switch]$Client)
$ErrorActionPreference='Stop'
if(-not $GamePath) {
    if(Test-Path -LiteralPath (Join-Path $PSScriptRoot 'Gothic2')) {$GamePath=Join-Path $PSScriptRoot 'Gothic2'}
    else {
        $root=Split-Path -Parent $PSScriptRoot
        $GamePath=if($Client){Join-Path $root 'client\OreboundCoop-G2-Alpha-0.1\Gothic2'}else{Join-Path $root 'games\Gothic 2 Gold'}
    }
}
$GamePath=[IO.Path]::GetFullPath($GamePath)
$systemPath=Join-Path $GamePath 'System'
if(-not(Test-Path -LiteralPath (Join-Path $systemPath 'Autorun\OreboundCoop.dll')) -and -not(Test-Path -LiteralPath (Join-Path $systemPath 'Autorun\ZweiklangCoop.dll'))){throw 'Orebound Co-op installation not found.'}
$exportRoot=Join-Path (Split-Path -Parent $GamePath) 'Diagnose'
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$stage=Join-Path $exportRoot ('Orebound-Diagnostics-'+$stamp)
New-Item -ItemType Directory -Path $stage -Force|Out-Null
$logs=Join-Path $systemPath 'ZweiklangLogs'
if(-not(Test-Path -LiteralPath $logs)){$logs=Join-Path $systemPath 'OreboundLogs'}
if(Test-Path -LiteralPath $logs){
    New-Item -ItemType Directory -Path (Join-Path $stage 'Logs') -Force|Out-Null
    Get-ChildItem -LiteralPath $logs -File -Filter 'session-*.jsonl*' | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $stage 'Logs')
    }
    $journal=Join-Path $logs 'loot-transactions.jsonl'
    if(Test-Path -LiteralPath $journal){Copy-Item -LiteralPath $journal -Destination (Join-Path $stage 'Logs')}
}
$ini=Join-Path $systemPath 'OreboundCoop.ini'
if(-not(Test-Path -LiteralPath $ini)){$ini=Join-Path $systemPath 'ZweiklangCoop.ini'}
Copy-Item -LiteralPath $ini -Destination $stage
$language='en';$match=[regex]::Match([IO.File]::ReadAllText($ini),'(?m)^Language=(en|de)\r?$');if($match.Success){$language=$match.Groups[1].Value}
$recovery=Join-Path $systemPath 'ZweiklangCoopState'
if(-not(Test-Path -LiteralPath $recovery)){$recovery=Join-Path $systemPath 'OreboundCoopState'}
if(Test-Path -LiteralPath $recovery){
    $recoveryTarget=Join-Path $stage 'Recovery'
    New-Item -ItemType Directory -Path $recoveryTarget -Force|Out-Null
    Get-ChildItem -LiteralPath $recovery -File -Filter '*.bin*'|Copy-Item -Destination $recoveryTarget
}
$saveMetadata=@()
foreach($saveRoot in Get-ChildItem -LiteralPath $GamePath -Directory|Where-Object {$_.Name -match '^saves($|_)'}){
    foreach($checkpoint in Get-ChildItem -LiteralPath $saveRoot.FullName -File -Recurse -Filter 'ZWEIKLANG_RECOVERY.BIN'){
        $relative=$checkpoint.FullName.Substring($GamePath.Length+1);$target=Join-Path $stage ('SaveCheckpoints\'+$relative)
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force|Out-Null
        Copy-Item -LiteralPath $checkpoint.FullName -Destination $target
        $actors=Join-Path $checkpoint.DirectoryName 'ZWEIKLANG_ACTORS.BIN'
        if(Test-Path -LiteralPath $actors -PathType Leaf){Copy-Item -LiteralPath $actors -Destination (Join-Path (Split-Path -Parent $target) 'ZWEIKLANG_ACTORS.BIN')}
        $header=Join-Path $checkpoint.DirectoryName 'SAVEINFO.SAV'
        if(Test-Path -LiteralPath $header -PathType Leaf){
            $info=Get-Item -LiteralPath $header;Copy-Item -LiteralPath $header -Destination (Join-Path (Split-Path -Parent $target) 'SAVEINFO.SAV')
            $saveMetadata+=[ordered]@{Checkpoint=$relative;CheckpointSHA256=(Get-FileHash -LiteralPath $checkpoint.FullName).Hash;SaveHeaderSHA256=(Get-FileHash -LiteralPath $header).Hash;SaveHeaderLastWriteTicks=$info.LastWriteTimeUtc.Ticks;SaveHeaderFileTimeUtc=$info.LastWriteTimeUtc.ToFileTimeUtc().ToString()}
        }
    }
}
ConvertTo-Json -InputObject @($saveMetadata) -Depth 4|Set-Content -LiteralPath (Join-Path $stage 'SaveCheckpoints.json') -Encoding UTF8
foreach($relative in @('Gothic.ini','SystemPack.ini','Log.txt','GD3D11\UserSettings.ini')){
    $file=Join-Path $systemPath $relative
    if(Test-Path -LiteralPath $file -PathType Leaf){
        $target=Join-Path $stage ('Runtime\'+$relative)
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force|Out-Null
        Copy-Item -LiteralPath $file -Destination $target
    }
}
$qolFiles=@('Data\OreboundCoop_Menu.vdf','Data\OreboundCoop_Core.vdf','Data\ZweiklangCoop_Core.vdf','Data\Quickloot.vdf','Data\Sprint.vdf','Data\Toolkit.vdf','Data\PatchMenu.vdf','System\Ninja.dll')
$qolProfile=@($qolFiles|ForEach-Object {
    $file=Join-Path $GamePath $_
    [ordered]@{File=$_;Exists=(Test-Path -LiteralPath $file -PathType Leaf);SHA256=$(if(Test-Path -LiteralPath $file -PathType Leaf){(Get-FileHash -LiteralPath $file).Hash}else{$null})}
})
ConvertTo-Json -InputObject @($qolProfile) -Depth 4|Set-Content -LiteralPath (Join-Path $stage 'QolProfile.json') -Encoding UTF8
$binaries=@('Gothic2.exe','Shw32.dll','Vdfs32g.dll','Ninja.dll','Autorun\OreboundCoop.dll','Autorun\OreboundCoop_Menu.d','Autorun\OreboundCoop_Npc.d','Autorun\ZweiklangCoop.dll','Autorun\ZweiklangCoop_Menu.d','Autorun\ZweiklangCoop_Npc.d')
$versions=@($binaries|Where-Object {Test-Path -LiteralPath (Join-Path $systemPath $_)}|ForEach-Object {
    $file=Get-Item -LiteralPath (Join-Path $systemPath $_)
    [ordered]@{File=$_;Version=$file.VersionInfo.FileVersion;SHA256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}
})
[ordered]@{ExportedAt=(Get-Date -Format o);Product='Orebound Co-op';Language=$language;Binaries=$versions;GamesStarted=$false;Note='Local export; no data sent. Logs may contain player names and LAN addresses.'}|
    ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $stage 'Versions.json') -Encoding UTF8
$zip=$stage+'.zip'
Compress-Archive -LiteralPath $stage -DestinationPath $zip
Write-Output ('Diagnostics saved locally: '+$zip)

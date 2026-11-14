[CmdletBinding()]
param([string]$SdkRoot,[string]$MSBuildPath,[string]$PythonPath="python",[switch]$TestsOnly)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not $MSBuildPath){
    $command=Get-Command MSBuild.exe -ErrorAction SilentlyContinue
    if($command){$MSBuildPath=$command.Source}
    else{
        $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if(Test-Path -LiteralPath $vswhere){$MSBuildPath=@(& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe')|Select-Object -First 1}
    }
}
if(-not $MSBuildPath -or -not(Test-Path -LiteralPath $MSBuildPath)){throw 'MSBuild fehlt. VS/Build Tools mit MSVC v142 installieren oder -MSBuildPath angeben.'}
& (Join-Path $PSScriptRoot 'Check-UnionMenu.ps1')
& (Join-Path $PSScriptRoot 'Test-ContentIdentity.ps1')
$source=Join-Path $root 'src\ZweiklangCoop'
$logs=Join-Path $root 'build\logs';New-Item -ItemType Directory -Path $logs -Force|Out-Null
if(-not $TestsOnly){
    & $PythonPath (Join-Path $PSScriptRoot "Build-NinjaModule.py")
    if($LASTEXITCODE -ne 0){throw "Ninja-Modul nicht gebaut. Python + zenkit und -PythonPath pruefen."}
    if(-not $SdkRoot){$SdkRoot=Join-Path $root 'local\sdk\GothicCoop'}
    $SdkRoot=[IO.Path]::GetFullPath($SdkRoot).TrimEnd('\')+'\'
    foreach($relative in @('UnionAfx.h','ZenGin\Gothic_II_Addon\ClassDeclarators_G2A.cpp','SDK\Union\v1.0l\Lib\Shw32.lib')){
        if(-not(Test-Path -LiteralPath (Join-Path $SdkRoot $relative))){throw ('Externe SDK-Datei fehlt: '+$relative+'. BUILDING.md beachten.')}
    }
    & $MSBuildPath (Join-Path $source 'ZweiklangCoop.vcxproj') /m /t:Build /p:Configuration=Release /p:Platform=Win32 /p:Game=G2NotR ("/p:SdkRoot=$SdkRoot") /verbosity:minimal 2>&1|Tee-Object -FilePath (Join-Path $logs 'plugin.log')
    if($LASTEXITCODE -ne 0){throw 'Plugin-Build fehlgeschlagen.'}
}
& $MSBuildPath (Join-Path $source 'NetworkTests.vcxproj') /m /t:Build /p:Configuration=Release /p:Platform=Win32 /verbosity:minimal 2>&1|Tee-Object -FilePath (Join-Path $logs 'tests-build.log')
if($LASTEXITCODE -ne 0){throw 'Test-Build fehlgeschlagen.'}
Push-Location $root
try{
    & (Join-Path $root 'build\alpha\tests\ZweiklangNetworkTests.exe') 2>&1|Tee-Object -FilePath (Join-Path $logs 'tests.log')
    if($LASTEXITCODE -ne 0){throw 'Technische Tests fehlgeschlagen.'}
}finally{Pop-Location}
Write-Output 'Build/Tests abgeschlossen. Kein Spiel gestartet. Ausgabe: build/alpha/G2NotR; Protokolle: build/logs.'

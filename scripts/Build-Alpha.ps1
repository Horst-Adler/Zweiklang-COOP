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
if(-not $MSBuildPath -or -not(Test-Path -LiteralPath $MSBuildPath)){throw 'MSBuild is missing. Install VS/Build Tools with MSVC v142 or specify -MSBuildPath.'}
& (Join-Path $PSScriptRoot 'Check-UnionMenu.ps1')
& (Join-Path $PSScriptRoot 'Test-ContentIdentity.ps1')
$source=Join-Path $root 'src\OreboundCoop'
$logs=Join-Path $root 'build\logs';New-Item -ItemType Directory -Path $logs -Force|Out-Null
if(-not $TestsOnly){
    & $PythonPath (Join-Path $PSScriptRoot "Build-Localization.py")
    if($LASTEXITCODE -ne 0){throw "Localization catalogue build failed."}
    & $PythonPath (Join-Path $PSScriptRoot "Build-NinjaModule.py")
    if($LASTEXITCODE -ne 0){throw "Ninja module build failed. Check Python, zenkit and -PythonPath."}
    if(-not $SdkRoot){$SdkRoot=Join-Path $root 'local\sdk\GothicCoop'}
    $SdkRoot=[IO.Path]::GetFullPath($SdkRoot).TrimEnd('\')+'\'
    foreach($relative in @('UnionAfx.h','ZenGin\Gothic_II_Addon\ClassDeclarators_G2A.cpp','SDK\Union\v1.0l\Lib\Shw32.lib')){
        if(-not(Test-Path -LiteralPath (Join-Path $SdkRoot $relative))){throw ('External SDK file is missing: '+$relative+'. See BUILDING.md.')}
    }
    & $MSBuildPath (Join-Path $source 'OreboundCoop.vcxproj') /m /t:Build /p:Configuration=Release /p:Platform=Win32 /p:Game=G2NotR ("/p:SdkRoot=$SdkRoot") /verbosity:minimal 2>&1|Tee-Object -FilePath (Join-Path $logs 'plugin.log')
    if($LASTEXITCODE -ne 0){throw 'Plugin build failed.'}
}
& $MSBuildPath (Join-Path $source 'NetworkTests.vcxproj') /m /t:Build /p:Configuration=Release /p:Platform=Win32 /verbosity:minimal 2>&1|Tee-Object -FilePath (Join-Path $logs 'tests-build.log')
if($LASTEXITCODE -ne 0){throw 'Test build failed.'}
Push-Location $root
try{
    & (Join-Path $root 'build\alpha\tests\OreboundNetworkTests.exe') 2>&1|Tee-Object -FilePath (Join-Path $logs 'tests.log')
    if($LASTEXITCODE -ne 0){throw 'Technical tests failed.'}
}finally{Pop-Location}
Write-Output 'Build/tests complete. No game started. Output: build/alpha/G2NotR; logs: build/logs.'

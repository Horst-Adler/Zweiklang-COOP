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
# Compatibility entry point. The current installer owns all validation/migration.
& (Join-Path $PSScriptRoot 'Install-Orebound.ps1') @PSBoundParameters

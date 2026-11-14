[CmdletBinding()]
param([string]$GamePath,[switch]$Client)
& (Join-Path $PSScriptRoot 'Export-Diagnostics.ps1') @PSBoundParameters

function Test-CoopDependencies {
    [CmdletBinding()]
    param([Parameter(Mandatory=$true)][string]$GamePath,[Parameter(Mandatory=$true)][string]$CatalogPath)
    $catalog=Get-Content -LiteralPath $CatalogPath -Raw|ConvertFrom-Json
    if($catalog.Schema -ne 1 -or $catalog.Modules.Count -ne 4){throw 'Invalid dependency catalogue.'}
    $ninja=Join-Path $GamePath $catalog.Ninja.File
    if(-not(Test-Path -LiteralPath $ninja -PathType Leaf) -or (Get-FileHash -LiteralPath $ninja).Hash -ne $catalog.Ninja.SHA256){throw 'Ninja 4.1.16 is missing or differs from the supported file.'}
    $present=@()
    foreach($module in $catalog.Modules){
        $path=Join-Path $GamePath $module.File
        if(Test-Path -LiteralPath $path){
            if(-not(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-FileHash -LiteralPath $path).Hash -ne $module.SHA256){throw ('Unsupported optional file: '+$module.File)}
            $present+=$module
        }
    }
    if(@($present|Where-Object RequiresToolkit).Count -and 'Toolkit' -notin @($present.Id)){throw 'These comfort modules require Toolkit 16.0. Disable all four comfort archives for minimal operation.'}
    [pscustomobject]@{Ninja=$catalog.Ninja.Version;OptionalModules=@($present.Id);Minimal=($present.Count -eq 0)}
}

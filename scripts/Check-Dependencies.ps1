function Test-CoopDependencies {
    [CmdletBinding()]
    param([Parameter(Mandatory=$true)][string]$GamePath,[Parameter(Mandatory=$true)][string]$CatalogPath)
    $catalog=Get-Content -LiteralPath $CatalogPath -Raw|ConvertFrom-Json
    if($catalog.Schema -ne 1 -or $catalog.Modules.Count -ne 4){throw 'Ungueltiger Abhaengigkeitskatalog.'}
    $ninja=Join-Path $GamePath $catalog.Ninja.File
    if(-not(Test-Path -LiteralPath $ninja -PathType Leaf) -or (Get-FileHash -LiteralPath $ninja).Hash -ne $catalog.Ninja.SHA256){throw 'Ninja 4.1.16 fehlt/weicht ab.'}
    $present=@()
    foreach($module in $catalog.Modules){
        $path=Join-Path $GamePath $module.File
        if(Test-Path -LiteralPath $path){
            if(-not(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-FileHash -LiteralPath $path).Hash -ne $module.SHA256){throw ('Nicht unterstuetzte optionale Datei: '+$module.File)}
            $present+=$module
        }
    }
    if(@($present|Where-Object RequiresToolkit).Count -and 'Toolkit' -notin @($present.Id)){throw 'Diese Komfort-Erweiterungen brauchen Toolkit 16.0. Fuer Minimalbetrieb alle vier Komfortarchive deaktivieren.'}
    [pscustomobject]@{Ninja=$catalog.Ninja.Version;OptionalModules=@($present.Id);Minimal=($present.Count -eq 0)}
}

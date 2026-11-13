function Get-CoopContentIdentity {
    [CmdletBinding()]
    param([Parameter(Mandatory=$true)][ValidateNotNullOrEmpty()][object[]]$Rows)
    $names=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $lines=[Collections.Generic.List[string]]::new()
    foreach($row in $Rows){
        $name=([string]$row.File).Replace('/','\').ToLowerInvariant()
        $digest=([string]$row.SHA256).ToUpperInvariant()
        $length=[long]$row.Bytes
        if([string]::IsNullOrWhiteSpace($name) -or [IO.Path]::IsPathRooted($name) -or $name -match '(^|\\)\.\.?(\\|$)|[|\r\n]' -or $length -lt 0 -or $digest -notmatch '^[0-9A-F]{64}$'){
            throw 'Invalid content descriptor'
        }
        if(-not $names.Add($name)){throw ('Duplicate content path: '+$name)}
        $lines.Add($name+'|'+$length.ToString([Globalization.CultureInfo]::InvariantCulture)+'|'+$digest)
    }
    # Ordinal ordering is identical on Windows PowerShell 5.1 and PowerShell 7,
    # regardless of OS collation tables, locale or directory enumeration order.
    $lines.Sort([StringComparer]::Ordinal)
    $canonical=$lines -join "`n"
    $hasher=[Security.Cryptography.SHA256]::Create()
    try{([BitConverter]::ToString($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($canonical)))).Replace('-','')}finally{$hasher.Dispose()}
}

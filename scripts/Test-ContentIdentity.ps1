$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'ContentIdentity.ps1')
$rows=@(
    [pscustomobject]@{File='data\anims.vdf';Bytes=12;SHA256=('A'*64)},
    [pscustomobject]@{File='data\anims_addon.vdf';Bytes=34;SHA256=('B'*64)}
)
# Independently generated SHA-256 of the specified ordinal UTF-8 fixture.
$expected='CDD183493DEEB137D88FB7976AA688B4D97FC649AE3C927C953E5440DCB1E59A'
$culture=[Globalization.CultureInfo]::CurrentCulture
$count=0
try{
    foreach($name in @('en-US','de-DE','tr-TR','')){
        [Globalization.CultureInfo]::CurrentCulture=[Globalization.CultureInfo]::GetCultureInfo($name)
        foreach($order in @(@(0,1),@(1,0))){
            $selected=@($order | ForEach-Object {$rows[$_]})
            if((Get-CoopContentIdentity -Rows $selected) -ne $expected){throw ('Culture/order mismatch: '+$name)}
            $count++
        }
        $mixed=@([pscustomobject]@{File='DATA/ANIMS.VDF';Bytes='12';SHA256=('a'*64)},$rows[1])
        if((Get-CoopContentIdentity -Rows $mixed) -ne $expected){throw ('Case/slash normalization failed: '+$name)}
        $count++
    }
}finally{[Globalization.CultureInfo]::CurrentCulture=$culture}
foreach($changed in @(
    [pscustomobject]@{File='data\other.vdf';Bytes=12;SHA256=('A'*64)},
    [pscustomobject]@{File='data\anims.vdf';Bytes=13;SHA256=('A'*64)},
    [pscustomobject]@{File='data\anims.vdf';Bytes=12;SHA256=('C'*64)}
)){
    if((Get-CoopContentIdentity -Rows @($changed,$rows[1])) -eq $expected){throw 'Actual content change was ignored'}
    $count++
}
foreach($bad in @(
    [pscustomobject]@{File='DATA/ANIMS.VDF';Bytes=12;SHA256=('A'*64)},
    [pscustomobject]@{File='..\outside';Bytes=12;SHA256=('A'*64)},
    [pscustomobject]@{File='bad|path';Bytes=12;SHA256=('A'*64)},
    [pscustomobject]@{File='data\invalid';Bytes=-1;SHA256=('A'*64)},
    [pscustomobject]@{File='data\invalid';Bytes=1;SHA256='bad'}
)){
    $rejected=$false
    try{$null=Get-CoopContentIdentity -Rows @($rows[0],$bad)}catch{$rejected=$true}
    if(-not $rejected){throw 'Invalid/duplicate descriptor accepted'}
    $count++
}
Write-Output ('PASS: '+$count+' content identity checks; PS '+$PSVersionTable.PSVersion+'; culture/order-independent, real changes detected, duplicates/invalid descriptors rejected.')

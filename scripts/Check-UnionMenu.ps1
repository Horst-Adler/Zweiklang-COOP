[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$script=Get-Content -LiteralPath (Join-Path $root 'src\OreboundCoop\scripts\OreboundCoop_Menu.d') -Raw
$known=@('C_MENU_ITEM','C_MENU_DEF','C_MENU_ITEM_DEF','C_MENU_ITEM_UNION_DEF')
$instances=@{}
foreach($m in [regex]::Matches($script,'(?is)\b(prototype|instance)\s+(\w+)\s*\((\w+)\)\s*\{(.*?)\};')) {
    $kind=$m.Groups[1].Value;$name=$m.Groups[2].Value;$parent=$m.Groups[3].Value;$body=$m.Groups[4].Value
    if($parent -notin $known){throw "Unknown or forward-declared base type: $name ($parent)"}
    if($kind -eq 'prototype' -and $parent -ne 'C_MENU_ITEM'){throw "Invalid prototype base: $name"}
    if($name -in $known){throw "Duplicate definition: $name"}
    $known+=$name
    if($kind -eq 'instance'){$instances[$name]=$body}
}
$menu=$instances['MENU_ZWEIKLANG_COOP']
$slots=@{}
$rows=@()
foreach($m in [regex]::Matches($menu,'items\[(\d+)\]\s*=\s*"([^"]+)"')) {
    $slot=$m.Groups[1].Value;$name=$m.Groups[2].Value
    if($slots.ContainsKey($slot)){throw "Duplicate menu slot: $slot"};$slots[$slot]=$name
    if($name -ne 'UNION_MENUITEM_BACK' -and -not $instances.ContainsKey($name)){throw "Missing instance: $name"}
    if($name -match '_EDIT$|_CHOICE$|_APPLY$|_START$|_LEAVE$') {
        $y=[regex]::Match($instances[$name],'posy\s*=\s*(\d+)')
        if(-not $y.Success){throw "Missing position: $name"}
        $rows+=[int]$y.Groups[1].Value
    }
}
if($rows.Count -ne 13){throw 'Expected four text fields, six choices and three actions.'}
$rows=@($rows|Sort-Object)
for($i=1;$i -lt $rows.Count;$i++){if($rows[$i]-$rows[$i-1] -lt 350){throw 'Overlapping control rows'}}
foreach($field in @('NAME','HOST','CLIENT','PORT')) {
    if(-not $instances[('MENUITEM_ZK_'+$field+'_EDIT')].Contains('ZWEIKLANG_EDIT_'+$field)){throw "Missing editor command: $field"}
}
foreach($key in @('Name','Host','Client','Port','Role','Gender','LogLevel','EngineMessages','FriendlyFire','Language')) {
    if($script -notmatch ('onchgsetoption\s*=\s*"'+$key+'"')){throw "Missing option binding: $key"}
}
Write-Output 'PASS: declaration order, base types, instances, menu slots, 13 control rows and 10 option bindings.'
Write-Output 'Note: structural checks do not replace the Union parser or in-game acceptance.'

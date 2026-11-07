[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$script=Get-Content -LiteralPath (Join-Path $root 'src\ZweiklangCoop\scripts\ZweiklangCoop_Menu.d') -Raw
$known=@('C_MENU_ITEM','C_MENU_DEF','C_MENU_ITEM_DEF','C_MENU_ITEM_UNION_DEF')
$instances=@{}
foreach($m in [regex]::Matches($script,'(?is)\b(prototype|instance)\s+(\w+)\s*\((\w+)\)\s*\{(.*?)\};')) {
    $kind=$m.Groups[1].Value;$name=$m.Groups[2].Value;$parent=$m.Groups[3].Value;$body=$m.Groups[4].Value
    if($parent -notin $known){throw "Unbekannter oder zu spaet definierter Basistyp: $name ($parent)"}
    if($kind -eq 'prototype' -and $parent -ne 'C_MENU_ITEM'){throw "Ungueltige Prototypbasis: $name"}
    if($name -in $known){throw "Doppelte Definition: $name"}
    $known+=$name
    if($kind -eq 'instance'){$instances[$name]=$body}
}
$menu=$instances['MENU_ZWEIKLANG_COOP']
$slots=@{}
$rows=@()
foreach($m in [regex]::Matches($menu,'items\[(\d+)\]\s*=\s*"([^"]+)"')) {
    $slot=$m.Groups[1].Value;$name=$m.Groups[2].Value
    if($slots.ContainsKey($slot)){throw "Doppelter Menueplatz: $slot"};$slots[$slot]=$name
    if($name -ne 'UNION_MENUITEM_BACK' -and -not $instances.ContainsKey($name)){throw "Instanz fehlt: $name"}
    if($name -match '_EDIT$|_CHOICE$|_APPLY$|_START$|_LEAVE$') {
        $y=[regex]::Match($instances[$name],'posy\s*=\s*(\d+)')
        if(-not $y.Success){throw "Position fehlt: $name"}
        $rows+=[int]$y.Groups[1].Value
    }
}
if($rows.Count -ne 12){throw 'Erwartet: vier Textfelder, fuenf Auswahlfelder und drei Aktionen.'}
$rows=@($rows|Sort-Object)
for($i=1;$i -lt $rows.Count;$i++){if($rows[$i]-$rows[$i-1] -lt 350){throw 'Ueberlappende Bedienzeilen'}}
foreach($field in @('NAME','HOST','CLIENT','PORT')) {
    if(-not $instances[('MENUITEM_ZK_'+$field+'_EDIT')].Contains('ZWEIKLANG_EDIT_'+$field)){throw "Editorbefehl fehlt: $field"}
}
foreach($key in @('Name','Host','Client','Port','Role','Gender','LogLevel','EngineMessages','FriendlyFire')) {
    if($script -notmatch ('onchgsetoption\s*=\s*"'+$key+'"')){throw "Optionsanbindung fehlt: $key"}
}
Write-Output 'PASS: Deklarationsreihenfolge, Basistypen, Instanzen, Menueplaetze, 12 Bedienzeilen und 9 Optionsanbindungen.'
Write-Output 'Hinweis: Strukturpruefung ersetzt nicht den Union-Parser und den Ingame-Test.'

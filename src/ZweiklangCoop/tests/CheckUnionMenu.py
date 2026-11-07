"""Offline integration checks; not a replacement for Union's runtime parser."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
script=(root/'scripts/ZweiklangCoop_Menu.d').read_text(encoding='ascii')
code=(root/'src/Plugin.cpp').read_text(encoding='utf-8-sig')
assert 'Parser = Menu;' in script and 'After = zUnionMenu.d;' in script
assert 'instance MenuItem_Union_Auto_ZweiklangCoop(C_MENU_ITEM_UNION_DEF)' in script
# Daedalus prototypes require the class itself, not another prototype.
prototypes=re.findall(r'prototype\s+(\w+)\s*\((\w+)\)\s*\{(.*?)\};',script,re.S|re.I)
assert len(prototypes)==3
for name,parent,body in prototypes:
    assert parent.upper()=='C_MENU_ITEM',(name,parent)
    assert body.lstrip().startswith('C_MENU_ITEM_DEF();'),name
instances={name.upper():body for name,body in re.findall(r'instance\s+(\w+)\s*\([^)]+\)\s*\{(.*?)\};',script,re.S|re.I)}
assert len(instances)==len(re.findall(r'\binstance\s+',script,re.I)),'duplicate instances'
menu=instances['MENU_ZWEIKLANG_COOP']
refs=re.findall(r'items\[\d+\]\s*=\s*"([^"]+)"',menu)
assert len(refs)==len(set(refs)),'duplicate menu slots'
for name in refs:
    assert name.upper() in instances or name=='UNION_MENUITEM_BACK',name
assert '"MENU_ZWEIKLANG_COOP"' in code
assert '"ZWEIKLANG_APPLY"' in code and '"ZWEIKLANG_APPLY"' in script
assert {'Name','Host','Client','Port','Role','Gender','LogLevel','EngineMessages','FriendlyFire'}==set(re.findall(r'onchgsetoption\s*=\s*"([^"]+)"',script))
assert code.count('ZWEIKLANGCOOP_MENU')==1
for field in ('NAME','HOST','CLIENT','PORT'):
    assert 'MENUITEM_ZK_'+field+'_EDIT' in instances
    assert 'ZWEIKLANG_EDIT_'+field in instances['MENUITEM_ZK_'+field+'_EDIT']
assert 'input->Run();' in code and 'input->HasBeenCanceled()?previous:input->GetValue()' in code
assert 'RefreshUnionInputs(menu);' in code
refresh=code.split('void RefreshUnionInputs(zCMenu* menu)',1)[1].split('bool UnionMenuEdit',1)[0]
assert 'input->Hide();' in refresh,'native input must remain hidden behind its visible editor control'
actions=code.split('int __fastcall MenuAction(zCMenu* menu,',1)[1].split('int __fastcall EngineReport',1)[0]
assert actions.count('menu->m_exitState=zCMenu::NONE;return 0;')==4,'local actions must not close zCMenu::Run'
assert 'menu->m_exitState=zCMenu::NONE;return 1;' not in actions
assert all(marker not in code for marker in ('MENU_MAIN','new zCMenuItem','InsertInWin','ShowSettings','OpenOptions'))
rows=[]
for name in refs:
    body=instances.get(name.upper(),'')
    y=re.search(r'posy\s*=\s*(\d+)',body)
    if y and ('_INPUT' in name or '_CHOICE' in name or name.endswith('_APPLY')):
        rows.append((name,int(y.group(1))))
assert len(rows)==10
rows.sort(key=lambda row:row[1])
assert all(b[1]-a[1]>=350 for a,b in zip(rows,rows[1:])),'control overlap'
# Ignore braces/parentheses inside text or comments for a basic structure check.
tokens=re.sub(r'//[^\n]*|"[^"\n]*"','',script)
for left,right in (('{','}'),('(',')')):
    depth=0
    for token in tokens:
        if token==left:depth+=1
        if token==right:depth-=1
        assert depth>=0
    assert depth==0
print('PASS: Union script dependency, registration, references, controls, save command and no main-menu mutation')
print('NOTE: Daedalus runtime compilation and in-game interaction remain untested.')

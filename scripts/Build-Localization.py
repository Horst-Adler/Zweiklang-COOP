"""Validate and generate the own English/German UI catalogue; standard Python only."""
from pathlib import Path
import json,re
root=Path(__file__).resolve().parents[1];base=root/'src/OreboundCoop';data=json.loads((base/'localization/ui.json').read_text(encoding='utf-8'))
assert data['schema']==1 and data['languages']==['en','de'] and data['defaultLanguage']=='en'
ids=set();records=[]
for e in data['entries']:
 assert e['id'] not in ids and re.fullmatch('[a-z0-9_.]+',e['id']);ids.add(e['id'])
 assert isinstance(e['en'],str) and e['en'] and isinstance(e['de'],str) and e['de']
 assert re.findall(r'\{\d+\}',e['en'])==re.findall(r'\{\d+\}',e['de']),e['id']
 records.append(' {'+','.join('u8'+json.dumps(e[k],ensure_ascii=True) for k in ['id','en','de'])+'},')
menu=[]
for e in data['menu']:
 assert e['id'] in ids and e['index'] in (0,1)
 menu.append(' {'+json.dumps(e['symbol'])+','+str(e['index'])+','+json.dumps(e['id'])+'},')
text='// Generated from localization/ui.json; do not edit by hand.\n#pragma once\nnamespace orebound::i18n {\nstruct Entry {const char* id;const char* en;const char* de;};\ninline constexpr Entry Catalogue[]={\n'+'\n'.join(records)+'\n};\nstruct MenuText {const char* symbol;int index;const char* id;};\ninline constexpr MenuText MenuCatalogue[]={\n'+'\n'.join(menu)+'\n};\n}\n'
target=base/'include/coop/LocalizationCatalogue.h'
if not target.exists() or target.read_text(encoding='utf-8')!=text:target.write_text(text,encoding='utf-8',newline='\n')
entries={e['id']:e for e in data['entries']}
script=base/'ninja/Ninja/ZweiklangCore/Content/Bridge.d'
old=script.read_text(encoding='ascii')
if data.get('script'):
 assignments=[]
 for language in ['de','en']:
  lines=[]
  for binding in data['script']:
   assert binding['id'] in entries and re.fullmatch(r'Patch_\w+',binding['symbol'])
   value=entries[binding['id']][language];value.encode('ascii')
   lines.append('            '+binding['symbol']+' = '+json.dumps(value)+';')
  assignments.append('\n'.join(lines))
 block='        // BEGIN GENERATED LOCALIZED JOURNAL TEXT\n        if (Patch_ZweiklangCore_Language == 1)\n        {\n'+assignments[0]+'\n        }\n        else\n        {\n'+assignments[1]+'\n        };\n        // END GENERATED LOCALIZED JOURNAL TEXT'
 changed,count=re.subn(r'        // BEGIN GENERATED LOCALIZED JOURNAL TEXT.*?        // END GENERATED LOCALIZED JOURNAL TEXT',lambda m:block,old,flags=re.S)
 assert count==1
 if changed!=old:script.write_text(changed,encoding='ascii',newline='\n')
print('PASS:',len(ids),'complete English/German entries;',len(menu),'menu bindings; unique IDs and matching placeholders.')

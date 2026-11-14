"""Prepare local co-op bridges from separately obtained original archives.

Third-party archives are not published with this repository.
"""
from pathlib import Path
import argparse,struct,hashlib,json,shutil,difflib
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input',type=Path,required=True,help='Directory containing separately obtained selected original files')
parser.add_argument('--output',type=Path,required=True,help='Local output directory outside the Git index')
parser.add_argument('--modules',nargs='*',choices=['Quickloot','Sprint','Toolkit','PatchMenu'],default=['Quickloot','Sprint','Toolkit','PatchMenu'],help='Prepare selected optional modules only; include Toolkit for dependent modules')
args=parser.parse_args();source=args.input.resolve();dest=args.output.resolve()
if source==dest:raise SystemExit('Input and output directories must differ.')
expected={
 'Quickloot.vdf':'6676CC0D59CA803DF1D0E626AB1C3191FBEFEFF63EF6E823413395D22F0BBC34',
 'Sprint.vdf':'1A74DCC2986252D34F927526780DAAAFC045F6891511209EA892B496CC150E4C',
 'Toolkit.vdf':'08A8873043AE3E95C808E0AD519E00F302239BF58A561F3CA22EA6D00F5DC260',
 'PatchMenu.vdf':'8DA288D5FCFF42EDDC2CBA99548A9C03A464E146DD9C5CFE39F7DAD2FFD08F7A',
 'Ninja.dll':'38CE2836A74227BEAF08C6972CD263389BC3D6F591BFF35AE812C83C6241CA8B'}
selected=set(args.modules)
if selected&{'Quickloot','Sprint','PatchMenu'}:selected.add('Toolkit')
needed={name+'.vdf' for name in selected}
if (source/'Ninja.dll').is_file():needed.add('Ninja.dll')
for name in needed:
    digest=expected[name]
    path=source/name
    if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest().upper()!=digest:
        raise SystemExit('Missing or unsupported original file: '+name)
dest.mkdir(parents=True,exist_ok=True)
def entries(raw):
    count=struct.unpack_from('<I',raw,272)[0];catalog=struct.unpack_from('<I',raw,288)[0]
    result=[]
    for i in range(count):
        at=catalog+i*80;name=raw[at:at+64].decode('ascii').strip('\0 ');offset,size,flags,attr=struct.unpack_from('<4I',raw,at+64)
        result.append((at,name,offset,size,flags,attr))
    return result
def adapt(name,file,function,marker,guard):
    original=(source/name).read_bytes();raw=bytearray(original);found=0;before=after='';files={}
    for at,entry,offset,size,flags,attr in entries(original):
        if flags&0x80000000:continue
        data=original[offset:offset+size]
        if entry.upper()==file:
            found+=1;before=data.decode('cp1252');needle=function+' {';assert before.count(needle)==1,(name,needle)
            injected='''
    // Zweiklang private coop bridge. Keep the upstream path outside a session.
    var int zkGuard; zkGuard = MEM_GetSymbolIndex("'''+guard+'''");
    if (zkGuard != -1) {
        MEM_CallByID(zkGuard);
        if (MEM_PopIntResult()) { return; };
    };
'''
            if name=='Quickloot.vdf':
                # Let upstream timers restore temporary RMB handling and finish
                # existing loot messages even while inventory mutations are blocked.
                place='// Check for RMB pressed / held';assert before.count(place)==1
                body=before.replace(place,injected+'\n\t'+place)
            else:body=before.replace(needle,needle+injected)
            after='const int '+marker+' = 1;\r\n'+body
            data=after.encode('cp1252')
        files[at]=data
    assert found==1
    end=struct.unpack_from('<I',raw,288)[0]+len(entries(raw))*80
    raw=raw[:end];volume=0
    for at,entry,offset,size,flags,attr in entries(original):
        if flags&0x80000000:continue
        data=files[at];struct.pack_into('<II',raw,at+64,len(raw),len(data));raw.extend(data);volume+=len(data)
    struct.pack_into('<I',raw,284,volume)
    target=dest/'Data'/name;target.parent.mkdir(exist_ok=True);target.write_bytes(raw)
    # Check every referenced file, including all animation/font assets.
    for at,entry,offset,size,flags,attr in entries(raw):
        if not flags&0x80000000:assert raw[offset:offset+size]==files[at]
    diff=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile=name+'/'+file,tofile='Zweiklang/'+file))
    (dest/(name+'.compat.patch')).write_text(diff,encoding='utf-8')
    return {'File':name,'OriginalSHA256':hashlib.sha256(original).hexdigest().upper(),'AdaptedSHA256':hashlib.sha256(raw).hexdigest().upper(),'ChangedScript':file,'Guard':guard,'Marker':marker}
changes=[]
if 'Quickloot' in selected:changes.append(adapt('Quickloot.vdf','INIT.D','func void _Ninja_Quickloot_Loot()','NINJA_QUICKLOOT_ZK_BRIDGE_REV','ZK_COOP_QOL_BLOCK_QUICKLOOT'))
if 'Sprint' in selected:changes.append(adapt('Sprint.vdf','NINJA_SPRINT_INIT.D','func void Ninja_Sprint_getPartyMembers(var C_NPC slf)','NINJA_SPRINT_ZK_BRIDGE_REV','ZK_COOP_QOL_BLOCK_PARTYSPRINT'))
(dest/'Data').mkdir(exist_ok=True,parents=True)
for name in ['Toolkit.vdf','PatchMenu.vdf']:
    if name in needed:shutil.copy2(source/name,dest/'Data'/name)
(dest/'System').mkdir(exist_ok=True)
if 'Ninja.dll' in needed:shutil.copy2(source/'Ninja.dll',dest/'System/Ninja.dll')
files=[]
for folder in ['Data','System']:
    for path in sorted((dest/folder).iterdir()):
        if path.is_file():files.append(dict(relative=path.relative_to(dest).as_posix(),sha256=hashlib.sha256(path.read_bytes()).hexdigest().upper(),bytes=path.stat().st_size))
(dest/'manifest.json').write_text(json.dumps(dict(bridge_revision=1,changes=changes,files=files),indent=2),encoding='utf-8')
print('Compatibility bridges prepared. Original inputs unchanged; do not publish the foreign output archives.')

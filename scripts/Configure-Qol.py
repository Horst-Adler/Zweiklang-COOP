"""Preserve existing settings/encoding; add only missing QoL defaults and loader."""
from pathlib import Path
import sys,re
root=Path(__file__).resolve().parents[1]
def set_option(path,section,key,value,merge=False):
    raw=path.read_bytes() if path.exists() else b'';lines=raw.splitlines(keepends=True)
    active=False;start=None;stop=len(lines)
    for i,line in enumerate(lines):
        scan=line[3:] if i==0 and line.startswith(b'\xef\xbb\xbf') else line
        heading=re.match(rb'\s*\[([^]]+)\]',scan)
        if heading:
            if active:stop=i;break
            active=heading[1].decode('ascii').lower()==section.lower()
            if active:start=i
        elif active:
            match=re.match(rb'\s*'+re.escape(key.encode())+rb'\s*=(.*)',line,re.I)
            if match:
                if not merge:return
                old=match[1].strip().decode('ascii')
                if any(x.strip().lower() in ('ninja.dll**','ninja**') for x in old.split(',')):return
                old=','.join(x.strip() for x in old.split(',') if x.strip().lower() not in ('ninja.dll','ninja'))
                lines[i]=(key+'='+old.rstrip(',')+(',' if old.strip(', ') else '')+value+'\r\n').encode('ascii');path.write_bytes(b''.join(lines));return
    if start is None:
        raw=raw+(b'' if not raw or raw.endswith(b'\n') else b'\r\n')+('['+section+']\r\n'+key+'='+value+'\r\n').encode('ascii')
    else:
        if stop and not lines[stop-1].endswith(b'\n'):lines[stop-1]+=b'\r\n'
        lines.insert(stop,(key+'='+value+'\r\n').encode('ascii'));raw=b''.join(lines)
    path.write_bytes(raw)
def configure(game):
    for relative in ['System/Gothic.ini','System/SystemPack.ini','System/Union.ini']:
        path=game/relative
        if path.exists() and path.read_bytes().startswith((b'\xff\xfe',b'\xfe\xff')):
            raise ValueError('UTF-16-Laufzeitkonfiguration nicht unterstützt; keine Änderungen vorgenommen.')
    gothic=game/'System/Gothic.ini'
    for key,value in [('Strength','0'),('Perma','1'),('Anim','0'),('Weapon','1'),('Party','1')]:set_option(gothic,'NINJA_SPRINT',key,value)
    for key,value in [('Key','RMB'),('Prefix','Erhalten:'),('UseAnimations','1'),('AnimSpeed','700')]:set_option(gothic,'NINJA_QUICKLOOT',key,value)
    set_option(game/'System/SystemPack.ini','PLUGINS','PluginList','Ninja.dll**',merge=True)
    if (game/'System/Union.ini').exists():set_option(game/'System/Union.ini','PLUGINS','PluginList','Ninja.dll**',merge=True)
if __name__=='__main__':
    game=Path(sys.argv[1]).resolve()
    if not (game/'System/Gothic2.exe').is_file():raise SystemExit('GamePath enthält keine Gothic2.exe.')
    configure(game)
    print('QoL defaults and Union Ninja loader configured; existing personal/graphics/key settings preserved:',game)

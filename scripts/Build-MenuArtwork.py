"""Convert own menu art to a uniquely named opaque BC1 texture and VDF.

Only resize/format conversion; no creative editing. Original textures and menus
are not overridden. Requires separately installed Pillow and zenkit.
"""
from pathlib import Path
import argparse,datetime,hashlib,io,json,struct,sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools/python-libs'))
from PIL import Image,ImageChops,ImageStat
import zenkit
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--image',required=True,type=Path)
parser.add_argument('--output',type=Path,default=root/'build/alpha/menu')
args=parser.parse_args();source=args.image.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
image=Image.open(source).convert('RGB').resize((2048,1024),Image.Resampling.LANCZOS)
mips=[image]
# The Gothic texture reader shifts both dimensions without clamping the short
# side. End at 2x1 for this 2:1 texture; a subsequent 1x1 level is not supported.
while min(mips[-1].size)>1:
 prev=mips[-1];mips.append(prev.resize((max(1,prev.width//2),max(1,prev.height//2)),Image.Resampling.LANCZOS))
encoded=[];decoded=[]
for im in mips:
 stream=io.BytesIO();im.save(stream,format='DDS',pixel_format='DXT1');data=stream.getvalue()
 assert data[:4]==b'DDS ' and data[84:88]==b'DXT1'
 expected=((im.width+3)//4)*((im.height+3)//4)*8
 assert len(data[128:])==expected,(im.size,len(data[128:]),expected)
 encoded.append(data[128:]);decoded.append(Image.open(io.BytesIO(data)).convert('RGBA'))
red,green,blue=mips[-1].getpixel((0,0))
blob=struct.pack('<4s8I',b'ZTEX',0,int(zenkit.TextureFormat.DXT1),image.width,image.height,len(mips),image.width,image.height,(255<<24)|(red<<16)|(green<<8)|blue)+b''.join(reversed(encoded))
texture_name='OREBOUND_COOP_MENU-C.TEX';texture_path=out/texture_name;texture_path.write_bytes(blob)
check=zenkit.Texture.load(texture_path)
assert (check.width,check.height,check.mipmap_count)==(2048,1024,11)
assert check.format==zenkit.TextureFormat.DXT1
for level,im in enumerate(decoded):
 native=Image.frombytes('RGBA',im.size,check.mipmap_rgba(level))
 assert native.getchannel('A').getextrema()==(255,255),('nonopaque mip',level)
 assert max(hi for lo,hi in ImageStat.Stat(ImageChops.difference(native,im)).extrema)<=1,level
quality=ImageStat.Stat(ImageChops.difference(image,decoded[0].convert('RGB'))).mean
assert max(quality)<8,quality
vfs=zenkit.Vfs();stamp=datetime.datetime(2026,11,14,15,0,tzinfo=datetime.timezone.utc)
virtual='_WORK/DATA/TEXTURES/_COMPILED/'+texture_name;vfs.mkdir('_WORK/DATA/TEXTURES/_COMPILED').create(texture_name,blob,timestamp=stamp)
target=out/'OreboundCoop_Menu.vdf';vfs.save(target,zenkit.GameVersion.GOTHIC2,stamp)
mount=zenkit.Vfs();mount.mount_disk(target);node=mount.resolve(virtual)
assert node and node.is_file() and node.open().data==blob
raw=target.read_bytes();count=struct.unpack_from('<I',raw,272)[0];catalog=struct.unpack_from('<I',raw,288)[0];entries=[]
for at in range(catalog,catalog+count*80,80):
 name=raw[at:at+64].decode('ascii').strip('\0 ');offset,size,flags,_=struct.unpack_from('<4I',raw,at+64)
 if not flags&0x80000000:assert raw[offset:offset+size]==blob;entries.append(name)
assert entries==[texture_name],'Never replace STARTSCREEN or any original menu/logo texture'
sha=hashlib.sha256(raw).hexdigest().upper()
catalogue=dict(Schema=1,Optional=True,File='Data/OreboundCoop_Menu.vdf',SHA256=sha,Texture='OREBOUND_COOP_MENU.TGA',MenuABI=1,SourceSHA256=hashlib.sha256(source.read_bytes()).hexdigest().upper())
(root/'config/menu-artwork.json').write_text(json.dumps(catalogue,indent=2)+'\n',encoding='utf-8')
header='#pragma once\nnamespace orebound::menuart { inline constexpr const char* ArchiveHash="'+sha+'"; inline constexpr const char* TextureName="OREBOUND_COOP_MENU.TGA"; inline constexpr int MenuABI=1; }\n'
header_path=root/'src/OreboundCoop/include/coop/MenuArtworkHash.h'
if not header_path.exists() or header_path.read_text(encoding='utf-8')!=header:header_path.write_text(header,encoding='utf-8')
manifest=dict(catalogue,Bytes=len(raw),TextureSize=[2048,1024],Mipmaps=len(mips),Format='Opaque DXT1 / BC1',Entries=entries,AllMipPixelsVerified=True,IndependentVdfMountVerified=True,CompressionMeanAbsoluteRGBError=quality,OriginalResourcesOverridden=False,GamesStarted=False,NativeVisualAcceptance=False)
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print('PASS: unique opaque BC1 menu texture, 11 supported mips, independent DDS/pixel and VDF verification; no original texture overrides. RGB error:',quality)

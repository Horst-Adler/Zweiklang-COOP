"""Isolated byte-preserving QoL configuration regressions; no game starts."""
from pathlib import Path
import importlib.util,tempfile
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('configure_qol',root/'scripts/Configure-Qol.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
cases=[
    (b'[VIDEO]\r\nzVidResFullscreenX=1920\r\n[KEYS]\r\nkeyAction=100\r\n',b'[OTHER]\r\nvalue=42\r\n'),
    ('; Grüße\r\n[NINJA_SPRINT]\r\nStrength=30\r\nPerma=0\r\n'.encode('cp1252'),b'[PLUGINS]\r\nPluginList=Other.dll,Ninja.dll\r\n'),
    (b'\xef\xbb\xbf[NINJA_SPRINT]\nStrength=99\n',b'[PLUGINS]\nPluginList=Other.dll,Ninja.dll**\n'),
    (b'[NINJA_QUICKLOOT]\nKey=V\nPrefix=Beute:\nAnimSpeed=900',b''),
    (b'[VIDEO]\nvalue=2',b'[PLUGINS]\nPluginList=Ninja,Other.dll\n'),
]
with tempfile.TemporaryDirectory(prefix='orebound-config-test-') as directory:
    game=Path(directory).resolve()
    assert game.is_relative_to(Path(tempfile.gettempdir()).resolve())
    system=game/'System';system.mkdir()
    for number,(gothic,systempack) in enumerate(cases,1):
        (system/'Gothic.ini').write_bytes(gothic);(system/'SystemPack.ini').write_bytes(systempack)
        (system/'Union.ini').write_bytes(b'[PLUGINS]\nPluginList=Own.dll\n')
        module.configure(game)
        after={path.name:path.read_bytes() for path in system.iterdir()}
        assert b'Own.dll,Ninja.dll**' in after['Union.ini']
        assert b'Ninja.dll**' in after['SystemPack.ini']
        for line in gothic.splitlines():
            assert line in after['Gothic.ini'],(number,line)
        assert after['Gothic.ini'].count(b'[NINJA_SPRINT]')==1
        module.configure(game)
        assert after=={path.name:path.read_bytes() for path in system.iterdir()},number
    original=b'; minimal profile\r\n[VIDEO]\r\nvalue=42\r\n'
    (system/'Gothic.ini').write_bytes(original)
    module.configure(game,loader_only=True)
    assert (system/'Gothic.ini').read_bytes()==original,'minimal loader setup changed gameplay/graphics settings'
    assert b'Ninja.dll**' in (system/'SystemPack.ini').read_bytes()
    (system/'Gothic.ini').write_bytes('[NINJA_SPRINT]\r\nStrength=55\r\n'.encode('utf-16'))
    before={path.name:path.read_bytes() for path in system.iterdir()}
    try:module.configure(game)
    except ValueError:pass
    else:raise AssertionError('Unsupported format was not rejected')
    assert before=={path.name:path.read_bytes() for path in system.iterdir()}
print('PASS: five configuration cases, retained values/encoding, UTF-8 BOM and idempotence; UTF-16 rejected before any mutation.')

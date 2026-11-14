"""Audit the explicit allowlist, complete index and publishable own files."""
from pathlib import Path,PurePosixPath
import argparse,hashlib,json,re,subprocess
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--staged',action='store_true',help='Check every file managed by the Git index')
args=parser.parse_args()
policy=json.loads((root/'publication-allowlist.json').read_text(encoding='utf-8'))
allowed=policy['allowed_paths']
assert len(allowed)==len(set(allowed)), 'Duplicate publication allowlist paths'
assert len(allowed)==len({value.casefold() for value in allowed}), 'Allowlist contains case-colliding Windows paths'
forbidden_parts={'games','website','client','backups','downloads','local','work','build','dist','references','__pycache__','third_party'}
binary=policy['synthetic_fixtures']
problems=[];records=[]
secret_patterns=[
    re.compile(r'gh[pousr]_[A-Za-z0-9]{25,}'),
    re.compile(r'github_pat_[A-Za-z0-9_]{30,}'),
    re.compile(r'AKIA[A-Z0-9]{16}'),
    re.compile(r'sk-(?:proj-)?[A-Za-z0-9_-]{30,}'),
    re.compile(r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),
    re.compile(r'(?i)(?:api[_-]?key|access[_-]?token|client_secret|password)\s*[:=]\s*["\'][^"\'\r\n]{12,}["\']'),
    re.compile(r'[A-Za-z]:\\Users\\[^\\\r\n]+\\'),
    re.compile(r'\\\\[A-Za-z0-9_.-]+\\(?:[A-Z]\$?\\|Users\\)'),
    re.compile(r'(?i)[\w.+-]+@(?:privaterelay\.appleid\.com|gmail\.com|outlook\.com|gmx\.(?:de|net)|web\.de)')
]
for relative in allowed:
    path=PurePosixPath(relative)
    if path.is_absolute() or '..' in path.parts or any(x.lower() in forbidden_parts for x in path.parts):
        problems.append(relative+': excluded path');continue
    target=root/relative
    if not target.is_file() or target.is_symlink():problems.append(relative+': missing or symbolic link');continue
    raw=target.read_bytes()
    if len(raw)>512*1024:problems.append(relative+': unexpectedly large file')
    digest=hashlib.sha256(raw).hexdigest()
    if relative in binary:
        if digest!=binary[relative]:problems.append(relative+': synthetic fixture hash mismatch')
    else:
        try:text=raw.decode('utf-8-sig')
        except UnicodeDecodeError:problems.append(relative+': unapproved binary file');continue
        if '\0' in text:problems.append(relative+': binary NUL data')
        if any(ord(c)<32 and c not in '\t\r\n' for c in text):problems.append(relative+': unexpected text control character')
        for pattern in secret_patterns:
            if pattern.search(text):problems.append(relative+': possible private data/secret (value not printed)')
    if raw.startswith((b'MZ',b'DDS ',b'ZTEX',b'PK\x03\x04')):problems.append(relative+': executable, image or archive')
    records.append(dict(path=relative,bytes=len(raw),sha256=digest))
if args.staged:
    result=subprocess.run(['git','ls-files','--stage','-z'],cwd=root,check=True,capture_output=True).stdout
    indexed=[]
    for value in result.split(b'\0'):
        if not value:continue
        header,path=value.split(b'\t',1);mode,oid,stage=header.decode().split()
        relative=path.decode('utf-8');indexed.append(relative)
        if relative not in allowed:problems.append(relative+': not in the publication allowlist')
        if mode!='100644' or stage!='0':problems.append(relative+': invalid mode/unresolved conflict')
        staged=subprocess.run(['git','cat-file','blob',oid],cwd=root,check=True,capture_output=True).stdout
        if relative in allowed and staged!=(root/relative).read_bytes():problems.append(relative+': index differs from audited working file')
    if set(indexed)!=set(allowed):problems.append('index does not match the full allowlist')
if problems:
    for problem in problems:print('ERROR:',problem)
    raise SystemExit(1)
print(f'PASS: {len(records)} explicitly approved files; no original games, private website, foreign archives or detected personal data.')
if args.staged:print('PASS: complete index exactly matches audited files.')

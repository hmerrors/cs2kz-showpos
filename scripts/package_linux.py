"""Package only this plugin and its configuration, never game/dependency binaries."""
from pathlib import Path
import argparse
import hashlib
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('--binary', required=True, type=Path)
parser.add_argument('--output', default='dist', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
data = args.binary.read_bytes()
if data[:6] != b'\x7fELF\x02\x01' or data[16:20] != b'\x03\x00\x3e\x00':
    raise SystemExit('Expected an ELF64 x86-64 plugin')
args.output.mkdir(parents=True, exist_ok=True)
archive = args.output / 'CS2KZ-ShowPos-Standalone-Linux.zip'
files = {
    'addons/showpos/bin/linuxsteamrt64/showpos.so': args.binary,
    'addons/showpos/gamedata/showpos.games.txt': root/'gamedata/showpos.games.txt',
    'cfg/showpos.cfg': root/'cfg/showpos.cfg',
}
for name in ['README.md','LICENSE','THIRD_PARTY.md','CHANGELOG.md']:
    files[name] = root/name
for file in (root/'docs').glob('*.md'):
    files['docs/'+file.name] = file
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for name, file in sorted(files.items()):
        info = zipfile.ZipInfo(name)
        info.create_system = 3
        info.external_attr = (0o100755 if name.endswith('.so') else 0o100644) << 16
        info.compress_type = zipfile.ZIP_DEFLATED
        z.writestr(info, file.read_bytes())
    info = zipfile.ZipInfo('addons/metamod/zzz_showpos.vdf')
    info.create_system = 3
    info.external_attr = 0o100644 << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    z.writestr(info, '"Metamod Plugin"\n{\n "alias" "showpos"\n "file" "addons/showpos/bin/linuxsteamrt64/showpos.so"\n}\n')
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert len([n for n in z.namelist() if n.endswith('.so')]) == 1
(args.output/'SHA256SUMS.txt').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n', encoding='utf-8')
print(archive)

"""Package one native ShowPos binary, configuration and documentation."""

import argparse
import hashlib
from pathlib import Path
import re
import struct
import zipfile


def validate_binary(data, platform):
    if platform == "linux":
        if len(data) < 64 or data[:6] != b"\x7fELF\x02\x01" or data[16:20] != b"\x03\x00\x3e\x00":
            raise ValueError("Expected an ELF64 x86-64 shared library")
        return
    if len(data) < 64 or data[:2] != b"MZ":
        raise ValueError("Expected a Windows x64 DLL")
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if pe_offset > len(data) - 24 or data[pe_offset:pe_offset + 6] != b"PE\0\0\x64\x86":
        raise ValueError("Expected a PE x64 DLL")
    if not struct.unpack_from("<H", data, pe_offset + 22)[0] & 0x2000:
        raise ValueError("PE image is not a DLL")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", required=True, choices=["windows", "linux"])
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--version", default="1.0.0-rc4")
    parser.add_argument("--output", default="dist", type=Path)
    args = parser.parse_args(argv)
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:-[A-Za-z0-9.]+)?", args.version):
        parser.error("version must be a release version such as 1.0.0-rc4")
    data = args.binary.read_bytes()
    try:
        validate_binary(data, args.platform)
    except ValueError as exc:
        parser.error(str(exc))
    root = Path(__file__).resolve().parents[1]
    args.output.mkdir(parents=True, exist_ok=True)
    if args.platform == "windows":
        platform_name = "Windows-x64"
        binary_name = "addons/showpos/bin/win64/showpos.dll"
    else:
        platform_name = "Linux-x86_64"
        binary_name = "addons/showpos/bin/linuxsteamrt64/showpos.so"
    archive = args.output / f"CS2KZ-ShowPos-{args.version}-{platform_name}.zip"
    files = {
        binary_name: args.binary,
        "addons/showpos/gamedata/showpos.games.txt": root / "gamedata/showpos.games.txt",
        "cfg/showpos.cfg": root / "cfg/showpos.cfg",
    }
    for name in ["README.md", "LICENSE", "THIRD_PARTY.md", "CHANGELOG.md"]:
        files[name] = root / name
    for name in ["ARCHITECTURE.md", "BUILD.md", "LINUX.md", "RELEASE_NOTES.md"]:
        files["docs/" + name] = root / "docs" / name
    if args.platform == "windows":
        files["Install-ShowPos.ps1"] = root / "scripts/Install-ShowPos.ps1"
    vdf = ('"Metamod Plugin"\n{\n "alias" "showpos"\n'
           f' "file" "{binary_name}"\n}}\n').encode("utf-8")
    contents = {name: file.read_bytes() for name, file in files.items()}
    contents["addons/metamod/zzz_showpos.vdf"] = vdf
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as package:
        for name, content in sorted(contents.items()):
            info = zipfile.ZipInfo(name)
            info.create_system = 3
            info.external_attr = (0o100755 if name == binary_name else 0o100644) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            package.writestr(info, content)
    with zipfile.ZipFile(archive) as package:
        if package.testzip() is not None or set(package.namelist()) != set(contents):
            raise RuntimeError("Archive verification failed")
        if package.read(binary_name) != data:
            raise RuntimeError("Packaged binary differs from the requested build")
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_suffix(archive.suffix + ".sha256").write_text(
        f"{checksum}  {archive.name}\n", encoding="utf-8"
    )
    print(archive)
    print(f"SHA256 {checksum}")


if __name__ == "__main__":
    main()

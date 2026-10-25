"""Let the client's customize packet carry the sixth Esteria appearance value.

The Ascension client sends only gender and the five ordinary appearance bytes in
CMSG_CHAR_CUSTOMIZE.  EsteriaAppearance.dll keeps the sixth value for Highmountain,
Earthen and Haranir, but the client never gives it to the server.

This tool adds small thunks to the existing .eapp section of the patched
Ascension.exe:

* the customize thunk asks the DLL for the same HighmountainFields the creator uses,
  maps the local race to Esteria's, and appends one byte (Highmountain/Earthen) or
  eight bytes plus HRC1 (Haranir) to the packet;
* one context thunk calls EsteriaAppearanceContext before the character-select hook,
  so a cold start resolves the same Esteria sections as the preview.  The unit hooks
  stay untouched: they also run for NPCs/bots and must not overwrite the context.

The server side is in patches/core-customize-extra.patch.  The fingerprint is the
CoA Custom 1.9 client.  On another client the tool refuses instead of guessing.
Keep a copy of Ascension.exe before running it.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

IMAGE_BASE = 0x00400000
SITE_VA = 0x004D8EF9
THUNK_VA = 0x00DFC760
IAT_CONTEXT = 0x00DFC1C8
CHARACTER_GLOBAL = 0x00B6B1A0
APPEND_VA = 0x0047AFE0

# pushfd; pushad; map the local race; ask the DLL for its HighmountainFields;
# append one byte for Highmountain/Earthen and eight bytes plus HRC1 for Haranir;
# continue at the stock send.  No face byte is touched.
THUNK = bytes.fromhex(
    "9c60a1a0b1b60085c00f84b10000008b5018528b1db4c1df0081eb20ed"
    "02008d8bc0870200ffd183c40483f82e741683f830741183f831740c83"
    "f832743383f833742eeb7b83ec10a1a0b1b600508d442404508d8bb0fa"
    "0100ffd183c4080fb65424058d4de852e814e867ff83c410eb4f83ec10"
    "a1a0b1b600508d442404508d8bb0fa0100ffd183c408bf050000000fb6"
    "143c8d4de852e8e4e767ff4783ff0d7cedbf000000000fb69740c8df00"
    "8d4de852e8c9e767ff4783ff047cea83c410619d8d55e852895dfce9d2"
    "c66dff"
)
HRC1_VA = 0x00DFC840
HRC1 = b"HRC1"

SITE_BEFORE = bytes.fromhex("8d55e852895dfc")

# pushfd; pushad; push the selected character; call EsteriaAppearanceContext;
# add esp,4; popad; popfd; then jump to the original CoA thunk.  Only the
# character-select hook is used: the unit hooks run for NPCs/bots too and must not
# overwrite the context with a non-character pointer.
SELECT_CONTEXT = bytes.fromhex("9c6051ff15c8c1df0083c404619d")
CONTEXT_SITES = (
    (0x004E3E74, 0x00DFC880, SELECT_CONTEXT),  # EsteriaSelectExtra
)
EAPP_RAW_SIZE = 0xC00


def align(value: int, boundary: int) -> int:
    return (value + boundary - 1) // boundary * boundary


def jump_to(target: int, at: int) -> bytes:
    return b"\xE9" + struct.pack("<i", target - (at + 5))


def patch(source: Path, output: Path) -> dict:
    data = bytearray(source.read_bytes())
    if data[:2] != b"MZ":
        raise ValueError("not a PE file")

    nt = struct.unpack_from("<I", data, 0x3C)[0]
    section_count = struct.unpack_from("<H", data, nt + 6)[0]
    optional_size = struct.unpack_from("<H", data, nt + 20)[0]
    section_table = nt + 24 + optional_size
    sections = {}
    for index in range(section_count):
        header = section_table + index * 40
        raw = data[header:header + 40]
        name = raw[:8].split(b"\0", 1)[0].decode("latin-1")
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<4I", raw, 8)
        sections[name] = [header, virtual_size, virtual_address, raw_size, raw_offset]

    if ".eapp" not in sections:
        raise ValueError("no .eapp section: this is not the patched CoA Custom Ascension.exe")
    eapp_header, eapp_vsize, eapp_va, eapp_rawsize, eapp_raw = sections[".eapp"]
    if eapp_va != 0x009FC000 or eapp_rawsize != 0x800:
        raise ValueError("unexpected .eapp layout")

    # The new thunks need beyond the old 0x800-byte tail.  .eapp is the last
    # section, so extending its raw data is enough; SizeOfImage already covers it.
    struct.pack_into("<I", data, eapp_header + 8, EAPP_RAW_SIZE)
    struct.pack_into("<I", data, eapp_header + 16, EAPP_RAW_SIZE)
    sections[".eapp"] = [eapp_header, EAPP_RAW_SIZE, eapp_va, EAPP_RAW_SIZE, eapp_raw]
    end = eapp_raw + EAPP_RAW_SIZE
    if len(data) < end:
        data.extend(bytes(end - len(data)))

    def file_offset(va: int) -> int:
        rva = va - IMAGE_BASE
        for _, _, section_va, section_rawsize, raw_offset in sections.values():
            if section_va <= rva < section_va + section_rawsize:
                return raw_offset + rva - section_va
        raise ValueError(f"VA {va:08x} is not file-backed")

    site_offset = file_offset(SITE_VA)
    if data[site_offset:site_offset + len(SITE_BEFORE)] != SITE_BEFORE:
        raise ValueError("customize packet fingerprint differs; client already patched or unknown")

    thunk_offset = file_offset(THUNK_VA)
    data[thunk_offset:thunk_offset + len(THUNK)] = THUNK
    marker_offset = file_offset(HRC1_VA)
    data[marker_offset:marker_offset + len(HRC1)] = HRC1
    struct.pack_into("<I", data, eapp_header + 8, EAPP_RAW_SIZE)
    relative = THUNK_VA - (SITE_VA + 5)
    data[site_offset:site_offset + 5] = b"\xE9" + struct.pack("<i", relative)

    context = []
    for site, thunk_va, template in CONTEXT_SITES:
        offset = file_offset(site)
        old = data[offset:offset + 5]
        if old[0] != 0xE9:
            raise ValueError(f"context hook fingerprint differs at {site:08x}: {old.hex()}")
        target = site + 5 + struct.unpack_from("<i", old, 1)[0]
        code = template + jump_to(target, thunk_va + len(template))
        code_offset = file_offset(thunk_va)
        data[code_offset:code_offset + len(code)] = code
        data[offset:offset + 5] = jump_to(thunk_va, site)
        context.append({"site": hex(site), "thunk": hex(thunk_va), "old_target": hex(target)})

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    return {
        "source": str(source),
        "output": str(output),
        "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "output_sha256": hashlib.sha256(data).hexdigest(),
        "site": hex(SITE_VA),
        "thunk": hex(THUNK_VA),
        "thunk_size": len(THUNK),
        "eapp_raw_size": hex(EAPP_RAW_SIZE),
        "context": context,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="patched CoA Custom Ascension.exe")
    parser.add_argument("output", type=Path, help="patched copy to write")
    args = parser.parse_args()
    print(json.dumps(patch(args.source, args.output), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

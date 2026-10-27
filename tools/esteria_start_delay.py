"""Delay the Esteria texture worker on a cold client start.

CoA Custom 1.9's EsteriaAppearance.dll creates a texture worker thread before the
D3D9 device and its first texture objects are ready.  On a cold start the worker
sometimes reaches `call dword ptr [eax+0x40]` with an invalid COM pointer
(EsteriaAppearance.dll + 0x1E187) and the client crashes before the login screen.
Running another GPU-heavy program hides the timing race; it is not a fix.

This tool adds a ten-second Sleep to the worker start routine at DLL RVA 0x41EF8.
The Sleep import is resolved through the DLL's own relocated IAT; no absolute
addresses are written.  It is a version-specific workaround for the current
414,720-byte CoA Custom 1.9 DLL.  On another build the fingerprints do not match
and the tool refuses.

The proper source fix is to queue the worker until the renderer signals that the
D3D9 device is ready.  Keep a copy of EsteriaAppearance.dll before running this.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

IMAGE_BASE = 0x10000000
THREAD_START = 0x10041EF8
THREAD_BEFORE = bytes.fromhex("6a0c6828160610")
RESUME = 0x10041EFF
THUNK_VA = 0x10054560
THUNK = bytes.fromhex(
    "e800000000582d654505006810270000ff908850050061"
    "6a0c8d882816060051e979d9feff"
)


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
        sections[name] = (virtual_size, virtual_address, raw_size, raw_offset)

    def file_offset(va: int) -> int:
        rva = va - IMAGE_BASE
        for _, section_va, section_rawsize, raw_offset in sections.values():
            if section_va <= rva < section_va + section_rawsize:
                return raw_offset + rva - section_va
        raise ValueError(f"VA {va:08x} is not file-backed")

    site = file_offset(THREAD_START)
    if data[site:site + len(THREAD_BEFORE)] != THREAD_BEFORE:
        raise ValueError("thread-proc fingerprint differs; already patched or another DLL")

    text = sections.get(".text")
    if not text or not (0x10000000 <= IMAGE_BASE + text[1] <= IMAGE_BASE + text[1] + text[3]):
        raise ValueError("no .text section")
    thunk_offset = file_offset(THUNK_VA)
    if thunk_offset + len(THUNK) > text[3] + text[2]:
        raise ValueError("not enough .text padding for the delay thunk")
    if data[thunk_offset:thunk_offset + len(THUNK)].strip(b"\0"):
        raise ValueError("the .text padding already contains data")

    data[thunk_offset:thunk_offset + len(THUNK)] = THUNK
    relative = THUNK_VA - (THREAD_START + 5)
    data[site:site + 7] = b"\xE9" + struct.pack("<i", relative) + b"\x90\x90"

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    return {
        "source": str(source),
        "output": str(output),
        "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "output_sha256": hashlib.sha256(data).hexdigest(),
        "thread_start": hex(THREAD_START),
        "thunk": hex(THUNK_VA),
        "delay_ms": 10000,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="published CoA Custom 1.9 EsteriaAppearance.dll")
    parser.add_argument("output", type=Path, help="patched DLL to write")
    args = parser.parse_args()
    print(json.dumps(patch(args.source, args.output), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

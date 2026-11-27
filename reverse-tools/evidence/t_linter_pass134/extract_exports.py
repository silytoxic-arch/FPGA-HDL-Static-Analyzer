#!/usr/bin/env python3
"""Read PE export metadata only; never load or execute the target DLL."""
import hashlib
import json
from pathlib import Path
import struct
import sys

data = Path(sys.argv[1]).read_bytes()
digest = hashlib.sha256(data).hexdigest()
if digest != '60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0':
    raise SystemExit('Target digest mismatch')
u16 = lambda offset: struct.unpack_from('<H', data, offset)[0]
u32 = lambda offset: struct.unpack_from('<I', data, offset)[0]
pe = u32(0x3c)
assert data[:2] == b'MZ' and data[pe:pe + 4] == b'PE\0\0'
optional = pe + 24
assert u16(optional) == 0x20b
base = struct.unpack_from('<Q', data, optional + 24)[0]
sections = []
for index in range(u16(pe + 6)):
    offset = optional + u16(pe + 20) + 40 * index
    virtual_size, rva, raw_size, raw = struct.unpack_from('<IIII', data, offset + 8)
    sections.append((rva, max(virtual_size, raw_size), raw))


def file_offset(rva):
    for start, size, raw in sections:
        if start <= rva < start + size:
            return raw + rva - start
    raise ValueError(hex(rva))


directory = file_offset(u32(optional + 112))
_, names_count, functions, names, ordinals = struct.unpack_from('<IIIII', data, directory + 20)
exports = []
for index in range(names_count):
    offset = file_offset(u32(file_offset(names) + 4 * index))
    name = data[offset:data.index(b'\0', offset)].decode('ascii')
    ordinal = u16(file_offset(ordinals) + 2 * index)
    address = base + u32(file_offset(functions) + 4 * ordinal)
    exports.append({'name': name, 'va': hex(address)})
Path(sys.argv[2]).write_text(json.dumps({'targetSha256': digest, 'exports': exports}, indent=2) + '\n')

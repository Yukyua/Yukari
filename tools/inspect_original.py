#!/usr/bin/env python3
"""Offline PE inventory. Reads the selected EXE; never executes it.

Dispatch rows are candidates until confirmed by executable references.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def inspect(path):
    b = path.read_bytes()
    def u16(at): return struct.unpack_from('<H', b, at)[0]
    def u32(at): return struct.unpack_from('<I', b, at)[0]
    pe = u32(0x3c)
    if b[:2] != b'MZ' or b[pe:pe+4] != b'PE\0\0':
        raise ValueError('not a PE executable')
    opt = pe + 24
    if u16(opt) != 0x10b:
        raise ValueError('this inventory supports PE32 only')
    base = u32(opt+28)
    sections = []
    start = opt + u16(pe+20)
    for i in range(u16(pe+6)):
        at = start + i*40
        virtual_size, rva, raw_size, raw_at = struct.unpack_from('<IIII', b, at+8)
        sections.append(dict(name=b[at:at+8].rstrip(b'\0').decode('ascii'),
                             rva=rva, virtual_size=virtual_size, raw_offset=raw_at,
                             raw_size=raw_size, characteristics=u32(at+36)))
    def raw(rva):
        for s in sections:
            if s['rva'] <= rva < s['rva'] + s['raw_size']:
                at = s['raw_offset'] + rva - s['rva']
                if at < len(b): return at
        raise ValueError(f'RVA has no file data: {rva:#x}')
    def text(at):
        end = b.find(b'\0', at, min(at+4096, len(b)))
        if end < 0: raise ValueError('unterminated string')
        return b[at:end].decode('ascii')
    imports = []
    directory = u32(opt+104)
    if directory:
        descriptor = raw(directory)
        while any(b[descriptor:descriptor+20]):
            lookup, _, _, name, iat = struct.unpack_from('<IIIII', b, descriptor)
            dll = text(raw(name))
            thunk = raw(lookup or iat)
            j = 0
            while (entry := u32(thunk+j*4)):
                symbol = f'ordinal:{entry & 0xffff}' if entry & 0x80000000 else text(raw(entry)+2)
                imports.append(dict(dll=dll, symbol=symbol, iat_va=hex(base+iat+j*4)))
                j += 1
            descriptor += 20
    data = next(s for s in sections if s['name'] == '.data')
    def row(at):
        try:
            name, handler, flag = struct.unpack_from('<III', b, at)
            if flag > 1: return None
            code = next((s for s in sections if s['rva'] <= handler-base < s['rva']+s['virtual_size']), None)
            if not code or not code['characteristics'] & 0x20000000: return None
            name_raw = raw(name-base)
            name_text = text(name_raw)
            if not name_text or len(name_text) > 128 or not all(c.isascii() and (c.isalnum() or c in '_=+-*/%<>') for c in name_text): return None
            va = base+data['rva']+at-data['raw_offset']
            return dict(table_va=hex(va), table_file_offset=hex(at), name=name_text,
                        name_va=hex(name), name_file_offset=hex(name_raw), handler_va=hex(handler), flag=flag)
        except (ValueError, UnicodeError, struct.error):
            return None
    # Report contiguous runs, not arbitrary pointer-shaped bytes.
    runs = []
    for alignment in range(0, 12, 4):
        current = []
        for at in range(data['raw_offset']+alignment, data['raw_offset']+data['raw_size']-11, 12):
            item = row(at)
            if item: current.append(item)
            else:
                if len(current) >= 8: runs.append(current)
                current = []
        if len(current) >= 8: runs.append(current)
    return dict(path=str(path.resolve()), sha256=hashlib.sha256(b).hexdigest(), size=len(b),
                pe_offset=hex(pe), machine=hex(u16(pe+4)), image_base=hex(base),
                entry_va=hex(base+u32(opt+16)), timestamp_raw=u32(pe+8),
                sections=sections, imports=imports, dispatch_candidate_runs=runs)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = inspect(args.exe)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n')
    print(f"SHA-256 {result['sha256']}; {len(result['imports'])} imports; dispatch run lengths {[len(r) for r in result['dispatch_candidate_runs']]}")

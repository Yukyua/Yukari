#!/usr/bin/env python3
"""Read-only, fixed-range assembly/constants supplement for scalar contracts.
Only accepts the verified original game.exe SHA-256. Does not inspect patches.
"""
import argparse
import hashlib
from pathlib import Path
import shutil
import struct
import subprocess
ROOT=Path(__file__).resolve().parents[1]
DIGEST='08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267'
RANGES=[(0x433e50,0x433f40),(0x43fa10,0x440440),(0x440440,0x441884),(0x401e10,0x402044),(0x402050,0x4021ff),(0x437570,0x4376e3),(0x43a380,0x43a3ab),(0x43a690,0x43a6e0),(0x43e830,0x43e8f0),(0x42ba49,0x42bb30),(0x42bd66,0x42be07),(0x42c7e0,0x42cae0),(0x42cb20,0x42cc74),(0x42d3b1,0x42d66c),(0x42d726,0x42d764),(0x42dd06,0x42dd1d),(0x432c30,0x432fa0),(0x442970,0x442bc4),(0x438780,0x4387e0),(0x438940,0x438a60),(0x437be0,0x437d30),
        (0x43c2b0,0x43c363),(0x43c3f0,0x43c470),(0x43c600,0x43cbd4),
        (0x429be0,0x429c60),(0x429930,0x4299b0),(0x432600,0x43269d),
        (0x464ca0,0x464d60),(0x465190,0x465210),(0x4652c0,0x465340),(0x4660b0,0x466130),(0x4326f0,0x432790),(0x428fe0,0x429175),(0x447780,0x4477cb),(0x4186b0,0x418793),(0x4188e0,0x418a04),(0x418a60,0x418bf4),(0x418420,0x418443),(0x418320,0x418347),(0x419ea0,0x419ed4),(0x4326a0,0x4326e1)]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original_exe',type=Path)
 p.add_argument('--objdump');p.add_argument('--output',type=Path,default=ROOT/'tools/evidence/scalar-services.asm')
 a=p.parse_args();data=a.original_exe.read_bytes()
 if hashlib.sha256(data).hexdigest()!=DIGEST:raise ValueError('not verified original executable')
 objdump=a.objdump or shutil.which('llvm-objdump') or '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/llvm-objdump'
 chunks=['; Verified original EXE SHA256 '+DIGEST+'\n; Supplement to saved REA decompilation; original x86 instructions are authoritative for FPU constants.\n']
 for start,end in RANGES:
  chunks.append(subprocess.check_output([objdump,'-d',f'--start-address={start:#x}',f'--stop-address={end:#x}',str(a.original_exe)],text=True))
 pe=struct.unpack_from('<I',data,60)[0];n=struct.unpack_from('<H',data,pe+6)[0];o=pe+24+struct.unpack_from('<H',data,pe+20)[0]
 def raw(va,size):
  rva=va-0x400000
  for i in range(n):
   vs,addr,sz,off=struct.unpack_from('<IIII',data,o+i*40+8)
   if addr<=rva and rva+size<=addr+sz:return data[off+rva-addr:off+rva-addr+size]
  raise ValueError('constant outside source section')
 chunks.append('; Floating constants from PE section mappings:\n')
 for va in [0x4a3950,0x4a6598,0x4a34f8,0x4a6590]:
  b=raw(va,8);chunks.append(f'; VA {va:#x} bytes={b.hex()} little-endian-double={struct.unpack("<d",b)[0]!r}\n')
 chunks.append('; Settings globals: .data has RVA 0xbc000, virtual size 0xc3958, raw size 0x10000. The following VAs are in its zero-filled virtual tail before startup/config loaders:\n')
 for va in [0x56c314,0x56c318,0x56c900,0x56c330,0x56c968,0x56c8ea,0x56c8e8,0x56c8e9,0x56c89b,0x56ca11,0x56c8e4,0x56c908,0x56c31c,0x56c31d,0x56c356,0x56c912,0x56c910,0x5587d0,0x5587d1,0x56c440,0x56c32c]:
  rva=va-0x400000
  if not 0xbc000+0x10000 <= rva < 0xbc000+0xc3958:raise ValueError('settings default not in verified zero-filled image tail')
  chunks.append(f'; VA {va:#x}: loader zero-filled image storage\n')
 for va in [0x4bd4d8,0x4a68b4]:chunks.append(f'; Native text fullwidth-space VA {va:#x} bytes='+raw(va,3).hex()+'\n')
 chunks.append('; NumberToString format VA 0x4a667c bytes='+raw(0x4a667c,4).hex()+' (%ld NUL)\n')
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(chunks));print(a.output)
if __name__=='__main__':main()

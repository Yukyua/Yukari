#!/usr/bin/env python3
"""Compile dump-text translation.csv into a lossless UTF-8 translation_patch."""
import argparse
import csv
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('csv',type=Path)
p.add_argument('output',type=Path)
p.add_argument('--encoding',choices=('CP932','CP936'),default='CP936')
a=p.parse_args()
rows=[]
with a.csv.open(encoding='utf-8-sig',newline='') as f:
    for r in csv.DictReader(f):
        if not r['translation']: continue
        script,offset=r['id'].rsplit('#str:',1)
        rows.append('\t'.join((script.encode().hex(),offset,r['source'].encode().hex(),r['translation'].encode().hex())))
a.output.write_text(f'YUKARI_TRANSLATION_PATCH_1\n@encoding\t{a.encoding}\n'+'\n'.join(rows)+'\n',encoding='utf-8')
print(f'{len(rows)} translated literals')

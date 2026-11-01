#!/usr/bin/env python3
"""Read-only audit of original YKC resources; emits metadata, never dialogue.

Usage: python3 tools/audit_resources.py game [--cli build/yukari]
Only data[0-9][0-9].ykc is opened. No executable or patch is inspected.
"""
import argparse
import collections
import json
import mmap
from pathlib import Path
import struct
import subprocess


def words(data, offset, count):
    if offset < 0 or offset + count * 4 > len(data):
        raise ValueError(f"DWORD range out of bounds at {offset:#x}")
    return struct.unpack_from('<' + 'I' * count, data, offset)


def entries(data):
    if data[:8] not in (b'YKC001\0\0', b'YKC002\0\0'):
        raise ValueError('unsupported YKC')
    offset, size = words(data, 16, 2)
    if size % 20 or offset + size > len(data):
        raise ValueError('invalid index range')
    encoding = 'cp932' if data[5] == ord('1') else 'utf-8'
    for pos in range(offset, offset + size, 20):
        name_at, name_size, at, length, reserved = words(data, pos, 5)
        if name_at + name_size > len(data) or at + length > len(data):
            raise ValueError('entry out of bounds')
        raw = data[name_at:name_at + name_size]
        if b'\0' not in raw:
            raise ValueError('unterminated name')
        yield raw.split(b'\0')[0].decode(encoding), at, length, reserved


def script(data):
    if data[:6] != b'YKS001' or data[6] > 1 or data[7] or words(data, 8, 1)[0] != 48:
        raise ValueError('unsupported YKS header')
    ca, cc, ia, ic, da, ds, locals_ = words(data, 16, 7)
    if da + ds > len(data):
        raise ValueError('invalid data range')
    payload = bytes(v ^ (0xaa if data[6] else 0) for v in data[da:da + ds])
    code = words(data, ca, cc)
    index = [words(data, ia + i * 16, 4) for i in range(ic)]
    strings = {}
    for i, (kind, a, b, c) in enumerate(index):
        if kind in (0, 1, 5, 7, 8, 9):
            pos = b if kind == 5 else a
            end = payload.find(b'\0', pos)
            if pos >= len(payload) or end < 0:
                raise ValueError(f'invalid string index {i}')
            strings[i] = payload[pos:end].decode('cp932')
        if kind in (1, 4, 8, 9):
            words(payload, c if kind in (8, 9) else b, 1)
    text_features = collections.Counter()
    for i, (kind, _, _, _) in enumerate(index):
        if kind == 5:
            text = strings[i]
            text_features['string_constants'] += 1
            text_features['strings_with_non_ascii'] += any(ord(c) > 127 for c in text)
            text_features['strings_with_hiragana'] += any('\u3040' <= c <= '\u309f' for c in text)
            text_features['strings_with_katakana'] += any('\u30a0' <= c <= '\u30ff' for c in text)
            text_features['hiragana_characters'] += sum('\u3040' <= c <= '\u309f' for c in text)
            text_features['katakana_characters'] += sum('\u30a0' <= c <= '\u30ff' for c in text)
    calls = collections.Counter()
    call_examples = {}
    top_types = collections.Counter()
    cursor = 0
    while cursor < cc:
        location = cursor
        element = code[cursor]
        cursor += 1
        if element >= ic:
            raise ValueError(f'invalid instruction index at word {location:#x}')
        kind = index[element][0]
        top_types[kind] += 1
        if kind == 0:
            count = code[cursor]
            cursor += 1
            if cursor + count > cc or any(v >= ic for v in code[cursor:cursor + count]):
                raise ValueError(f'invalid arguments at word {location:#x}')
            name = strings[element]
            calls[name] += 1
            call_examples.setdefault(name, location)
            cursor += count
        elif kind == 5:
            raise ValueError(f'unsupported top-level type 5 at word {location:#x}')
    return dict(encrypted=data[6], locals=locals_, code_words=cc,
                elements=ic, types=collections.Counter(v[0] for v in index),
                top_types=top_types, calls=calls, call_examples=call_examples, text_features=text_features)


def graphic(data):
    if len(data) < 64 or data[:8] != b'YKG000\0\0':
        raise ValueError('unsupported YKG header')
    ca, cs, aa, az, fa, fs = words(data, 0x28, 6)
    ca = ca or words(data, 8, 1)[0]
    cs = cs or len(data) - ca
    if ca < 0x30 or ca + cs > len(data) or aa + az > len(data) or fa + fs > len(data) or fs % 32:
        raise ValueError('invalid YKG sections')
    frames = [words(data, fa + i, 8) for i in range(0, fs, 32)]
    return dict(color_magic=data[ca:ca + 8].hex(), alpha_magic=data[aa:aa + 8].hex() if az else '',
                alpha=bool(az), frames=len(frames), header_words=words(data, 8, 8),
                frame_types=collections.Counter(v[5] for v in frames),
                frame_unknown=collections.Counter(v[6] for v in frames),
                frame_actions=collections.Counter(v[7] for v in frames),
                frame_durations=collections.Counter(v[4] for v in frames))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--cli', type=Path, help='also verify every YKS through existing C++ parser')
    args = parser.parse_args()
    report = dict(archives=[], scripts=dict(count=0, encrypted=collections.Counter(),
                  types=collections.Counter(), top_types=collections.Counter(), calls=collections.Counter(),
                  call_examples={}, unknown_types={}, failures=[], cli_failures=[], code_words=0, elements=0,
                  max_locals=0, text_features=collections.Counter()), graphics=dict(count=0, alpha=0, animated=0, frames=0,
                  color_magic=collections.Counter(), alpha_magic=collections.Counter(), header_words=collections.Counter(),
                  frame_types=collections.Counter(), frame_unknown=collections.Counter(),
                  frame_actions=collections.Counter(), frame_durations=collections.Counter(), examples={}, failures=[]),
                  duplicate_names=[], configuration=[])
    seen = {}
    for path in sorted(args.directory.glob('data[0-9][0-9].ykc')):
        with path.open('rb') as file, mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as data:
            archive = dict(name=path.name, size=len(data), signature=data[:8].hex(),
                           index_offset=words(data, 16, 1)[0], count=0, extensions=collections.Counter(),
                           reserved=collections.Counter())
            for name, offset, size, reserved in entries(data):
                identity = f'{path.name}:{name}'
                if name in seen:
                    report['duplicate_names'].append([seen[name], identity])
                seen[name] = identity
                archive['count'] += 1
                archive['extensions'][Path(name).suffix.lower()] += 1
                archive['reserved'][reserved] += 1
                suffix = Path(name).suffix.lower()
                if suffix not in ('.yks', '.ykg', '.ini'):
                    continue
                blob = data[offset:offset + size]
                if suffix == '.ini':
                    # Configuration values only: omit comments and prose.
                    for line in blob.decode('cp932').splitlines():
                        line = line.strip()
                        if line and not line.startswith('//') and '=' in line:
                            report['configuration'].append([identity, line])
                elif suffix == '.yks':
                    total = report['scripts']
                    total['count'] += 1
                    try:
                        result = script(blob)
                        for key in ('types', 'top_types', 'calls', 'text_features'):
                            total[key].update(result[key])
                        for function, word in result['call_examples'].items():
                            total['call_examples'].setdefault(function, dict(entry=identity, archive_offset=offset, word=word))
                        total['encrypted'][result['encrypted']] += 1
                        total['code_words'] += result['code_words']
                        total['elements'] += result['elements']
                        total['max_locals'] = max(total['max_locals'], result['locals'])
                        for kind in result['types']:
                            if kind not in (0, 1, 4, 5, 7, 8, 9, 10):
                                total['unknown_types'].setdefault(kind, identity)
                    except (ValueError, IndexError, UnicodeError) as error:
                        total['failures'].append([identity, offset, str(error)])
                    if args.cli:
                        result = subprocess.run([str(args.cli.resolve()), 'script-entry', str(path), name],
                                                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
                        if result.returncode:
                            total['cli_failures'].append([identity, result.stderr.strip()])
                else:
                    total = report['graphics']
                    total['count'] += 1
                    try:
                        result = graphic(blob)
                        total['alpha'] += result['alpha']
                        total['animated'] += bool(result['frames'])
                        total['frames'] += result['frames']
                        for key in ('color_magic', 'alpha_magic'):
                            total[key][result[key]] += 1
                        total['header_words'][','.join(map(str, result['header_words']))] += 1
                        for key in ('frame_types', 'frame_unknown', 'frame_actions', 'frame_durations'):
                            total[key].update(result[key])
                        example = 'animated' if result['frames'] else 'alpha' if result['alpha'] else 'color_only'
                        total['examples'].setdefault(example, dict(entry=identity, archive_offset=offset,
                                                                  size=size, header=blob[:64].hex()))
                    except (ValueError, IndexError) as error:
                        total['failures'].append([identity, offset, str(error)])
            report['archives'].append(archive)
    print(json.dumps(report, ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()

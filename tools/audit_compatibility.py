#!/usr/bin/env python3
"""Rebuild the original dispatch/source/static-YKS coverage matrix, without dialogue.

Run from repository root:
  python3 tools/audit_compatibility.py --game-dir game
Compiles the small C++ parser helper in a temporary directory. Reads only the five
original archives, original dispatch evidence JSON, and Yukari source. Categories
are source routing facts, never a claim of original semantic equivalence.
"""
import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CATEGORIES = ['explicit_handler', 'registration_only', 'case_skip_or_collect', 'unhandled']
LABELS = dict(zip(CATEGORIES, ['显式处理分支', '仅登记', 'Case仅跳块或收集', '无处理']))
DELEGATES = {
    'ScalarServices': ('services.call(n,', 'src/services.cpp'),
    'SettingsServices': ('settings.call(n,', 'src/settings_services.cpp'),
    'AreaServices': ('areas.call(n,', 'src/area_services.cpp'),
    'PlatformServices': ('platform->call(n,', 'src/platform_services.cpp'),
    'CdCommands': ('cd_commands->handle(call,', 'src/cd_commands.cpp'),
    'AudioCommands': ('audio_commands.handle(call,', 'include/yukari/audio_commands.hpp'),
}
# Registration fallback stores these calls, but concrete consumers read them later.
CONSUMED = {
    'TextSpeedSet': 'StrOut reveal/update reads the stored delay; other StrOut variants do not reveal.',
    'AutoSpeedSet': 'KeyWait reads the stored auto delay.',
    'DefaultKeyinSoundStopSet': 'advance() consults it to stop Flag[270] voice.',
    'SubMenuScript': 'Right mouse event opens the configured script; enable modes are not enforced.',
    'AreaSetMouseLeftScript': 'Text-wait left-click path checks configured rectangles and opens the script.',
}
NOTES = {
    'Index': 'Index text is native save comment2 (header 0x18/1c); complete read-history scheduler equivalence requires separate validation.',
    'GraphicCreateSaveData': 'Preview rendering route exists; complete original typography/layout equivalence remains unverified.',
}
CASE_NOTES = {
    'Case': 'VM skips body outside selection; text Select collects this declaration and dispatches selected body.',
    'MouseCase': 'VM skips body; runtime normalizes MouseCase parameters and collects mouse selection.',
    'MouseCase2': 'VM skips body; runtime collects rectangle click/hover/sound selection.',
    'IndicateCase': 'VM skips body; runtime collects hover visual indication.',
    'MouseKeyCase': 'VM skips body; runtime collects mouse/key declarations.',
    'KeyTableCase': 'VM skips body; runtime collects key declarations; complete original key mapping remains unverified.',
    'WindowMessageCase': 'VM skips body; runtime collects message declaration; Windows message mapping remains partial.',
    'TimeInterrupt': 'VM skips body; active timed selection consumes the timer declaration.',
}

def line_for(source, name):
    for i, line in enumerate(source.splitlines(), 1):
        if '"' + name + '"' in line:
            return i
    return None

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--game-dir', type=Path, required=True)
    p.add_argument('--output', type=Path, default=ROOT/'analysis/compatibility/dispatch-coverage.json')
    p.add_argument('--helper', type=Path, help='prebuilt tools/audit_compatibility.cpp executable')
    p.add_argument('--probe-helper', type=Path, help='prebuilt tools/probe_delegates.cpp executable')
    args = p.parse_args()
    runtime_path, vm_path = ROOT/'src/runtime.cpp', ROOT/'src/vm.cpp'
    runtime, vm = runtime_path.read_text(), vm_path.read_text()
    dispatch_path = ROOT/'analysis/original-engine/dispatch-candidates.json'
    dispatch = json.loads(dispatch_path.read_text())
    contracts_path=ROOT/'tools/compatibility_contracts.json'
    contracts=json.loads(contracts_path.read_text())
    registration = runtime.split('static const std::set<std::string> names={', 1)[1].split('};', 1)[0]
    registered = set(re.findall(r'"([A-Za-z0-9_]+)"', registration))
    host = runtime.split('std::optional<HostResult> host(', 1)[1].split('bool inside(', 1)[0]
    # These are explicit dispatch conditions, not arbitrary string occurrences.
    host_direct = set(re.findall(r'\bn\s*==\s*"([A-Za-z0-9_]+)"', host))
    cases = set(re.findall(r'"([A-Za-z0-9_]+)"', vm.split('bool case_command(', 1)[1].split('return false;', 1)[0]))
    vm_direct = {'=', 'if', 'If', 'ScriptCall', 'ScriptJump', 'ScriptSubCall', 'Return', 'SelectID', 'NowScriptStateGet'}
    with tempfile.TemporaryDirectory(prefix='yukari-compatibility-') as tmp:
        helper = args.helper
        if helper is None:
            helper = Path(tmp)/'audit'
            cmd = ['c++', '-std=c++17', '-I'+str(ROOT/'include'), str(ROOT/'tools/audit_compatibility.cpp'), str(ROOT/'src/formats.cpp'), str(ROOT/'src/resource_source.cpp'), '-o', str(helper)]
            if sys.platform == 'darwin':
                cmd.append('-liconv')
            subprocess.run(cmd, check=True)
        output = subprocess.check_output([str(helper.resolve()), str(args.game_dir.resolve())], text=True)
        names=Path(tmp)/'dispatch-names.txt';names.write_text('\n'.join(e['name'] for e in dispatch)+'\n')
        probe=args.probe_helper
        if probe is None:
            probe=Path(tmp)/'delegate-probe'
            flags=subprocess.check_output(['pkg-config','--cflags','--libs','sdl2','libpng','sndfile'],text=True).split()
            sources=['tools/probe_delegates.cpp','src/formats.cpp','src/resource_source.cpp','src/original_scene.cpp','src/services.cpp','src/settings_services.cpp','src/area_services.cpp','src/platform_services.cpp','src/audio.cpp','src/cd_commands.cpp']
            command=['c++','-std=c++17','-I'+str(ROOT/'include'),*[str(ROOT/s) for s in sources],*flags,'-o',str(probe)]
            if sys.platform=='darwin':command.append('-liconv')
            subprocess.run(command,check=True)
        probe_output=subprocess.check_output([str(probe.resolve()),str(names),tmp],text=True,
            env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SDL_RENDER_DRIVER='software'))
    probes=collections.defaultdict(list)
    for line in probe_output.splitlines():
        fields=line.split('\t')
        if fields[0]!='PROBE':raise ValueError('unexpected delegate probe record')
        _,name,provider,outcome,*message=fields
        probes[name].append(dict(provider=provider,outcome=outcome,message=' '.join(message)))
    scripts, calls = {}, collections.defaultdict(list)
    for line in output.splitlines():
        fields = line.split('\t')
        if fields[0] == 'SCRIPT':
            _, archive, name, at, size, count = fields
            scripts[(archive, name)] = dict(archive=archive, script=name, archive_offset=int(at), size=int(size), instructions=int(count))
        else:
            _, archive, name, fn, count, word = fields
            calls[fn].append(dict(archive=archive, script=name, count=int(count), first_word=int(word)))
    rows=[]
    for entry in dispatch:
        fn=entry['name']
        delegated=[p for p in probes[fn] if DELEGATES[p['provider']][0] in host]
        delegated.sort(key=lambda p:host.index(DELEGATES[p['provider']][0]))
        if fn in cases:
            category='case_skip_or_collect'; mechanism='vm_case'; note=CASE_NOTES.get(fn, 'VM skips following body. No runtime consumer for this Case declaration.')
        elif fn in vm_direct:
            category='explicit_handler'; mechanism='vm_core'; note='Explicit VM core operation; see docs/vm.md and VM tests for supported semantics.'
        elif delegated:
            category='explicit_handler'; mechanism='delegate:'+delegated[0]['provider']; note='Actual delegate API probe established this route; argument/provider errors do not prove semantic support.'
        elif fn in host_direct or fn in CONSUMED:
            category='explicit_handler'; mechanism='host_branch' if fn in host_direct else 'registration_with_consumer'; note=CONSUMED.get(fn, NOTES.get(fn, 'Has an explicit behavior branch; not a claim of complete original semantic equivalence.'))
        elif fn in registered:
            category='registration_only'; mechanism='remember_only'; note='Accepted and saved in configuration; no concrete consumer for this registration was found.'
        else:
            category='unhandled'; mechanism='none'; note='No VM/host handler: invoking this function fails with unimplemented engine function.'
        occurrences=sorted(calls.get(fn,[]), key=lambda x:(x['archive'],x['script']))
        source='src/vm.cpp' if fn in vm_direct or fn in cases else 'src/runtime.cpp'
        if mechanism.startswith('delegate:'):source=DELEGATES[delegated[0]['provider']][1]
        if category in ('unhandled','registration_only') or fn in contracts['unsupported']:
            status='unsupported';basis=contracts['unsupported'].get(fn,note)
        elif fn in contracts['complete']:
            status='complete';basis=contracts['complete'][fn]
        else:
            status='partial';basis=contracts['partial'].get(fn,'A route exists, but original argument/state/consumer equivalence is not fully reviewed or implemented.')
        source_text=vm if source=='src/vm.cpp' else runtime if source=='src/runtime.cpp' else (ROOT/source).read_text()
        rows.append(dict(**entry,category=category,category_label=LABELS[category],mechanism=mechanism,
            semantic_note=note,implementation_status=status,status_basis=basis,delegate_probes=probes[fn],
            source=dict(path=source,line=line_for(source_text,fn)) if category!='unhandled' else None,
            static_calls=sum(x['count'] for x in occurrences),script_count=len(occurrences),scripts=occurrences))
    known={x['name'] for x in dispatch}
    if set(calls)-known:
        raise RuntimeError('script functions missing from original dispatch: '+repr(set(calls)-known))
    summary={cat:dict(label=LABELS[cat],dispatch_entries=sum(r['category']==cat for r in rows),called_functions=sum(r['category']==cat and r['static_calls']>0 for r in rows),static_calls=sum(r['static_calls'] for r in rows if r['category']==cat)) for cat in CATEGORIES}
    status_summary={s:dict(dispatch_entries=sum(r['implementation_status']==s for r in rows),called_functions=sum(r['implementation_status']==s and r['static_calls']>0 for r in rows),static_calls=sum(r['static_calls'] for r in rows if r['implementation_status']==s)) for s in ['complete','partial','unsupported']}
    report=dict(schema_version=2,scope='Original data01.ykc through data05.ykc; type-0 instruction sites in all 571 YKS. Static counts are not runtime execution counts.',
        methodology='C++ Archive/Resources/parse_script inventory; real Scalar/Settings/Area/Platform/Audio delegate API probes under dummy SDL; explicit reviewed contract sets in tools/compatibility_contracts.json. Probe exceptions prove routing only. No implementation percentage is claimed.',
        contract_status_meaning=contracts['meaning'],status_summary=status_summary,
        source_sha256={**{str(path.relative_to(ROOT)):digest(path) for path in [ROOT/'src/formats.cpp',ROOT/'src/resource_source.cpp',ROOT/'src/audio.cpp',ROOT/'src/render.cpp',dispatch_path]}, 'src/runtime.cpp':hashlib.sha256(runtime.encode()).hexdigest(), 'src/vm.cpp':hashlib.sha256(vm.encode()).hexdigest()},
        script_count=len(scripts),unique_called_functions=len(calls),static_call_count=sum(r['static_calls'] for r in rows),
        category_summary=summary,unexpected_source_names=sorted((host_direct|registered|cases|vm_direct)-known),
        scripts=sorted(scripts.values(),key=lambda x:(x['archive'],x['script'])),dispatch=rows)
    for relative in ['src/services.cpp','src/settings_services.cpp','src/area_services.cpp','src/platform_services.cpp','src/audio.cpp','include/yukari/audio_commands.hpp','tools/probe_delegates.cpp','tools/compatibility_contracts.json']:
        report['source_sha256'][relative]=digest(ROOT/relative)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ['script_count','unique_called_functions','static_call_count','category_summary','status_summary','unexpected_source_names']}, ensure_ascii=False,indent=2))
    print('Top unhandled instruction functions:')
    for r in sorted((r for r in rows if r['category']=='unhandled' and r['static_calls']),key=lambda r:-r['static_calls']):
        e=r['scripts'][0]
        print(f"{r['name']}: {r['static_calls']} in {r['script_count']} scripts; {e['script']} word=0x{e['first_word']:x}; native={r['handler_va']}")

if __name__=='__main__':
    main()

#!/usr/bin/env python3
"""Generate a complete original dispatch ledger without claiming unreviewed contracts.
Reads saved original-EXE decompilation evidence only, never game/patch executables.
Run after audit_compatibility.py; rerun after additional original evidence exports.
"""
import json
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
NATIVE={
    '=':'Target previously selected by VM; zero args preserves pending target for next return; integer expression or typed string concatenation/equality; mutates variable/data storage.',
    'if':'One evaluated condition; false skips following linked block. No function result.',
    'If':'Alias of if handler.',
    'ScriptCall':'Script string and optional label, or same-script label. Push fresh local/data context, shared Flag/String banks; child Return does not propagate a value.',
    'ScriptJump':'Script/label destination; replaces current continuation by ending caller after child; same-file label jumps within context.',
    'ScriptSubCall':'Saves current state as savedata.---, invokes ScriptCall, marks new context byte +0xc9 when pushed.',
    'Return':'Optional integer; ends current context. Context pop unlinks/frees without copying child return into parent.',
    'RGB':'One arg reverses RGB/BGR byte order; three args pack r + 256*g + 65536*b into integer result.',
    'Index':'Replaces context-owned index string using optional first argument; no args clears it; returns null.',
    'LF':'Optional text-window id, otherwise current window; calls line-feed helper for found window; null return.',
    'FlagReset':'Clears per-game Flag/String banks, not separate GlobalFlag/GlobalString.',
    'StringLength':'Optional first string; returns encoded source byte length before NUL (not Unicode character count), zero for absent/null. Independent ScalarServices implemented.',
    'NumberToString':'First integer formatted as signed 32-bit decimal %ld; no arguments returns native null. Independent ScalarServices implemented.',
    'sin':'Exactly one integer degree angle: trunc(sin(angle * 3.141592 / 180) * 1000 + 0.4); other argc returns null. Native x86 overflow returns integer indefinite. Independent service.',
    'cos':'Exactly one integer degree angle: trunc(cos(angle * 3.141592 / 180) * 1000 + 0.4); other argc returns null. Independent service.',
    'tan':'Exactly one integer degree angle: trunc(tan(angle * 3.141592 / 180) * 1000 + 0.4); other argc returns null. Independent service.',
    'TimerStart':'Exactly one timer id 0..15: stores uint32 timeGetTime; invalid id/count no-op. Zero start is inactive sentinel. Independent service.',
    'TimerGet':'Exactly one active timer id 0..15: returns uint32 tick subtraction as int32 bits; otherwise zero. Independent service.',
    'TimerWait':'Exactly id,duration; active 0..15 timer waits until unsigned elapsed >= duration, pumping events in at most 100ms sleeps with skip/quit abort. Independent service computes remaining duration; runtime must complete pump/cancel lifecycle.',
    'PlayTime':'Returns saved accumulated seconds plus unsigned tick delta / 1000. Independent service with explicit restorable clock state.',
    'PlayTimeReset':'Resets accumulated seconds and records timeGetTime; null result. Independent service.',
    'FileExistCheck':'Optional first filename pattern: FindFirstFile/FindNextFile search, ignoring directory matches; integer found flag, zero without arg. Independent service requires actual pattern query provider.',
    'FileExistCheckFromYKC':'Optional first archive resource name: calls mounted archive resolver, returns found flag; zero without arg. Independent service requires actual query provider.',
    'WindowSizeXGet':'Integer logical window width from global 0x5570c4; independent service requires window-size provider.',
    'WindowSizeYGet':'Integer logical window height from global 0x5570c0; independent service requires window-size provider.',
    'DebugModeCheck':'Original handler unconditionally returns integer zero; independent service.',
    'ExclusiveCheck':'Compares zero GUID to default nonzero GUID or exactly 11 integer components (DWORD, WORD, WORD, eight BYTEs); returns equality flag. Not an inferred mutex query. Independent service.',
}
NATIVE.update({
    'TextBoxSet':'At least4 args replace native EDIT control; first3 stored at object28/2c/24, fourth at20 and LOGFONT.lfHeight, optional fifth at238. Independent TextModel requires real UI lifecycle hooks; geometry confirmed by419f90 as x/y/width/height; fourth arg also font height, fifth COLORREF confirmed by404340.',
    'TextBoxDelete':'Destroys the current native edit control if present, clears global pointer; independent UI-hook lifecycle.',
    'TextBoxStringGet':'Reads current EDIT text through GetWindowTextLengthA/GetWindowTextA, returns an allocated string including empty case; independent TextModel requires live UI provider.',
    'TextBoxStringSet':'Optional non-null string sets native EDIT text (NUL-terminated; empty clears); no args/null leaves it. Independent UI-hook path.',
    'StrIn':'No args returns uninitialized result. Existing layer concatenates type3 encoded glyph/payload strings, type4 as @o(id), type5 as @g(id); LF7/other sprites ignored. Independent TextModel reconstructs from records.',
    'NameWindowSet':'First arg selects ctx+10 name layer when layer manager exists. With5+ args replaces bounds and resets cursor/anchor plus hanging indent, but does not clear overflow or sprites.',
    'NameOut':'Prepends selected name-layer id and delegates StrOutNWC. Initial instruction centers measured text in remaining line width and synchronizes anchor_x; temporary TextSpeed=0 is restored after output. Independent TextModel provides centered layout, no reveal clock.',
    'RubyFontSet':'Optional first integer writes global56c444; returns current integer font id. Independent typed state; @r(count,text) uses measured ruby height and centered position, then restores main cursor/anchors.',
    'FontSet':'First layer id plus second font id changes existing layer+fc; missing layer no-op. Independent TextModel.',
    'DefaultFontSet':'Only argc4/5/6/7 update selected0x108-byte font record. Even forms height+weight; odd forms height+width+weight. Default cell=height and line=width if nonzero else height; 6/7 forms explicitly override cell/line. Independent TextModel retains cell metrics; native font source preserves LOGFONT family/height/width/weight. GDI captures match full request identity, otherwise FreeType MONO remains an approximate source.',
    'OriginalFontSet':'Loads shared original-font resource through40f9d0->40f570; independent model requires actual resource provider, never empty success.',
    'GraphicFontSet':'Loads shared graphic-font resource through40f9b0->40f570; independent model requires actual resource provider.',
    'OriginalFont':'Appends type4 inline image. Advance=max(cellWidth,imageWidth), wrap lookahead uses same advance; line step=max(lineHeight,imageHeight). Independent layout primitive with actual image metrics.',
    'GraphicFont':'Same cursor/anchor/wrap rule as OriginalFont, appends type5. Independent layout primitive with actual image metrics.',
})
NATIVE.update({
    'DefaultFontEffect':'Script value plus one selects native internal effect 1..5, otherwise plain. Software DIB pipeline preserves original 4x floor downsample, directional border and asymmetric combination shadow. Actual GDI source captures supported; FreeType source is explicitly a substitute.',
})
for name in ['Year','Month','Day','Hour','Minute','Second','Millisecond','Week']:
    NATIVE[name]='Local calendar field via helper 0x429be0 (year+1900, month+1, Sunday=0); zero on clock conversion failure. Independent ScalarServices implemented.'
for name in ['TextSpeed','AutoSpeed','AutoModeCharactorWait']:
    NATIVE[name+'Set']='Exactly one integer writes the native scalar without clamping; other argc no-op. Independent SettingsServices state/query/change-hook path.'
    NATIVE[name+'Get']='Returns the stored integer scalar; arguments ignored. Independent SettingsServices.'
for name in ['AutoMode','SkipMode','AlreadySkipMode']:
    NATIVE[name+'Set']='Forces mode bit to one (not a toggle; arguments ignored), updates associated native mode indicators. Independent SettingsServices exposes bit plus change hook; runtime must consume indicator/audio effects.'
    NATIVE[name+'Reset']='Clears only this mode and updates native indicator state. Independent SettingsServices exposes bit/change hook.'
    NATIVE[name+'Check']='Returns whether the corresponding independent native mode bit equals one. Independent SettingsServices.'
for name in ['SkipModeCancelLKey','AutoModeCancelLKey','AlreadySkipModeCancelLKey']:
    NATIVE[name]='Exactly one arg sets boolean; always returns current bool. Other argument counts query only. Independent SettingsServices.'
for name in ['SaveEnableMode','LoadEnableMode','SubMenuEnableMode']:
    NATIVE[name]='Exactly one argument equal to zero/one updates enable flag; other values/counts leave state intact. SubMenuEnableMode belongs to current script frame. Independent SettingsServices.'
NATIVE.update({
    'ScriptSkipMode':'Exactly one value 0..2 writes current frame +0xc4. Value zero clears skip/already-skip and active state, keeps auto mode. Independent SettingsFrame required.',
    'ScriptSkipReset':'Alias of AllSkipModeReset: clears auto/skip/already-skip plus active state, updates indicators/audio helpers. Independent SettingsServices exposes state/change hook.',
    'AllSkipModeReset':'Clears auto/skip/already-skip plus active state, updates indicators/audio helpers. Independent SettingsServices exposes state/change hook.',
    'ModeExclusive':'Exactly one arg stores boolean exclusive policy but always returns integer zero. Independent SettingsServices.',
    'DefaultKeyinSoundStopSet':'Optional first arg: stores inverse flag; public integer return is arg!=0 or current value. Zero native BSS gives public initial one. Independent SettingsServices.',
    'DefaultKeyinSoundStopGet':'Returns inverse native storage bool, not raw cell. Independent SettingsServices.',
    'AlreadyNoWaitModeSet':'Optional first arg sets context bool, returns stored bool. Independent SettingsServices; context default must be seeded/reviewed.',
    'AlreadyNoWaitModeGet':'Returns context already-no-wait bool. Constructor omits initialization of byte80112; port explicitly initializes disabled for deterministic behavior (not an observed native initial value).',
    'CursorAutoHide':'Optional first integer writes timeout; always returns current timeout. Independent SettingsServices.',
    'SkipScrollSpeed':'Optional positive first integer updates speed; always returns current integer. Independent SettingsServices.',
    'TransitionMode':'Exactly one arg maps <-1/-1/0/>0 to internal -1/0/1/2; sets step16 and renderer mode2/2/1/0. Alias of TransitionModeSet. Independent SettingsServices.',
    'TransitionModeSet':'Exactly one arg maps <-1/-1/0/>0 to internal -1/0/1/2; sets step16 and renderer mode2/2/1/0. Independent SettingsServices.',
    'TransitionModeGet':'Maps stored internal -1/0/1/2 to public -2/-1/0/1. Independent SettingsServices.',
    'SubMenuScript':'No args clears script/restores menu command; first string replaces script, optional second truncated to BYTE. Independent state/query/hook; platform menu lifecycle is runtime-owned.',
    'SaveDataLoadToCallScript':'Optional first string replaces load callback (empty clears); no args keeps current callback. Independent SettingsServices.',
    'SelectTransitionSet':'Zero/one/two args set mode/parameter with defaults0/-1; more args no-op. Independent SettingsServices.',
    'DefaultTextWindowGroup':'Replaces text-window id list, skipping null argument pointers. Independent SettingsServices typed record.',
})
for name in ['SelectFunction','MouseSelectFunction','KeyWaitFunction']:
    NATIVE[name+'Set']='Optional first integer sets callback selector; native null result. Independent SettingsServices.'
    NATIVE[name+'Get']='Optional first integer ALSO sets selector, then returns current integer. Constructor defaults SelectFunction=127, MouseSelectFunction=0, KeyWaitFunction=2047. Independent SettingsServices.'
for name in ['SystemKeySetDecide','SystemKeySetSelectPrev','SystemKeySetSelectNext','SystemKeySetWindowErase','SystemKeySetAutoMode','SystemKeySetHistoryMode','SystemKeySetKeyInSkip','SystemKeySetSkip']:
    NATIVE[name]='Nonempty list clamped to first16 Windows key integers; helper4326f0 clears table and removes zero entries. Empty args keep prior table. Independent SettingsServices exposes normalized table for event consumers.'
for name in ['IconSetAutoMode','IconSetAlreadySkip','IconSetSkip','IconSetAlready']:
    NATIVE[name]='At least five integer args replace five-field mode-indicator record; shorter calls no-op. Independent SettingsServices typed record/change hook; runtime creates/removes indicator sprites.'
for name in ['AreaSetHistoryMode','AreaSetSkip','AreaSetAlreadySkip','AreaSetAutoMode','AreaSetAutoModeExecute','AreaSetWindowErase','AreaSetHistoryNext','AreaSetHistoryPageNext','AreaSetHistoryPrev','AreaSetHistoryPagePrev','AreaSetHistoryLast','AreaSetHistoryHead','AreaSetHistoryExit','AreaSetHistoryMouseR','AreaSetHistoryScrollBarH','AreaSetHistoryScrollBarV']:
    NATIVE[name]='Shared43a970 normalizes seventeen DWORDs; zero args disables while preserving remaining fields. Valid nonzero counts5/11/12/14/15/16; other counts report native error. AreaServices models enabled/visible-parent hit, local-to-parent translation and sprite/sound fields; scrollbar variants adjust extent.'
NATIVE.update({
    'AreaSetSkipCheck':'Returns registered AreaSetSkip enabled flag, not current hover/click state. Independent AreaServices.',
    'AreaSetAlreadySkipCheck':'Returns registered AreaSetAlreadySkip enabled flag. Independent AreaServices.',
    'AreaSetHistoryPageSize':'Optional first integer updates context page count; returns stored value; constructor default2. Independent AreaServices.',
    'AreaSetMouseLeftScript':'First id creates/reuses ordered dynamic record; argc>6 copies script and normalizes remaining args through43a970; shorter args disable mouse region while preserving separate script/key storage. Independent AreaServices.',
    'AreaSetMouseAreaScript':'MouseLeftScript registration plus flags bit0 and native byte90=1. Independent AreaServices.',
    'AreaSetIndicate':'Inserts empty script and fixed -1 placeholders before MouseLeftScript; twelve-arg form inserts placeholder before final sound arg. Independent AreaServices, safe bounded original pointer record.',
    'AreaSetHistoryIndicate':'Indicate wrapper plus area flags bit31. Scheduler excludes these from ordinary key/mouse registration. Independent AreaServices.',
    'AreaSetDelete':'Optional first id deletes matching dynamic area/key/script record, preserving other records order. Independent AreaServices.',
    'KeySetScript':'Alias of AreaSetKeyInScript; first id selects ordered record; argc5/6/7 stores1/2/3 alternative VK keys, script and two scalar fields. Short calls retain separate old key table. Independent AreaServices.',
    'AreaSetKeyInScript':'First id selects ordered record; argc5/6/7 stores1/2/3 alternative VK keys, script and two scalar fields. Native key input OR-matches a nonzero VK and rejects repeat bit30. Independent AreaServices.',
})

NATIVE.update({
 'GraphicRefresh':'No arguments required: invalidates/rebuilds all layer glyph raster state and flushes according to native skip policy. Runtime redraws stored records with current font and inherited global effects/colors, retaining explicit per-sprite overrides and cell coordinates.',
 'GraphicCreateSaveData':'Exactly layer,slot,x,y; returns0 on metadata failure, otherwise replaces generic parent, adds local thumbnail/date/comment sprites, moves/shows parent and returns1. All display settings consumed; see save-preview.md.',
 'SaveDataLoadToCallScript':'Nonempty arg registers load-hook script, empty string clears, no args preserves. Reader pushes a normal frame above restored context; Return continues saved context. Runtime regression covers both native and own saves.',
 'HistoryMode':'Always enters modal history regardless of argument value; native glyph/voice/layout/region consumers are implemented by HistoryView. See history-view.md.',
 'KeyWait':'User-input wait with function-mask auxiliary actions, voice-aware auto timer and accumulated per-round AutoModeCharactorWait delay. See wait-function-gates.md.',
 'KeyVoiceWait':'Voice-aware100ms retries even without Auto; completes once matching nonloop voice ends when Auto is off. Input may finish early. See wait-function-gates.md.',
 'KeyWaitNA':'Calls KeyWait only if current instruction was not already read.',
 'KeyWaitTTE':'Measures KeyWait elapsed wall time; no automatic deadline.',
 'KeyWaitTTELoop':'Measures KeyWait elapsed modulo10000, preserving zero.',
 'SelectTTE':'Measures underlying Select elapsed wall time, without adding timeout or auto-choice.',
 'SelectTTELoop':'Measures Select elapsed modulo10000; zero becomes1.',
 'MouseSelectTTE':'Measures MouseSelect elapsed without adding timeout.',
 'MouseSelectTTELoop':'Measures MouseSelect elapsed modulo10000; zero becomes1.',
 'TTE':'Nonzero elapsed and optional unsigned strict upper bound; argument form clears elapsed on success and skips linked block on failure.',
})
for name in ['StrOut','StrOutNW','StrOutNWC']:
    NATIVE[name]='One target or layer/string pairs; one source glyph/control per target per round, preserving declared order and duplicate targets. Sleep once per full round including final round; NW/NWC suppress delay, NWC centers. Page overflow resumes same round target. See text-schedule.md.'
for name in ['KeyWaitFunctionSet','SelectFunctionSet','MouseSelectFunctionSet']:
    NATIVE[name]='Stores corresponding wait/selector mask; runtime gates history, erase, submenu, registered script areas, auto/skip actions and attached keyboard/wheel bindings. Case declarations stay independent. See wait-function-gates.md.'

def evidence():
    result={}
    for path in sorted((ROOT/'analysis/original-engine').rglob('*.json')):
        try:
            outer=json.loads(path.read_text())
            for block in outer.get('result',{}).get('content',[]):
                if block.get('type')!='text':continue
                obj=json.loads(block['text'])
                proc=obj.get('parameters',{}).get('procedure','')
                body=obj.get('normalized_result','')
                if isinstance(body,str) and '{' in body and re.fullmatch(r'0x[0-9a-fA-F]+',proc):
                    result.setdefault(int(proc,16),[]).append((str(path.relative_to(ROOT)),obj.get('evidence_id','')))
        except (ValueError,TypeError,AttributeError):
            pass
    return result

def main():
    coverage=json.loads((ROOT/'analysis/compatibility/dispatch-coverage.json').read_text())
    refs=evidence()
    rows=[]
    for row in coverage['dispatch']:
        ev=refs.get(int(row['handler_va'],16),[])
        name=row['name']
        if row['category']=='case_skip_or_collect':
            contract='Shared native handler skips following linked block; host selection scheduler must separately interpret declaration args.'
        else:
            contract=NATIVE.get(name,'Body exported; complete argument/result/state contract requires review.' if ev else 'Dispatch entry only; argument/result/state contract not established.')
        reference='<br>'.join('['+Path(p).name+'](../'+p+')' for p,_ in ev[:2]) or 'Pending body export'
        rows.append(f"| `{name}` | `{row['handler_va']}` | {row['flag']} | {row['category']} | {contract} | {reference} |")
    text='''# Complete original instruction contract ledger

The ledger contains all **357** names from the original 12-byte dispatch table at
VA 0x4bc3e0. Dispatch flag is copied verbatim; it is not by itself proof of return
or scheduling semantics. Only original EXE evidence is used. A body export alone
is not a reviewed contract. Unknown contracts remain explicit and must not be
implemented as successful empty returns.

Current routing labels come from the separately fingerprinted coverage audit;
source behavior and original contract are deliberately separate columns. That
snapshot can become stale as runtime work proceeds. See compatibility-audit.md
for static frequencies and concrete implementation limits.

## Contract ledger

| Instruction | Original handler | Dispatch flag | Current source route | Native contract / outstanding work | Body evidence |
|---|---|---:|---|---|---|
'''+ '\n'.join(rows)+'''

## Independent scalar services

`include/yukari/services.hpp` / `src/services.cpp` implement 24 reviewed services.
`ScalarServices::call(name,args)` returns no optional value for an unknown name,
and a typed ServiceResult for handled calls. Monostate is used only for observed
native null results. Missing file/archive/window providers throw rather than
reporting false compatibility. Native text length counts source-encoded bytes;
the default is CP932, with explicit encoding override for imported translated
save contexts. TimerWait returns remaining delay; the runtime must keep pumping
events and apply original skip/quit cancellation. ServiceState exposes the 16
timer slots and accumulated play-clock fields for a runtime snapshot.

The successful original REA exports are under
`analysis/original-engine/instructions/`. Their trigonometric pseudocode
omits FPU arguments; `tools/evidence/scalar-services.asm` records the original
instructions and mapped constants, including the unusual positive 0.4 bias.
Large-angle/libm last-bit equivalence is not Windows execution tested.
`tests/services_tests.cpp` checks encoded text length, signed extrema, negative
trigonometric rounding, unsigned tick wrap, zero sentinel, clock failure,
required platform providers, and GUID component truncation without commercial
assets.

## Movie decoder integration

`include/yukari/movie.hpp` / `src/movie.cpp` provide a local-file streaming
FFmpeg decoder. MoviePlay's native wrapper is synchronous: set playing flag,
enter movie lifecycle, wait/pump, then clear flag and redraw the scene. The
original sample is MPEG-1 video plus MP2 audio, 1024x576, approximately 98s.
Movie::advance(elapsed_ms) presents frames according to normalized source PTS;
frame() exposes RGBA, take_audio() yields timestamped float stereo 48kHz chunks,
and finished() includes delayed decoder frames/resampler tail. The runtime owns
SDL texture/audio queues, playback clock, volume, input/skip handling and scene
restoration. Decoder failure throws; unsupported media never silently succeeds.

## Independent settings services

`include/yukari/settings_services.hpp` / `src/settings_services.cpp` provide 85
reviewed command routes with argument-count rules, boolean normalization, typed
queries, normalized records and state restore. Unknown names return nullopt.
Unlike a raw remember() map, getters return actual typed results; FunctionGet
commands also honor their native optional setter argument. Frame-local script
skip/submenu state is passed explicitly through SettingsFrame. Fullscreen needs
actual platform hooks and throws if unavailable. changed(name,state) lets the
runtime consume speed, mode, icon, callback, history and save-preview settings;
having this independent API does not itself prove every renderer/event consumer
has been integrated. Mode indicator/audio helpers are external effects.

Default values follow PE zero-filled global storage and original context
constructor428fe0/initialize447780. Callback ids start127/0/2047; select transition
starts2/-1; keywait icon is disabled. The inverted keyin-sound getter consequently
starts at one; internal transition mode zero maps to public -1. Constructor leaves
already-no-wait byte80112 unwritten; the port explicitly disables it initially
instead of reproducing allocator garbage. This one initial value is a
deterministic portability choice, not claimed native observation.
SaveDataDisplaySetTo* retains enable/layout/font/date-field semantics and exposes
records; verified standard font defaults9/2/0x303030 and comment font inheritance triples-1 are preserved by partial setters and snapshot migration.
`tests/settings_services_tests.cpp` checks five groups under ASan/UBSan including
set-vs-toggle differences, frame isolation, no-arg reset/query rules, key-table
normalization, layout masks and actual fullscreen hook consumption.

## Independent area services and integration regressions

`area_services.hpp/cpp` normalize 26 area/keyboard command routes. Definitions
preserve the seventeen native DWORDs, dynamic id/script ordering and alternative
VK tables. Query APIs expose actual normalized parent/local rectangle,
normal/hover/pressed sprite frames and three sound ids. Parent lookup is required
for hit testing: a registered enabled region needs an existing visible parent;
native-truncated parent x/y is added to local bounds. Original4186b0,4188e0 and
418a60 use signed greater/less branches, including all four rectangle edges.
Key input418710 OR-matches nonzero alternatives and ignores repeated events when
native key-table flag9c is zero. Native helpers preserve short key registrations'
separate table, and AreaSetDelete removes the associated binding.

`tests/runtime_commands.py --exe build/yukari` generates isolated YKS/YKC/PNG and
optional ffmpeg OGG/MPEG fixtures. Seventeen actual runtime command scenarios cover save/load
while waiting for text, generated movie/audio synchronous return, mouse cancel,
ScriptSubCall checkpoint/return, LF and add/subtract pixel results, text Select,
and measured SelectTTE with TimeInterrupt/TTE branches, native/own load hooks, preview parent lifecycle/snapshot, function masks and round-robin text. Each test hashes its source
files before/after and isolates state in TemporaryDirectory. Commercial samples
are never needed. The complete original MPEG separately decoded2938 video frames
and4705803 stereo sample frames, including decoder/resampler tails.

## Reproduction

`python3 tools/audit_instruction_contracts.py` rebuilds this ledger from saved
evidence and the coverage matrix. Contracts marked pending need original-body
review, not inference from function names. `tools/extract_services_evidence.py`
reproduces fixed-range assembly/constants only when the input EXE SHA-256 matches
the verified original 08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267.
'''
    (ROOT/'docs/instruction-contracts.md').write_text(text)
    print(f'{len(rows)} entries; {sum(int(row["handler_va"],16) in refs for row in coverage["dispatch"])} entries with saved handler bodies')
if __name__=='__main__':main()

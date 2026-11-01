# Original x86 4x downsample oracle

On 2026-10-31 the controlled 32-bit fixture executed the original software downsample instructions from game and compared them with the compiled current `native_glyph_downsample4` implementation. **All 2,108 cases and 2,163,635 BGRA output pixels matched exactly, including the fourth byte.** This is execution of the original x86 arithmetic under CrossOver/Wine, not a native Windows or font-rasterization equivalence claim.

Only new fixture source/report files were added. The original executable, game archives, original CrossOver bottle, production renderer/runtime and CMake were not modified. No dependencies, fonts, registry settings or system components were installed.

## Locked source and isolation

- Original executable SHA-256: `08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267`.
- Original software helper bytes: VA `0x405250..0x405643`, 1,012 bytes. SHA-256: `46e06dfc01a94b37ef99934151e9a5548de3eb25e7406f4749b798794942ada0`.
- Canonical static evidence: `analysis/original-engine/gdi-fonts2/Downsample4x405250.json`; direct original disassembly corroborates the software branch, surface fields and outgoing calls.
- Fixture compiled as PE32/i686, image base `0x10000000`, ASLR disabled. Tested fixture SHA-256: `640ef7e2ad812f168cab50a6175f9cea2b7e3c74988bf5c2c391090be64ef060`.
- CrossOver 26.3.0 / Wine 11, using only the existing APFS-cloned bottle `/private/tmp/yukari-gdi-oracle-bottle`. Original bottle was not selected.

The initial `LoadLibraryExW(DONT_RESOLVE_DLL_REFERENCES)` approach returned a nonpreferred image address; the fixture rejected it before calling original code. A private full-PE allocation at `0x400000` also failed. The occupying allocation was not identified, and neither failure is treated as an arithmetic result. No installation or broad environment repair was attempted.

The successful approach does **not load the game as a module at all**. It opens the SHA-locked file read-only, validates the PE32 section table, extracts only the audited software helper, and copies it into a fresh private page. Three address operands are relocated as described below; all other software-helper bytes are compared against the original source and required to remain identical. The page is then sealed execute/read. No imports, relocations, TLS callbacks, game startup, game entrypoint, DirectDraw objects, or other game services are initialized or requested.

## Complete instrumentation ledger

Offsets below are relative to helper start `0x405250`. The original source is never patched; changes affect only a private in-process copy.

| Instruction / operand | Original bytes | Private-copy replacement |
| --- | --- | --- |
| `4052a8`: compare software/DirectDraw selector; operand at `+0x5a..+0x5d` | Instruction `83 3d 94 d8 4c 00 00`; absolute operand `94 d8 4c 00` = `DAT_004cd894` | Pointer to zero DWORD at private-page `+0x800`; this is immutable after the page becomes RX. Compare opcode/immediate remain original. |
| `405304`: direct allocation call; operand at `+0xb5..+0xb8` | `e8 f8 ef 05 00`, original target `464301`, a jump thunk to `463a71` | Relative displacement to fixture's own cdecl malloc wrapper. Call opcode remains `e8`. |
| `40562f`: direct free call; operand at `+0x3e0..+0x3e3` | `e8 c8 ec 05 00`, original target `4642fc`, a jump thunk to `463919` | Relative displacement to fixture's own cdecl free wrapper. Call opcode remains `e8`. |

No sum, shift, byte load/store, loop or return instruction is changed. The original conditional branch at `4052af` is `0f 85 8f 03 00 00`; its copied target is page offset `0x3f4`, immediately beyond the extracted software body. That address contains `int3`, not copied DirectDraw code or an external function. With the sealed zero selector the branch is not taken. The rest of the private page is also trap-filled except for the immutable selector. An unexpected native exception terminates the fixture with a diagnostic and exit 66 rather than entering a game error dialog.

The software branch's only outgoing calls are the two redirected CRT calls. The wrapper bounds allocations to 1 MiB, requires one outstanding allocation, adds/verifies 16-byte guards around the temporary buffer, and requires exactly one free per call. Input and destination arrays have separate guards; each call also verifies that the source pixels and both surface headers remain unchanged. Surface layout follows actual machine-code use: `+8` pixel pointer, `+24h` **height**, `+28h` **width/stride**, not the opposite.

## Cases and results

Fixed extents, in source pixels: `(0,0)`, `(0,4)`, `(4,0)`, `(4,4)`, `(4,8)`, `(8,4)`, `(12,28)`, `(28,12)`, `(124,128)`, `(128,124)`, `(256,4)`, `(4,256)`. Each runs zero, all-ff, alternating zero/ff, channel ramp and random DWORD patterns. Another 2,048 deterministic xorshift cases choose independent width/height multiples of four from 4 to 256, with random values in all four byte channels. Seed: `0x94d3057b`.

The fixture compiles the production `src/native_glyph_effect.cpp` directly. It calls `native_glyph_downsample4` on the same source pixels and compares every returned DWORD with the original machine-code result; it does not duplicate a second hand-written averaging reference. It checks native return `AL == 1`, allocation/free counts, source/header immutability and all guard regions.

Actual successful stdout, exit 0:

```json
{"schema":1,"original_sha256":"08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267","helper_va":"0x405250","backend":"software","game_entry_requested":false,"load_flags":"PRIVATE_SOFTWARE_KERNEL_NO_LOADER","cases":2108,"compared_bgra_pixels":2163635,"allocations":2108,"frees":2108,"fnv64_words":"1233673d5629298c","status":"PASS"}
```

`fnv64_words` is a deterministic FNV-style fold over compared output DWORDs, not a byte-stream SHA. A negative run supplied the fixture executable itself instead of the original. It exited 1 with `ORACLE_FAIL original EXE SHA256 mismatch; nothing mapped or executed`, before allocating an executable helper page. A final host-side hash of the original remained identical to the locked digest.

## Reproduction

Compile using the installed MinGW32 compiler; no dependency acquisition is needed:

```sh
i686-w64-mingw32-g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -municode \
  -Wl,--image-base,0x10000000 -Wl,--disable-dynamicbase -Iinclude \
  tools/gdi_oracle/native_downsample_oracle.cpp src/native_glyph_effect.cpp \
  -o /private/tmp/yukari-native-downsample-oracle32.exe -ladvapi32
```

The compiler reported only an existing misleading-indentation warning in `font_effect.hpp:52`; the fixture source itself produced no warnings. The report records the tested build above rather than claiming a warning-free production tree.

Run only in the explicitly isolated clone:

```sh
CX_BOTTLE_PATH=/private/tmp WINEDEBUG=-all \
  /Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine \
  --bottle yukari-gdi-oracle-bottle --no-update \
  /private/tmp/yukari-native-downsample-oracle32.exe \
  /path/to/game/game.exe
```

CrossOver converts the two existing host-file arguments to Windows paths. The fixture accepts exactly one original-file argument and no options to broaden the executed function or input bounds. Do not point these commands at a user's normal bottle.

## Limits

This validates the **software** 4x reduction for complete, four-aligned images against original x86 instructions. It does not validate DirectDraw's RGB-mask path, clipping to differently sized destination surfaces, 2x reduction, GDI `TextOut`/font selection/antialiasing, effect dilation/shadow kernels, cell geometry or the entire game engine. The tested raw-row averaging commutes with a complete vertical row reversal; it does not independently establish top-down versus bottom-up GDI storage orientation.

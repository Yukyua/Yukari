# Original default.ini startup configuration

`engine_config.hpp/cpp` parses bytes only; it does not open loose files, create save directories, load cursors or modify runtime. `parse_engine_config(bytes,encoding)` returns typed optionals plus ordered entries with source line/byte offset. Missing optionals retain the caller's established defaults. Unknown keys remain visible with recognized=false, rather than being claimed as supported settings.

Original loader42c7e0 first asks archive resolver42a2f0 for default.ini, then falls back to loose default.ini. Its source artifact is original game.exe SHA25608bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267. Canonical evidence: `instructions2/EngineDefaultConfig42cb00.json`, ev_00edb6f18f228b918c3c64cd156e3d7ee4aa306d2efac47564f40509e5fd4a45. Despite the export filename, the returned procedure is42c7e0.

## Exact singleton mapping

Original machine code42dd06 loads ECX=5587c0 and42dd10 calls42c7e0.42c81f saves this. Thus a loader write this+140a0 directly writes DAT56c860; there is no inferred intermediary copy. Fixed assembly ranges in `tools/evidence/scalar-services.asm` include caller, tokenization and key branches.

| Key | this offset | Absolute target | Observed assignment |
|---|---:|---:|---|
| TransitionWaitType | 140a0 | 56c860 | first strtol value, no normalization |
| FontShadowThickSize | 140d0 | 56c890 | first integer |
| FontBorderThickSize | 140d4 | 56c894 | first integer |
| SaveDataWithoutGraphicTransmittance | 140dc | 56c89c | append each integer to list |
| SaveDataWithoutGraphicLayer | 140ec | 56c8ac | append each integer to list |
| SpriteWithoutGraphicAttribute | 1410c | 56c8cc | append each integer to list |
| SkipScrollSpeed | 14148 | 56c908 | update only when first integer >0 |
| ModeExclusive | 14150 | 56c910 | first integer !=0 |
| NomalCursor | 13e98 | 56c658 | requested path; cursor load success also updates handle/path fields |
| HistorySoundIconPositionType | 140bc | 56c87c | first integer |
| HistorySoundIconPositionSet | 140c0/140c4 | 56c880/56c884 | first2 integers |
| HistoryRowsMargin | 140b4 | 56c874 | first integer; constructor default 30 |
| HistoryLineMargin | 140b8 | 56c878 | first integer |
| WindowSize | 13c78/13c7c | 56c438/56c43c | first2 integers; also updates saved dimension copies |
| WindowSizeVariable | 14174 | 56c934 | first integer !=0 |
| SaveDataIntoSelectPointData | 140a8 | 56c868 | bool; empty resets false; alias SelectPointSaveDataEnable |
| SaveDataIntoHistoryData | 140a9 | 56c869 | bool; empty resets false |
| SaveDataIntoKeyWaitIconNumber | 140d8 | 56c898 | bool; empty resets false |
| SaveDataWithoutScriptData | 140da | 56c89a | bool; empty resets false |
| SaveDataDirectory | 140ac | 56c86c | first path with trailing backslash appended |
| SoundVolumeType | globals | 56db2c and4cdb6e | first integer !=0; empty resets both false |

TransitionWaitType=2 therefore really selects native DAT56c860=2. It must not be reduced to bool or treated as zero. Native transition helpers distinguish values<2 from cumulative timing>=2; Mode2's wrapper selects a different helper whenever this cell is nonzero. See existing TransitionMode1/2 evidence for individual raster timing. Original constructor also proves initial transition_wait_type0, skip_scroll_speed1, mode_exclusive0; parser leaves absence explicit instead of guessing every engine default.

List mapping is established by writes and push helper42c2c0. Whether a list suppresses save writing, load restoration or parent attribute inheritance must follow its actual consumers; a key name/comment alone does not prove that later behavior. Likewise ModeExclusive requires mode-transition consumers, not merely storing a flag.

## Native token grammar

Space, tab and comma are separators everywhere. Any // sequence starts a comment, including inside apparent quoted strings or paths. Quotes and backslashes have no escape/grouping meaning. Keys are case-sensitive; a split key before equals is ignored. Values are tokens after the first equals. Repeated scalar keys overwrite only when their argument-count/value rule accepts them; repeated list keys append, retaining duplicates. Save booleans and SoundVolumeType reset false with no values, while ModeExclusive and most scalars leave existing values unchanged. SaveDataDirectory normalization is pure string behavior here: unlike native, parser never creates that directory.

Integers use original Windows32 strtol(base0): decimal/octal/hex, signed extrema, ignored trailing junk, zero for no digits and saturation on overflow. This is implemented explicitly so host64-bit long or modern binary-prefix extensions do not change results.

The original lexer has1024-byte line and16-token buffers. Parser rejects oversized lines/tokens and truncated source-codepage characters instead of reproducing unsafe overwrites. Embedded NUL ends the file. Comments are discarded before decoding; remaining text uses the caller's explicit encoding (default CP932).

## Verification

`tests/engine_config_tests.cpp` verifies grammar, duplicate lists/scalars, bool aliases and empty resets, cursor/save paths, key case, integer behavior, limits and NUL termination under ASan/UBSan. An explicit optional path accepts the read-only extracted original default.ini; all17 active entries have typed mappings and match original values, including TransitionWaitType2, layer lists11/14 and120, border thickness3, scroll speed3 and exclusive mode1. No game package is written or commercial fixture bundled.

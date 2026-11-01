# Native text layout model

`include/yukari/text_model.hpp` and `src/text_model.cpp` are independent of SDL/runtime/render. The model stores positioned source-codepage glyph and inline-image records, not a text string that is repeatedly laid out. All claims below concern original game.exe static evidence; Windows GDI raster equivalence remains unverified.

## Runtime connection

Create `TextModel(hooks, encoding)` with source encoding CP932 for original scripts. `layer(id)` exposes font selector, TextFlow cursor/anchor/bounds and records; `layers()` enumerates and `erase_layer(id)` removes it. Configure fonts using `call("DefaultFontSet", args)`. Glyph bitmap sizes come from `hooks.glyph_size(font, sourceEncodedGlyph)` or the measure fallback; `hooks.measure` supplies actual proportional whole-string extents. `hooks.dbcs_lead` classifies source-codepage bytes. Never use UTF-8 byte length to identify DBCS glyphs.

Call `output_step(id, sourceEncodedRemaining)` to append one glyph/control; advance by its returned source-byte count. `glyph()` accepts an already classified encoded character and raster dimensions. Each record inherits `layer.style_token`, including ruby records. The token is an opaque renderer-owned color/border/shadow identity. Render type3 at stored x/y/font and type4/5 using actual font-image resources. Type7 is a zero-size LF marker.

`output_step` checks overflow after the output prelude and before the next token. A zero return with flow.overflow means pending page continuation. Runtime must perform native KeyWait/PF, call `clear(id)`, then retry exactly the same remaining encoded input. Pending/prepared state prevents rerunning the prelude and survives snapshot/restore. `clear` removes types3/4/5/7, resets cursor/anchor/indent and clears overflow. `reset_cursor` only resets cursor/anchor/indent and deliberately preserves overflow and records. The original reset412d80 and clear415890 differ here.

`name_out(utf8, initial_instruction)` implements the safe standalone NameOut path: name-window selection, NWC initial centering and glyph/control output with no reveal clock. NameOut446380 prepends context+10 to StrOutNWC446190; that wrapper, on initial frame+44==0, sets x/anchor_x to x+(right-x-measuredWidth)/2 and temporarily forces TextSpeed0. Runtime should preserve/restore its reveal configuration around this path. For resumable NameOut output, measure and center once, then use output_step and runtime-owned continuation; calling name_out again would center again unless initial_instruction=false.

The typed `call` API supports DefaultFontSet, FontSet, NameWindowSet, NameOut, RubyFontSet, StrIn and edit-control operations. Font/image registration requires real resource hooks; unsupported providers throw. Unknown names return nullopt. OriginalFont/GraphicFont placement uses `graphic_font` or source @of/@gf with actual size providers; runtime owns the current target-layer context.

`snapshot()/restore(Bytes)` preserve fonts, all flow fields, records/style tokens, selectors, encoding and registered resource paths. Payloads are bounded to64MiB with FNV integrity detection; restore fully validates a candidate before replacing state. Hooks are retained. Model snapshot v4 additionally stores all five TextBoxSpec fields and current UTF-8 UI text. Restore validates all data first, then rebuilds/deletes the actual edit control through hooks; provider failure leaves model state unchanged and attempts to rebuild the prior UI. Version3 snapshots without an active text box remain compatible; v3 active-box snapshots are explicitly rejected because that format never stored geometry/text. Runtime rebuilds font-image resources using the two path getters. The snapshot stores Cocoa UI values as UTF-8, including values beyond the original source codepage; script-driven TextBoxStringSet still enforces its source encoding. This is a Yukari model snapshot, not a native Yuka save block.

## Observed rules

- Glyph415280 emits at current x/y. Positive fixed cell uses full cell for DBCS and half cell otherwise; proportional uses bitmap width. Anchor updates before lookahead. If x+cell>=right and bounds are not all zero, wrap to left and y+=line. Overflow becomes set when bottom<=y+line.
- LF4166f0 advances and appends marker15420 only when x==anchor_x; a preceding automatic wrap suppresses an extra line advance. It then synchronizes both anchors.
- Inline graphic415490 and original-font415610 records use advance=max(cell,imageWidth), line=max(line,imageHeight), and advance again for wrap lookahead. OriginalFont uses type4, GraphicFont type5.
- `@r(count,text)`4348a0 measures RubyFontSet's font, temporarily shifts y by minus ruby height and x by minus(rubyWidth/2) minus(cell*count/2), emits ruby glyphs through416790, then restores main x/y/anchors. Any overflow produced by ruby placement remains set. Style is inherited. Native uses base0 strtol.
- `@m(count)`434af0 measures upcoming characters and invokes LF if right<=x+extent. It consumes only the control, leaving upcoming text for later calls. Native repeatedly uses the FIRST upcoming byte's DBCS classification for the entire run; this quirk is retained.
- `@gf(id)`434ca0 and `@of(id)`434e10 use nonnegative base0 integer image ids. Negative ids consume without adding a record.
- Fixed string extent405ad0 is cellWidth*sourceEncodedByteCount/2, height=lineHeight. Proportional strings require GDI-equivalent metrics provider. The helper strips @m/@r contents but includes the closing parenthesis in its measured string; this native quirk is retained.
- StrIn434f80 concatenates type3 glyph/payload bytes, type4 as `@o(id)`, type5 as `@g(id)`, ignoring LF7 and other sprites. Thus input control spelling @of/@gf and reconstructed spelling @o/@g differ.
- DefaultFontSet43e540 accepts only4/5/6/7 args. Odd forms specify LOGFONT width in addition to height/weight. Without explicit overrides, cell=height and line=width when nonzero, otherwise height. Last2 args in6/7 forms override cell/line.
- RubyFontSet4351d0 is an optional setter that returns current font id. NameWindowSet436c40 changes selected layer and optional bounds then resets cursor; it does not clear old records.

## Native edit control

TextBoxSet43d7e0 replaces the active EDIT object; Delete435220 destroys it. StringGet435240 reads actual GetWindowTextLengthA/GetWindowTextA. StringSet4352f0 sets a non-null first argument (empty clears); zero args/null leave state. The independent model calls explicit create/delete/get/set hooks, never substitutes a private cached string for a real UI control.

Create4049b0 stores the first3 args in object+28/+2c/+24, fourth in+20 AND LOGFONT.lfHeight, optional fifth in+238. Later evidence419f90 directly confirms object+28/+2c are CreateWindowEx x/y, +24 is width and +20 is height. Thus the fourth script arg is BOTH edit height and LOGFONT.lfHeight. Fifth arg+238 is COLORREF, proved by the parent WM_CTLCOLOREDIT branch and edit404340 painting; it is not a text-length limit.419f50 sends message0x8001; its exact receiver remains unresolved after trampoline419ef0/main410590/base4263b0, but geometry is established independently by the actual constructor. Registration40f9b0/40f9d0 loads actual shared image resources via40f570.

## Punctuation, prefixes and page continuation

`rules()` exposes the source-encoded native configuration: kinsoku_pairs, word_run_pairs, protect_word_runs and ordered hanging_prefixes. Dynamic tables have no guessed defaults. The fixed suppress-after-rollback and indentation-space bytes are both CP9328140, directly observed at original VA4bd4d8 and4a68b4; fixed-range evidence records them.

Before each pending glyph/control,4455f0 rolls back automatic wrap to saved anchors if first DBCS pair is kinsoku and the next pair is not; it clears overflow. With word-run protection enabled, text longer than5 and shorter than255 encoded bytes checks the first3 DBCS pairs: first outside configured set, next2 inside. It measures the first pair plus following configured run and LF-wraps when right<=x+extent.

At the initial top-left cursor a matching ordered prefix stores hanging_indent=(prefixEncodedLength*cellWidth)>>1 using native unsigned multiplication. On later left-edge lines without a matching prefix it emits native fullwidth-space glyphs hanging_indent/cellWidth times. Rollback bypasses both prefix stages. After the token, rollback can consume trailing pairs from the fixed suppression table; the scan retains its prior table index as native does. Pending overflow waits only after these stages; PF clears sprites/cursor but prepared state resumes the original token without repeating them.

These are implemented layout operations with focused tests, rather than registration records. Runtime must supply actual initialized configuration tables and perform wait/history/scene refresh; model alone does not implement the page-history scheduler.

## Remaining boundaries

 Malformed/unterminated controls, truncated codepage glyphs and unsafe native-buffer cases reject rather than copying out of bounds or hanging. The simple argument-list NameOut wrapper exposes a safe single-string profile; original malformed multi-argument pointer behavior is not emulated. Raster pixels still depend on the rendering backend and chosen font. A complete instruction route must wire these layout records, actual resource providers and runtime wait/continuation semantics together.

Sources: successful original REA exports in `analysis/original-engine/instructions/` (TextOutputCore, TextGlyphPlace, LFCore, TextCursorReset, TextClearCore and dispatch handler files), and `instructions2/` (TextRubyCode, TextMotionCode, TextGraphicFontCode, TextOriginalFontCode, TextOriginalGlyph, RubyGlyph, MeasureString, NameWindowSelect, TextBoxCreate, TextBoxNativeCreate, OriginalFontRegister, GraphicFontRegister).

`tests/text_model_tests.cpp` checks punctuation rollback/suppression, hanging indentation, protected-run wrapping, pending overflow snapshot/continuation, glyph/DBCS positions, wrap/LF records, StrIn reconstruction, reset-vs-clear, name centering, ruby cursor/style inheritance, image advances, @m, native measure quirk, UI lifecycle/providers and atomic snapshot corruption rejection. The independent executable passes ASan/UBSan without commercial assets.

## Cocoa TextBox provider

`native_text_box.hpp/mm` provides a real NSTextField child view and active field-editor get/set. `NativeTextBox(window, logicalWidth, logicalHeight, fontFamilyProvider, forwardKey, blockedKey).bind(textHooks)` supplies the four TextModel UI callbacks. The provider must outlive the model and be destroyed on the AppKit main thread before SDL_Window destruction. It follows parent resize, logical letterboxing and flipped view coordinates. Native raw fields in TextBoxSpec map to x/y/width/height; spec.style is the native COLORREF. The edit is transparent/unbordered as original EDIT child style50000000/exstyle20. Native family0 is requested through a callback; unavailable families use system font. Cocoa metrics/highlighting are portability substitutions, not identical GDI pixels.

Original constructor43cdb0 sets classEditWindow vtable4a668c; original table bytes identify+4 virtual handler404340, reached from common trampoline419ef0. That handler forwards keydown/up to parent except Backspace/Space/Delete/keypad0..9/arrows and keys accepted by filter404300. Provider uses a local event monitor only while its own field editor owns focus; callback forwarding can connect that original filter. Without a custom forwarder permitted events become SDL key events. Marked IME text defers engine forwarding until committed; Cocoa handles editing, selection, clipboard and IME. This platform adaptation still needs interactive IME acceptance testing.

`tests/native_text_box_tests.cpp` creates a hidden Cocoa SDL window and verifies real native create/get/set/replacement/delete, snapshot-driven UI reconstruction and exact logical geometry. The actual Cocoa test passed on macOS with WindowServer access; the sandbox lacked displays, so that run used the approved hidden-window execution outside the sandbox. Other SDL drivers explicitly reject native creation and the test checks that rejection. No game assets or files are used. Source compiled cleanly with Wall/Wextra/Wpedantic.

## Runtime integration status

Runtime now uses positioned glyph records and native glyph coordinates/colors on original import, rather than reflowing saved text. Version6 own-runtime state includes model, deduplicated style map and pending/reveal/wait continuation; versions1..5 remain readable. Audio snapshot sources are preloaded using their buffered/streamed type. TextWindowInvisible hides configured visible text groups until matching input restores them. HistoryRecord marks individual glyph/image/LF records to avoid duplicates. Ten synthetic runtime cases (the original seven plus three positioned-text cases), original title startup/hover/restart, and26 CTest cases passed before runtime ownership returned to the parent.

One scheduling boundary remains documented: a multi-layer slow StrOut currently finishes one queued target before the next. Native445ed0 processes one token per target per tick; final fast layout is unaffected, but slow reveal should become round-robin. This has been reported to the runtime owner; the independent model itself remains token-based.

Model v4 regression tests also simulate a user editing the live control without a script setter, injected provider allocation failure/old-UI rollback, corruption rejection before any UI call, UTF-8 snapshot roundtrip and legacy v3 no-box import. These model tests pass ASan/UBSan. Native Cocoa lifecycle tests pass using a real hidden WindowServer window; no claim is made that interactive keyboard/IME behavior has been exhaustively tested.

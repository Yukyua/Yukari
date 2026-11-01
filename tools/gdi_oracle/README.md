# GDI source-plane oracle and renderer integration review

This tool samples Windows GDI through `CreateFontIndirectA`, `GetTextExtentPoint32A` and `TextOutA`. It does not execute the game, inspect a translation executable, install fonts, modify a Wine bottle, or implement an assumed Yuka border algorithm. The only new production-independent source is `gdi_oracle.cpp`.

## What was verified on this Mac

On 2026-10-31 both installed MinGW compilers successfully compiled the fixture without warnings:

```sh
x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -municode -Iinclude tools/gdi_oracle/gdi_oracle.cpp src/gdi_cache.cpp -o /tmp/yukari-gdi-oracle64.exe -lgdi32 -luser32 -ladvapi32
i686-w64-mingw32-g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -municode -Iinclude tools/gdi_oracle/gdi_oracle.cpp src/gdi_cache.cpp -o /tmp/yukari-gdi-oracle32.exe -lgdi32 -luser32 -ladvapi32
```

`wine`/`wine64` are absent from PATH. CrossOver exists at `/Applications/CrossOver.app`; its `Contents/SharedSupport/CrossOver/bin/wine --version` reports CrossOver 26.3.0, build 26.3.0.39832. Its x86_64 Mach-O Wine loader exists. After this inventory, the parent explicitly requested an executable fixture check. The existing3.4GB bottle was APFS-cloned to `/private/tmp/yukari-gdi-oracle-bottle`, and only that isolated copy was used. The sandbox initially rejected wineserver socket binding; the approved rerun successfully executed x64 and x86 self-built fixture processes. This is actual Wine GDI execution, not a native Windows claim. The original bottle was not started or modified. No dependencies or fonts were installed.

The system font directories contain Hiragino Sans and Hiragino Mincho ProN, plus Arial Unicode. These are usable Japanese fallback fonts but are not pixel matches for Microsoft MS Gothic/Mincho. Renderer `default_font_path()` currently picks face index 0 in Hiragino W3.

Read-only name-table inspection of the already installed CrossOver bottle fonts found:

| File under `~/Library/Application Support/CrossOver/Bottles/windows/drive_c/windows/Fonts` | Family name | SHA-256 | Actual name-table copyright |
| --- | --- | --- | --- |
| MSGothic.ttf | MS Gothic | 13a6114201477225f9baa4af6f2eae4ab03626bd24c0358a763cc45f196a2466 | Wada Laboratory, Univ. of Tokyo /efont/, 1990–2003 |
| MSMincho.ttf | MS Mincho | 5118d0078dd64c5efd54e89566ac82a1ae955f15504734c77181f0187a57678c | Wada Laboratory, Univ. of Tokyo /efont/, 1990–2003 |

These are renamed/substitute fonts, not verified Microsoft originals. Their name records report version `Look update time of this file.` and fontversion 0. A successful GetTextFace result of “MS Gothic” therefore does not prove authentic Microsoft outlines. No translated/patch font was inspected or used, and no font file was copied into the project.

## Capture procedure

On Windows, run the compiled executable in a new output folder. On Wine, explicitly select a writable disposable bottle; do not use an unrelated default bottle. Label Wine output as Wine, never as native Windows GDI output. The cache automatically detects Wine through ntdll wine_get_version. Record OS build, rasterizer/Wine version, actual installed font hash, display DPI/font smoothing settings and language/ACP beside each fixture.

```powershell
# ASCII alias for MS Gothic; CP932 text 日本語。ABあい
.\gdi_oracle32.exe --out gothic-27-4x --face-hex 4d5320476f74686963 --text-hex 93fa967b8cea8142414282a082a2 --height 27 --width 0 --weight 200 --scale 4 --quality 0 --charset 128 --margin 8
```

The exact original family from game is fullwidth `ＭＳ ゴシック`; it can be supplied as its CP932 bytes through `--face-hex` instead of the ASCII alias. Inputs are raw bytes, so a shell's Unicode argument conversion cannot corrupt source-encoded glyphs. The tool records ACP and selected face, but does not change machine locale. A different ACP must be recorded and investigated before calling the result an original CP932 reference.

Options expose LOGFONT height, base width, weight, charset, quality, output/clip precision, pitch/family, raw face name and raw text. The font used in the DC is:

```cpp
LOGFONTA lf{};
lf.lfHeight = baseHeight * scale;       // retain sign
lf.lfWidth = (baseWidth * scale) / 2;  // C++ signed integer truncation
lf.lfWeight = weight;
lf.lfCharSet = charset;
lf.lfQuality = quality;
lf.lfOutPrecision = outPrecision;
lf.lfClipPrecision = clipPrecision;
lf.lfPitchAndFamily = pitch;
// copy exact original raw family bytes into lfFaceName
HFONT font = CreateFontIndirectA(&lf);
```

The scaling/width division follows the software `GlyphRaster405cd0` call sites; the original constructor 0x404d20 clears the full 0x108-byte record, sets lfHeight=24 and lfCharSet=128, then initializes scale2. Thus lfQuality=DEFAULT_QUALITY(0), out/clip precision=0 and pitch=0. DefaultFontSet replaces height/width/weight/family. The saved raw assembly is `FontConstructor404d20.asm.txt`. DEFAULT_QUALITY does not guarantee binary monochrome pixels; do not replace it with NONANTIALIASED_QUALITY(3) merely to satisfy a binary kernel input contract. Plain native effect uses scale1, while antialiased/effected branches use scale4 in the inspected engine. Scale2 is exposed to test the existing 2x downsampler branch separately.

A compatible memory DC selects this font and calls `GetTextExtentPoint32A` with the byte length, then `GetTextMetricsW` and `GetTextFaceW`. A top-down 32-bit BI_RGB `CreateDIBSection` is selected into a second memory DC. Text alignment is `TA_LEFT|TA_TOP|TA_NOUPDATECP`, MM_TEXT; the fixture fills the entire DIB background, selects OPAQUE text mode, sets background/text COLORREF and calls `TextOutA`. `GdiFlush` precedes CPU reads. Its outputs are:

- `PREFIX-mask.ppm/.bmp`: black glyph RGB on white background, including the requested margin. This is a source RGB mask, not an inferred alpha channel.
- `PREFIX-ink.ppm/.bmp`: selected text color on selected background, same dimensions and origin.
- `PREFIX.json`: requested LOGFONT, selected font face, ACP, measured extent, TEXTMETRIC and exact source bytes.

GDI's BI_RGB fourth byte is not alpha. PPM preserves authoritative RGB. BMP export sets its fourth byte to 255 only for convenient viewing. No downsampling is applied: high-resolution planes must go through the independently evidenced 0x405250/0x404e50 algorithms before pixel comparison. The extra capture margin is not a native glyph-size/padding claim; remove it using the recorded origin before applying native geometry.

By default the fixture isolates the high-resolution glyph source. The optional complete-scratch mode below also reproduces the native GDI TextOut placements and background modes. It never applies native CPU dilation/combined-shadow passes or downsampling itself. The software GDI branch is sampled; DirectDraw-surface DC behavior remains a separate comparison.

## Read-only integration findings

At review time `src/render.cpp:143..179` caches `Glyph{advance,left,top,width,height,coverage}` by `(face pointer,y_ppem,glyph index)`, loads FT_LOAD_DEFAULT and renders FT_RENDER_MODE_NORMAL. Faces are cached by file path and always opened with TTC index0. It ignores family selection, lfWeight and lfWidth. Existing glyph coverage is reusable across colors, but a native high-resolution source needs its true rasterization identity in the key: file/hash or immutable font identity, TTC face index, signed logical height, width, weight/synthetic transform, italic/escapement if supported, charset/charmap, scale, FT load/render flags and the hinting policy. At minimum include x_ppem or x-scale; y_ppem alone collides when widths change. Keep color/effect composition outside this base coverage cache unless the cache stores decorated planes.

`measure()` uses FT advance with kerning, rounds glyph advances and uses FT line-height. `rasterize()` adds border/shadow inset, performs another whole-string wrap, and chooses baseline from FT ascender. `positioned()` omits wrapping but still shifts the cell origin by FT ascender and uses FT glyph bearings/kerning inside each run. These choices are not GDI `GetTextExtentPoint32A` cell metrics. Preserve native TextModel's already resolved positions, and use a dedicated native cell/glyph raster path; do not feed its glyph records back through generic wrapping or reconstruct their bitmap size from `measure_text`.

`decorate()` first paints one offset shadow, then a symmetric square max-dilation border, then foreground. Native 0x405cd0 uses integer 4x TextOut coordinates, temporary color/mask planes, and directional/repeated stamping; final-resolution max dilation cannot reproduce the same corner coverage or rounding. Keep the generic decorator for fallback/UI text and give native glyphs a distinct effect path. Script effect0 maps to the native default/1x branch; script effects1..4 map to switch cases2..5 because DefaultFontEffect stores argument+1. A raw/native-effect field is necessary to retain effect0 versus effect1 even when both have border_width=0 and shadow_offset=0.

Runtime style serialization (`write_text_style`/`read_text_style`, around lines342–343) currently retains path, size, line/letter spacing, foreground/border/shadow colors, border width and shadow offset. `intern_text_style` compares the same fields. The font table's legacy serialization stores only size/line/spacing. TextModel's own snapshot already stores family, signed height, width, weight, cell width and line height; runtime copies only abs(height), line height and zero letter spacing into renderer styles. A native glyph path should retain the full immutable logical font parameters, raw native effect, border/shadow thickness and selected face identity in TextStyle snapshots, comparison and restored records. Do not abs() away GDI height's sign (positive cell height and negative character height differ). Increment the own-save schema and give older saves explicit fallback defaults.

`draw_text()` currently overwrites a record's interned font size/path/spacing with today's `fonts[rec.font]` when that font index exists. This can destroy a restored/historical style snapshot after a DefaultFontSet redefinition. For immutable native bitmap-equivalent glyph records, preserve their captured font identity/version; only recolor or reflow when the original command actually requests it. New fields must also propagate through current_text_style, SavePreview and History styles, rather than being dropped when colors are copied onto another TextStyle.

## Minimum production changes proposed (not applied here)

1. Add a NativeFont descriptor and native effect descriptor to TextStyle (or a small optional nested struct). Populate it from TextModel TextFont instead of flattening to abs(height). Keep generic UI defaults unchanged.
2. Add one native-cell raster entry to TextRasterizer, shared by measure/glyph-size/positioned rendering. It accepts source glyph/cell origin and returns native bitmap dimensions plus separate RGB/mask source planes. A callback may inject recorded GDI source planes for oracle tests; normal macOS rendering can use an explicitly labelled FreeType source. Do not claim the latter is GDI-identical.
3. Implement the proved 4x effect-plane algorithm on those source planes, then the exact integer native downsampler. Cache base high-resolution glyphs independent of color and cache decorated cells only under a complete effect/color key.
4. Include descriptors in interning and versioned snapshots. Preserve per-record snapshots through draw_text/history/preview, and bind glyph_size separately from logical text extent where the native model distinguishes them.

## Comparison matrix

Use actual original font settings first: height27, width0, weight200, cell28, line32; then height +/-27, explicit widths13/27, weight400/700, 1x/2x/4x, Gothic/Mincho, effects0..4 and border/shadow thickness0/1/2/3. Exercise CP932 single-byte `A`, `W`, punctuation, space; double-byte `日`, `あ`, `、`, `。`, `「`, `」`, and mixed `A日B`. Include strings to check extent versus per-glyph placement and adjacent cells to expose clipping. Test source glyph contours independently from color plane arithmetic and native layout.

Compare in stages: selected face/actual font hash → GDI extent/cell dimensions → raw high-resolution RGB mask → downsampled RGB and mask planes → composed output over black/white/checker backgrounds. Report exact mismatch count, bounds, maximum channel error and per-channel mean error; retain fixtures and manifests. A pixel-equal effect test should inject the same source planes into the native algorithm, avoiding conflation of font outlines and effect arithmetic. Separately assess the portability gap of FT versus native Windows GDI. Repeat-save/restore should reproduce descriptors, glyph dimensions and pixels; cache tests should prove width/weight/face index and effect-mode changes do not reuse incompatible entries.

## Evidence and limits

`analysis/original-engine/instructions2/GlyphRaster405cd0.json` (ev_871b77120319fb1d30da0cd724c86ee8ea120166e0650313e0bf500385ced0fe), `instructions/handler-0x43e540.c.txt` (ev_1a58fe2d8fa4a311371f45a410e9fcb547c4d88af4888a843f4b8e1d218b0c7a), `docs/title-startup-audit.md` and read-only source review support the implementation observations. The decompiler has incorrect recovered HDC/local types in some GPU branches; those pseudocode expressions are not sufficient to claim exact variable identity or output equivalence. The independently exported downsamplers in `gdi-fonts2` and real native-Windows captures remain required for final comparison before a GDI-equivalence claim. No render/runtime/CMake file was changed in this review.

## Runtime-readable dual-plane cache

`include/yukari/gdi_cache.hpp` / `src/gdi_cache.cpp` define `GdiGlyphCache`, `Bytes encode_gdi_cache(const GdiGlyphCache&)` and `GdiGlyphCache decode_gdi_cache(const Bytes&)`. This module has no SDL, FreeType or Wine dependency. `tests/gdi_cache_tests.cpp` covers dual-plane round trip, signed font height, nonzero origin, truncation, invalid stride and undefined alpha; native clang ASan/UBSan passed.

The fixture writes it with `--cache glyph.ygdi --font-file C:\\Windows\\Fonts\\msgothic.ttc`. `--font-file` is read-only: Windows CryptoAPI hashes the specified file with SHA256; it does not load or install it. The caller must ensure this is the font actually selected by GDI, since matching a family name alone cannot establish file identity. GetTextFace is recorded separately. The host field is automatically `wine:<wine_get_version>` when that export exists in ntdll, otherwise `windows-native`; `--host LABEL` appends an OS/build/test label and cannot disguise detected Wine. `--text-utf8` and `--face-utf8` are also supported through explicit UTF8→CP932 conversion without substitution. Raw hex options avoid command-line codepage conversion ambiguity.

`YKGDI002` has an exactly128-byte little-endian header. Bytes0..7 contain that magic; all following slots are 32-bit little endian and offset = 8+slot*4. Signed fields are two's-complement int32.

| Slot | Field |
| --- | --- |
| 0,1,2 | version2, header bytes128, flags3 (top-down mask + actual color) |
| 3,4,5 | signed base height, signed base width, signed weight |
| 6,7,8 | charset, quality, scale1/2/4 |
| 9,10,11 | out precision, clip precision, pitch/family |
| 12,13 | signed measured high-resolution text extent width/height |
| 14,15 | signed bitmap width/height |
| 16,17 | signed glyph origin x/y (capture margin) |
| 18,19 | byte stride = bitmapWidth*4, source ACP |
| 20..24 | byte lengths: requested family UTF8, requested text UTF8, actual family UTF8, host UTF8, lowercase font-file SHA256 ASCII |
| 25 | mask BGRA byte count |
| 26,27 | actual foreground/background COLORREF (R in low byte) |
| 28 | color BGRA byte count |
| 29 | reserved0 |

After the header are the five strings in that order without terminators, then mask BGRA, then actual color BGRA. Both planes have identical dimensions/stride, are top-down and have alpha normalized to255. `bgra` is black-on-white GDI TextOut source; `color_bgra` is the actual requested fg/bg TextOut result. Do not reconstruct the latter from coverage: GDI's RGB rounding and smoothing can differ. For reproducible runtime lookups include family/signed height/base width/weight/charset/quality/scale/precisions/pitch/text/fg/bg and font hash, and enforce the expected host provenance. Every cache contains a single request, which may be a glyph or a whole string; use glyph requests for positioned native glyph records and string requests only for whole-string extent comparisons.

The codec bounds dimensions/payloads, rejects unsupported flags/reserved values, enforces exact byte lengths and alpha255, requires64 lowercase hash digits and actual-family/host labels. It does not attest that the supplied file was selected or that the label proves a Windows version. Native provider code must explicitly label fallback FreeType sources on cache miss. Cache presence does not make the entire rendered glyph GDI-equivalent until native effect-plane composition/downsampling and positioning are also matched.

## Complete native GDI scratch cache (YKGDI003)

Use `--native-effect 1..5 --border N --shadow N --border-color RRGGBB --shadow-color RRGGBB` with `--cache FILE --font-file FONT`. This consumes `FontEffectPlan` directly, sets scale1 for effect1 or scale4 for2..5, creates the plan's complete scratch dimensions, and records origin0 without capture margin. Foreground comes from `--color`; native scratch background comes from foreground (plain/AA), shadow color (shadow), or border color (border/combined). Generic `--background` and `--scale` are overridden by the native plan in this mode.

The exact GDI stage captured is **immediately before native_glyph_finish**: plain/AA color and mask draws at0; Shadow color is OPAQUE fg-on-shadow, mask performs the offset draw followed by origin0 with TRANSPARENT background; Border color is OPAQUE at plan.color origin, mask is OPAQUE at plan.mask origin; BorderShadow uses TRANSPARENT color/mask draws at its two respective plan origins. Whole scratch RGB backgrounds are initialized before drawing. This preserves actual GDI gray/color rounding and overlaps. The later native CPU border dilation, combined shadow and 4x downsampling are intentionally not included, so passing these planes to native_glyph_finish applies each once.

Version3 has a168-byte header and reuses fields0..29 from version2, with version=3, header bytes=168, flags=7 (complete pre-finish native scratch). Extra slots are:

| Slot | Field |
| --- | --- |
| 30 | native effect1..5 |
| 31,32 | border and shadow thickness |
| 33,34 | resolved border/shadow COLORREF |
| 35,36 | final logical bitmap width/height = high scratch dimensions / scale |
| 37..39 | reserved0 |

Strings and dual planes start at byte168, in the same order as version2. The shared codec supports both formats. Version3 measured extent can exceed scratch by up to the native truncation/alignment rule; do not apply the version2 margin-fit assertion. Cache identity must additionally match effect/thickness/colors.

**When creating NativeGlyphPlane DWORDs, discard cache alpha255 and form 0x00RRGGBB.** The original mask predicates compare the whole DWORD against 0x00ffffff and0; feeding 0xffffffff would be incorrect. Alpha255 is a portable container convention, never the original GDI high byte or native opacity.

The Unicode `wmain` entry converts arguments to UTF8 itself; build with `-municode`. Thus `--text-utf8`/`--face-utf8` are actual UTF8 internally even when Windows CRT ACP differs. For the portable BMP/PPM filename streams use an ASCII output prefix; font-file hashing accepts Unicode paths via CreateFileW. Font hashing reads only; font selection still uses original CreateFontIndirectA and must be checked against actual face and supplied file identity.

## Actual executable regression evidence

The self-built fixture ran successfully in the isolated bottle, detecting Wine11.0 and the installed Wada/efont MS Gothic substitute (hash above). Actual charset=128, quality=0, requested weight=200, selected TEXTMETRIC weight=400; ACP1252 was recorded rather than silently treated as932. TextOutA nevertheless selected the font's Japanese charset for CP932 `93fa`; this observation does not replace a Japanese-Windows reference.

| Process/capture | Extent | Scratch | Native finish | Gray mask pixels |
| --- | --- | --- | --- | --- |
| x64 effect4, border2, fg RGB c8f0a0 |108x108|116x116|29x29|449|
| x64 effect5, border2/shadow2, fg c8f0a0/border405060/shadow203040 |108x108|144x144|36x36|449|
| x86 effect3, shadow2, fg c8f0a0/shadow203040 |108x108|116x116|29x29|815|

`fixtures/wine-11-substitute-ms-gothic-u65e5-effect4.ygdi` and its JSON are the actual first Wine process output, not synthetic test pixels. They contain generated glyph raster only, not a copied font file. The paired mask/color cache is decoded and passed through the independently implemented native_glyph_finish with ASan/UBSan; its output dimensions agree with FontEffectPlan. Gray input is present, proving DEFAULT_QUALITY cannot be assumed monochrome even for this local setup.

```sh
clang++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -Iinclude src/gdi_cache.cpp src/native_glyph_effect.cpp tools/gdi_oracle/inspect_cache.cpp -o /tmp/yukari-inspect-gdi-cache
/tmp/yukari-inspect-gdi-cache tools/gdi_oracle/fixtures/wine-11-substitute-ms-gothic-u65e5-effect4.ygdi
```

This validates real process → codec → native plane finishing, not pixel equivalence to Windows, original Microsoft fonts, the entire engine effect output, or text layout. Those require an independently captured reference. A separate raw YKGDI002 process also exercises the Unicode command-line path. Binary fixture format version3 uses168bytes (8-byte magic plus40 DWORD slots); a previous160-byte draft was corrected before runtime integration.

### Odd border3 capture

`fixtures/wine-11-substitute-ms-gothic-u65e5-border3.ygdi` is an additional actual x86 Wine capture of `日`, native effect4, border3/shadow2, base height27/width0/weight200, charset128/quality0, white foreground and black border. The associated JSON retains process output and adds a cache_header object parsed from the archived binary to verify exact colors and request fields. GDI extent108x108 gives scratch120x120 and native_glyph_finish output30x30. The raw mask contains449 gray pixels (2760 nonwhite pixels); the finished30x30 mask contains80 gray pixels. This is the odd-thickness case used by game; it remains a Wine/substitute-font fixture, not a Microsoft-font Windows reference.

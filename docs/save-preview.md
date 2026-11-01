# Original save preview semantics

Evidence: original `Yuka_GraphicCreateSaveData` at 0x438d90 (`instructions` canonical handler), layer writer 0x415c00 (`story/LayerWriter.json`), original executable defaults, and game `OptionSystem` script. This describes the original executable, not the translated executable.

`GraphicCreateSaveData(layer, slot, x, y)` requires exactly four arguments. Its boxed integer result starts at 0. It first reads slot timestamp (0x4331e0) and comments (0x4332b0); failure leaves the old layer untouched and returns 0. On success it deletes the existing destination through 0x416540, creates a generic layer through 0x412a00, adds its child sprites, calls GraphicMove 0x415ba0 with x/y, makes the parent visible through 0x415b80, and returns 1. x/y are parent placement, not dimensions. It temporarily installs the preview font effect and restores the previous global effect before returning.

The thumbnail is a child sprite with kind 6, visible byte 1. Its DIB is decoded from save metadata and scaled only when dimensions differ. There is no full-preview background DIB, background fill, or hardcoded full-preview clip. Date and unbounded comments use ordinary text sprites (0x4157a0); bounded comment2 uses the same glyph-flow helper as native text (0x415280). Child coordinates are local to the parent. Parent movement, visibility, alpha and deletion must affect the entire preview; unrelated overlay IDs do not reproduce this lifecycle. Generic constructor defaults apply; the handler itself only sets parent position and visibility, and does not set parent alpha, image resource, transforms or colors.

## Configuration consumed

The independent `save_preview_layout` model reads these SettingsState keys:

| Key suffix after `SaveDataDisplaySetTo` | Meaning | Original default |
| --- | --- | --- |
| Font | record: font index, raw effect, raw COLORREF | 9, 2, 0x303030 |
| GraphicEnabled / DateEnabled / Comment1Enabled / Comment2Enabled | Original enable bits 1 / 2 / 4 / 8 | all enabled |
| GraphicX / GraphicY / GraphicW / GraphicH | Thumbnail position and dimensions | 5, 5, 80, 60 |
| DateX / DateY | Date local origin | 140, 5 |
| DateFormat | integer printf format, not strftime | `%4.4d/%2.2d/%2.2d %2.2d:%2.2d` |
| DateFields | at most 16 integer field codes; 0 terminates | 1, 3, 4, 12, 13 |
| Comment1X / Comment1Y | Comment1 origin | 140, 25 |
| Comment2X / Comment2Y | Unbounded comment2 origin | 140, 45 |
| Comment1Font / Comment2Font | override font/effect/color | -1, -1, 0xffffffff (inherit) |
| Comment2Rect | left, top, right, bottom | empty (unbounded) |

Date codes: 1=year, 2=year%100, 3=month, 4=day, 11=hour%12 (including 0), 12=hour, 13=minute, 14=second. The source uses FileTimeToSystemTime, so these are UTC components. The model rejects undefined field codes or unsafe noninteger printf conversions instead of reproducing uninitialized reads or buffer corruption.

Comment1 skips `DAT_56c864` source-encoded bytes before decoding. Font override negative values inherit; color 0xffffffff inherits. A nonempty comment2 rectangle replaces the layer text-flow rectangle temporarily, emits glyphs with the native kinsoku handling, and replaces the overflowing tail with CP932 0x8163 (ellipsis). It restores the previous flow rectangle and cursor afterward. The host draw callback must use native TextModel flow rather than draw this text as one unbounded line. Empty strings do not create sprites.

game overrides thumbnail to 126x70 at (9,9), date to (175,8), comment1 to (175,46), comment2 to (175,66), and font index 8/effect 2. Color is supplied by the script. Preview font index must pass through the original font resolver; it is not a pixel font size.

A failed thumbnail read does not make an otherwise valid preview fail: original code still creates an empty thumbnail DIB instance and may draw text. The model omits that unavailable bitmap while retaining successful metadata and text.

## Native save and host canvas

The original 0x415c00 writer emits every occupied sprite's 0x7c header, including kind 6. It only writes sprite payload for a resource-name pointer at +0x48 or animation table at +0x88/+0x8c. The preview's dynamically allocated thumbnail DIB is not serialized as raw pixels. Thus kind 6 is not skipped, but its DIB cannot be reconstructed from a serialized raw bitmap payload. This is consistent with native GraphicCopy: CPU bitmap contents are not serialized by this writer. Do not claim original save preserves a generated thumbnail bitmap.

Yukari's own-save canvas is an implementation choice: flatten child thumbnail/text onto a transparent RGBA layer, keep parent placement and other layer attributes, and preserve bitmap changes through `layer_pixels`. This avoids independent overlays and explicitly provides stronger bitmap persistence in Yukari's own save format. It does not imply that an original save contains such pixel payloads.

`SavePreviewLayout` returns success, parent placement, optional bitmap, and resolved text runs. `draw_save_preview` requires explicit bitmap/text hooks for the returned elements. It never silently ignores a missing renderer hook. Parent must still enforce argc, metadata validity, replacement ordering and integer return value.

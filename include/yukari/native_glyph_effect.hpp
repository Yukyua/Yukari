#pragma once
#include "yukari/font_effect.hpp"
#include <cstdint>
#include <vector>
namespace yukari {
// Top-down rows, DWORDs in Windows DIB byte order: 0x00RRGGBB. The high
// byte is retained because native mask predicates compare the entire DWORD.
struct NativeGlyphPlane {
 int32_t width=0,height=0;
 std::vector<uint32_t> pixels;
};
struct NativeGlyphPlanes {
 NativeGlyphPlane color,transparency; // white transparency=clear, black=opaque
};
uint32_t native_glyph_colorref_to_dib(uint32_t color);
// Exact 405250 software path for an entire 4x scratch image. Dimensions must
// be multiples of four; each of the four byte channels is independently floored.
NativeGlyphPlane native_glyph_downsample4(const NativeGlyphPlane& source);
// Inputs are complete high-resolution scratch planes AFTER GDI TextOut at
// plan.color/mask_glyph_origin_hi. For Shadow, mask must already contain both
// GDI draws. Applies the native software border and combined-shadow passes.
// For Plain, input/output is 1x. This performs no font rasterization.
NativeGlyphPlanes native_glyph_finish(const FontEffectPlan& plan,
 NativeGlyphPlane high_color,NativeGlyphPlane high_mask);
// Exact convenience path for monochrome (0 or 0x00ffffff) GDI glyphs. A gray
// input is rejected: GDI colored antialiasing cannot be inferred bit-exactly
// from its black-on-white rendering. Extents are actual GDI measured extents.
NativeGlyphPlanes native_glyph_effect_binary(const FontEffectPlan& plan,
 const NativeGlyphPlane& glyph_mask,uint32_t foreground_colorref);
// Portable approximation for an antialiased black-on-white source mask.
// Each pixel must be neutral RGB (white=transparent, black=full glyph). It
// retains the original effect geometry/dilation/downsample while approximating
// GDI's colored TextOut coverage from the FreeType grayscale raster.
NativeGlyphPlanes native_glyph_effect_coverage(const FontEffectPlan& plan,
 const NativeGlyphPlane& glyph_mask,uint32_t foreground_colorref);
}

#pragma once
#include "yukari/formats.hpp"
namespace yukari {
// Recorded source raster, not a decorated/downsampled native cell. All strings
// are UTF-8; family/text preserve the request, actual_family records GetTextFace.
struct GdiGlyphCache {
    int32_t height{},width{},weight{};
    uint32_t charset{},quality{},scale{},out_precision{},clip_precision{},pitch{},acp{};
    int32_t extent_width{},extent_height{},bitmap_width{},bitmap_height{},origin_x{},origin_y{};
    std::string family,text,actual_family,host,font_sha256;
    Bytes bgra; // Top-down, tightly packed BGRA; black-on-white, alpha255.
    uint32_t foreground=0xffffff,background{}; // Exact native COLORREF request.
    Bytes color_bgra; // Actual TextOut color plane; never reconstructed from gray.
    // Nonzero selects YKGDI003 complete GDI scratch planes immediately before
    // native_glyph_finish(), including actual overlapping TextOut mask draws.
    int32_t native_effect{},border_thickness{},shadow_thickness{};
    uint32_t border_color{},shadow_color{};
};
Bytes encode_gdi_cache(const GdiGlyphCache&);
GdiGlyphCache decode_gdi_cache(const Bytes&);
}

#pragma once
#include "yukari/native_glyph_effect.hpp"
#include <filesystem>
#include <memory>
#include <string>
namespace yukari {
struct NativeFontSpec {
 std::string family;
 int32_t height=24,width=0,weight=0;
 uint32_t charset=128,quality=0;
 int32_t effect=2,border_thickness=2,shadow_thickness=2;
 uint32_t border_color=0,shadow_color=0;
 bool inherit_effect=true,inherit_border=true,inherit_shadow=true;
 bool operator<(const NativeFontSpec&)const;
 bool operator==(const NativeFontSpec& other)const{return !(*this<other)&&!(other<*this);}
};
struct NativeFontOptions {std::filesystem::path font_directory,gdi_cache_directory;bool require_gdi_capture=false;};
struct NativeFontBitmap {
 NativeGlyphPlanes planes;
 int32_t advance{},cell_height{};
 bool captured=false;
 std::string actual_family,source;
};
class NativeFontRasterizer {
public:
 NativeFontRasterizer();~NativeFontRasterizer();
 void configure(NativeFontOptions);
 std::shared_ptr<const NativeFontBitmap> rasterize(const std::string& utf8,const std::filesystem::path& fallback,const NativeFontSpec&,uint32_t foreground);
 uint64_t cache_hits()const;uint64_t rasterizations()const;
 uint64_t source_cache_hits()const;uint64_t source_rasterizations()const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}

#include "yukari/native_font.hpp"
#include "yukari/gdi_cache.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace yukari;
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
NativeGlyphPlane plane(const Bytes& bytes,int w,int h){NativeGlyphPlane p{w,h,{}};for(size_t i=0;i<bytes.size();i+=4)p.pixels.push_back(uint32_t(bytes[i])|(uint32_t(bytes[i+1])<<8)|(uint32_t(bytes[i+2])<<16));return p;}
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("expected fixture and fallback font paths");
 const auto fixture=decode_gdi_cache(read_file(argv[1]));NativeFontSpec f;f.family=fixture.family;f.height=fixture.height;f.width=fixture.width;f.weight=fixture.weight;f.charset=fixture.charset;f.quality=fixture.quality;f.effect=fixture.native_effect;f.border_thickness=fixture.border_thickness;f.shadow_thickness=fixture.shadow_thickness;f.border_color=fixture.border_color;f.shadow_color=fixture.shadow_color;
 NativeFontRasterizer captured;captured.configure({{},std::filesystem::path(argv[1]).parent_path(),true});
 const auto bitmap=captured.rasterize(fixture.text,"/missing-font",f,fixture.foreground);
 check(bitmap->captured&&bitmap->source.find("wine")!=std::string::npos,"actual GDI source was replaced or mislabeled");
 auto plan=resolve_font_effect({f.effect,2,f.shadow_thickness,f.border_thickness,f.shadow_color,f.border_color,0,0});
 auto expected=native_glyph_finish(plan,plane(fixture.color_bgra,fixture.bitmap_width,fixture.bitmap_height),plane(fixture.bgra,fixture.bitmap_width,fixture.bitmap_height));
 check(bitmap->planes.color.pixels==expected.color.pixels&&bitmap->planes.transparency.pixels==expected.transparency.pixels,"GDI source planes changed during provider lookup");
 auto repeat=captured.rasterize(fixture.text,"/missing-font",f,fixture.foreground);check(repeat==bitmap&&captured.cache_hits()==1,"GDI bitmap cache not reused");
 bool rejected=false;try{captured.rasterize(fixture.text,"/missing-font",f,fixture.foreground^1);}catch(const std::runtime_error&){rejected=true;}check(rejected,"strict GDI mode silently substituted a different foreground");
 rejected=false;try{auto other=f;other.weight++;captured.rasterize(fixture.text,"/missing-font",other,fixture.foreground);}catch(const std::runtime_error&){rejected=true;}check(rejected,"GDI source ignored logical font weight");
 NativeFontRasterizer fallback;NativeFontSpec normal;normal.family="Fallback";normal.height=27;normal.weight=200;normal.effect=2;
 auto plain=fallback.rasterize("日",argv[2],normal,0xffffff);check(!plain->captured&&plain->planes.color.width>0&&plain->cell_height==27,"fallback source provenance or positive LOGFONT height lost");
 normal.effect=4;normal.border_thickness=3;auto border=fallback.rasterize("日",argv[2],normal,0xffffff);check(border->planes.color.width>=plain->planes.color.width+3&&border->planes.color.height==plain->planes.color.height+3,"odd native border extent rounded to integer radius");
 normal.effect=5;auto combined=fallback.rasterize("日",argv[2],normal,0xffffff);check(combined->planes.color.width%4==0&&combined->planes.color.height%4==0,"combined glyph canvas lost native 4px alignment");
 normal.effect=2;normal.width=20;auto narrow=fallback.rasterize("日",argv[2],normal,0xffffff);normal.width=40;auto wide=fallback.rasterize("日",argv[2],normal,0xffffff);check(wide->advance>narrow->advance,"LOGFONT width ignored or reused from stale glyph cache");
 normal.width=0;normal.height=-27;auto negative=fallback.rasterize("日",argv[2],normal,0xffffff);check(negative->cell_height>=27,"negative LOGFONT em height treated as positive cell height");
 check(fallback.source_rasterizations()==4&&fallback.source_cache_hits()==2,"font effects rerasterized identical monochrome source");
 check(fallback.rasterizations()==6,"native source cache identity lost a font/effect field");
 auto alias=normal;alias.family="AliasOfSameFallback";
 auto alias_bitmap=fallback.rasterize("日",argv[2],alias,0xffffff);
 check(alias_bitmap==negative,"same actual fallback face and effect did not share final glyph");
 // Compare shared ink against fresh rasterizers across colors/effects and
 // different requested family names selecting the same fallback face.
 normal.height=27;normal.width=0;
 for(int effect:{2,4,5})for(uint32_t color:{0x182739U,0xf1e2d3U}){
  normal.effect=effect;normal.family="AnotherFallbackAlias";NativeFontRasterizer fresh;
  auto reused=fallback.rasterize("日",argv[2],normal,color),reference=fresh.rasterize("日",argv[2],normal,color);
  check(reused->advance==reference->advance&&reused->cell_height==reference->cell_height&&reused->planes.color.pixels==reference->planes.color.pixels&&reused->planes.transparency.pixels==reference->planes.transparency.pixels,"shared native ink changed effects or geometry");
 }
 std::cout<<"Real GDI dual-plane capture, strict source matching, odd border, combined canvas, logical font sizes and cache identity passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

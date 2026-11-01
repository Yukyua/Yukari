#include "yukari/gdi_cache.hpp"
#include "yukari/native_glyph_effect.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
int main(int argc,char** argv){try{
 for(int a=1;a<argc;++a){std::ifstream in(argv[a],std::ios::binary);if(!in)throw std::runtime_error("cannot open cache");yukari::Bytes bytes(std::istreambuf_iterator<char>(in),{});auto c=yukari::decode_gdi_cache(bytes);
  auto plane=[&](const yukari::Bytes& p){yukari::NativeGlyphPlane out{c.bitmap_width,c.bitmap_height,{}};for(size_t i=0;i<p.size();i+=4)out.pixels.push_back(uint32_t(p[i])|(uint32_t(p[i+1])<<8)|(uint32_t(p[i+2])<<16));return out;};
  auto color=plane(c.color_bgra),mask=plane(c.bgra);size_t gray=0,ink=0;for(auto p:mask.pixels){if(p!=0&&p!=0xffffff)++gray;if(p!=0xffffff)++ink;}
  std::cout<<argv[a]<<" host="<<c.host<<" face="<<c.actual_family<<" hash="<<c.font_sha256<<" effect="<<c.native_effect<<" extent="<<c.extent_width<<'x'<<c.extent_height<<" scratch="<<c.bitmap_width<<'x'<<c.bitmap_height<<" mask_ink="<<ink<<" gray="<<gray;
  if(c.native_effect){yukari::FontEffectInput i;i.native_effect=c.native_effect;i.border_thickness=c.border_thickness;i.shadow_thickness=c.shadow_thickness;i.border_color=c.border_color;i.shadow_color=c.shadow_color;auto p=yukari::resolve_font_effect(i);auto done=yukari::native_glyph_finish(p,std::move(color),std::move(mask));auto wanted=p.canvas(c.extent_width,c.extent_height);if(done.color.width!=wanted.first||done.color.height!=wanted.second)throw std::runtime_error("native finish dimensions disagree with plan");size_t finished_gray=0;for(auto pixel:done.transparency.pixels)if(pixel!=0&&pixel!=0xffffff)++finished_gray;std::cout<<" finish="<<done.color.width<<'x'<<done.color.height<<" finish_gray="<<finished_gray;}
  std::cout<<'\n';
 }return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

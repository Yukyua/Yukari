#include "yukari/native_glyph_effect.hpp"
#include <limits>
#include <stdexcept>
#include <utility>
namespace yukari {
namespace {
size_t area(int32_t w,int32_t h){
 if(w<0||h<0||uint64_t(w)*uint64_t(h)>std::numeric_limits<size_t>::max()/sizeof(uint32_t))throw std::runtime_error("invalid native glyph plane extent");
 return size_t(w)*size_t(h);
}
void validate(const NativeGlyphPlane& p){if(p.pixels.size()!=area(p.width,p.height))throw std::runtime_error("native glyph plane size mismatch");}
NativeGlyphPlane filled(int32_t w,int32_t h,uint32_t color){return {w,h,std::vector<uint32_t>(area(w,h),color)};}
uint32_t& raw(NativeGlyphPlane& p,int32_t x,int32_t y){return p.pixels[size_t(p.height-1-y)*p.width+x];}
void dilate(NativeGlyphPlane& mask,int32_t b){
 if(b<0||b>mask.width||b>mask.height)throw std::runtime_error("native glyph border exceeds scratch plane");
 // 406a.. and 4070.. operate in-place, forward in bottom-up DIB order.
 for(int32_t y=0;y<mask.height;++y)for(int32_t x=0;x<mask.width-b;++x)
  if(raw(mask,x+b,y)!=0x00ffffff)for(int32_t j=0;j<b;++j)raw(mask,x+j,y)=0;
 for(int32_t y=0;y<mask.height-b;++y)for(int32_t x=0;x<mask.width;++x)
  if(raw(mask,x,y+b)!=0x00ffffff)for(int32_t j=0;j<b;++j)raw(mask,x,y+j)=0;
}
void combined_shadow(NativeGlyphPlane& color,NativeGlyphPlane& mask,int32_t s,uint32_t shadow){
 if(s<0)throw std::runtime_error("negative native glyph shadow offset");
 // Native loop may access outside its DIB for s=0. Bound the derived source
 // and destination coordinates rather than reproducing memory corruption.
 for(int32_t i=mask.height-1;i>=s;--i)for(int32_t x=mask.width-1;x>=s;--x){
  const int64_t sy=int64_t(mask.height)-1+s-i,dy=int64_t(mask.height)-i+1;
  if(sy<0||sy>=mask.height||dy<0||dy>=mask.height)continue;
  if(raw(mask,x-s,int32_t(sy))!=0x00ffffff){
   if(raw(mask,x,int32_t(dy))!=0)raw(color,x,int32_t(dy))=shadow;
   raw(mask,x,int32_t(dy))=0;
  }
 }
}
void draw_binary(NativeGlyphPlane& dst,const NativeGlyphPlane& glyph,int32_t ox,int32_t oy,uint32_t color){
 for(int32_t y=0;y<glyph.height;++y)for(int32_t x=0;x<glyph.width;++x){
  const int64_t dx=int64_t(x)+ox,dy=int64_t(y)+oy;
  if(dx>=0&&dy>=0&&dx<dst.width&&dy<dst.height&&glyph.pixels[size_t(y)*glyph.width+x]==0)
   dst.pixels[size_t(dy)*dst.width+size_t(dx)]=color;
 }
}
uint32_t blend(uint32_t background,uint32_t foreground,uint32_t transparency){
 uint32_t out=0;for(unsigned shift:{0U,8U,16U}){
  const auto b=(background>>shift)&255,f=(foreground>>shift)&255;
  out|=((f*(255-transparency)+b*transparency+127)/255)<<shift;
 }return out;
}
void draw_coverage_color(NativeGlyphPlane& dst,const NativeGlyphPlane& glyph,int32_t ox,int32_t oy,uint32_t color){
 for(int32_t y=0;y<glyph.height;++y)for(int32_t x=0;x<glyph.width;++x){
  const int64_t dx=int64_t(x)+ox,dy=int64_t(y)+oy;if(dx<0||dy<0||dx>=dst.width||dy>=dst.height)continue;
  const auto transparency=glyph.pixels[size_t(y)*glyph.width+x]&255;
  if(transparency<255)dst.pixels[size_t(dy)*dst.width+size_t(dx)]=blend(dst.pixels[size_t(dy)*dst.width+size_t(dx)],color,transparency);
 }
}
void draw_coverage_mask(NativeGlyphPlane& dst,const NativeGlyphPlane& glyph,int32_t ox,int32_t oy){
 for(int32_t y=0;y<glyph.height;++y)for(int32_t x=0;x<glyph.width;++x){
  const int64_t dx=int64_t(x)+ox,dy=int64_t(y)+oy;if(dx<0||dy<0||dx>=dst.width||dy>=dst.height)continue;
  const auto transparency=glyph.pixels[size_t(y)*glyph.width+x]&255;if(transparency==255)continue;
  auto& target=dst.pixels[size_t(dy)*dst.width+size_t(dx)];const auto old=target&255;
  const auto value=(old*transparency+127)/255;target=value|(value<<8)|(value<<16);
 }
}
}
uint32_t native_glyph_colorref_to_dib(uint32_t c){return (c&0x0000ff00)|((c&0xff)<<16)|((c>>16)&0xff);}
NativeGlyphPlane native_glyph_downsample4(const NativeGlyphPlane& src){
 validate(src);if(src.width%4||src.height%4)throw std::runtime_error("native glyph 4x scratch dimensions not aligned");
 auto dst=filled(src.width/4,src.height/4,0);
 for(int32_t y=0;y<dst.height;++y)for(int32_t x=0;x<dst.width;++x){
  // Sum two byte channels in independent 16-bit lanes. Sixteen 8-bit
  // samples fit each lane, preserving the native floor division exactly.
  uint32_t rb=0,ga=0;
  for(int32_t dy=0;dy<4;++dy){
   const auto* row=src.pixels.data()+size_t(y*4+dy)*src.width+x*4;
   for(int32_t dx=0;dx<4;++dx){const auto p=row[dx];rb+=p&0x00ff00ff;ga+=(p>>8)&0x00ff00ff;}
  }
  dst.pixels[size_t(y)*dst.width+x]=((rb>>4)&0x00ff00ff)|((ga<<4)&0xff00ff00);
 }
 return dst;
}
NativeGlyphPlanes native_glyph_finish(const FontEffectPlan& plan,NativeGlyphPlane color,NativeGlyphPlane mask){
 validate(color);validate(mask);
 if(color.width!=mask.width||color.height!=mask.height)throw std::runtime_error("native glyph color/mask dimensions differ");
 if(plan.kind==FontEffectPlan::Border||plan.kind==FontEffectPlan::BorderShadow)dilate(mask,plan.border_dilate_left_hi);
 if(plan.kind==FontEffectPlan::BorderShadow){
  if(plan.shadow_thickness<0||plan.shadow_thickness>INT32_MAX/4)throw std::runtime_error("invalid native glyph shadow offset");
  combined_shadow(color,mask,plan.shadow_thickness*4,native_glyph_colorref_to_dib(plan.shadow_color)|(plan.shadow_color&0xff000000));
 }
 if(plan.kind==FontEffectPlan::Plain)return {std::move(color),std::move(mask)};
 return {native_glyph_downsample4(color),native_glyph_downsample4(mask)};
}
NativeGlyphPlanes native_glyph_effect_binary(const FontEffectPlan& plan,const NativeGlyphPlane& glyph,uint32_t foreground){
 validate(glyph);for(auto pixel:glyph.pixels)if(pixel!=0&&pixel!=0x00ffffff)throw std::runtime_error("exact native glyph convenience path requires monochrome GDI mask");
 const auto output=plan.canvas(glyph.width,glyph.height);
 const auto scale=plan.kind==FontEffectPlan::Plain?1:4;
 if(output.first>INT32_MAX/scale||output.second>INT32_MAX/scale)throw std::runtime_error("native glyph scratch extent overflow");
 const auto fg=native_glyph_colorref_to_dib(foreground);
 uint32_t background=fg;
 if(plan.kind==FontEffectPlan::Shadow)background=native_glyph_colorref_to_dib(plan.shadow_color);
 if(plan.kind==FontEffectPlan::Border||plan.kind==FontEffectPlan::BorderShadow)background=native_glyph_colorref_to_dib(plan.border_color);
 auto color=filled(output.first*scale,output.second*scale,background);
 auto mask=filled(color.width,color.height,0x00ffffff);
 draw_binary(color,glyph,plan.color_glyph_origin_hi.first,plan.color_glyph_origin_hi.second,fg);
 draw_binary(mask,glyph,plan.mask_glyph_origin_hi.first,plan.mask_glyph_origin_hi.second,0);
 if(plan.kind==FontEffectPlan::Shadow)draw_binary(mask,glyph,plan.shadow_mask_translation_hi.first,plan.shadow_mask_translation_hi.second,0);
 return native_glyph_finish(plan,std::move(color),std::move(mask));
}
NativeGlyphPlanes native_glyph_effect_coverage(const FontEffectPlan& plan,const NativeGlyphPlane& glyph,uint32_t foreground){
 validate(glyph);for(auto pixel:glyph.pixels)if((pixel&0xff000000)||((pixel&255)!=((pixel>>8)&255))||((pixel&255)!=((pixel>>16)&255)))throw std::runtime_error("antialiased glyph mask must contain neutral RGB coverage");
 const auto output=plan.canvas(glyph.width,glyph.height);const auto scale=plan.kind==FontEffectPlan::Plain?1:4;
 if(output.first>INT32_MAX/scale||output.second>INT32_MAX/scale)throw std::runtime_error("native glyph scratch extent overflow");
 const auto fg=native_glyph_colorref_to_dib(foreground);uint32_t background=fg;
 if(plan.kind==FontEffectPlan::Shadow)background=native_glyph_colorref_to_dib(plan.shadow_color);
 if(plan.kind==FontEffectPlan::Border||plan.kind==FontEffectPlan::BorderShadow)background=native_glyph_colorref_to_dib(plan.border_color);
 auto color=filled(output.first*scale,output.second*scale,background);auto mask=filled(color.width,color.height,0x00ffffff);
 draw_coverage_color(color,glyph,plan.color_glyph_origin_hi.first,plan.color_glyph_origin_hi.second,fg);
 draw_coverage_mask(mask,glyph,plan.mask_glyph_origin_hi.first,plan.mask_glyph_origin_hi.second);
 if(plan.kind==FontEffectPlan::Shadow)draw_coverage_mask(mask,glyph,plan.shadow_mask_translation_hi.first,plan.shadow_mask_translation_hi.second);
 return native_glyph_finish(plan,std::move(color),std::move(mask));
}
}

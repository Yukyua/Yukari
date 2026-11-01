#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <utility>
namespace yukari {
inline int32_t font_effect_from_script(int32_t effect){uint32_t raw=uint32_t(effect)+1;int32_t result;std::memcpy(&result,&raw,4);return result;}
struct FontEffectInput {
 int32_t native_effect=0,default_native_effect=2;
 int32_t shadow_thickness=2,border_thickness=2;
 uint32_t shadow_color=0xffffffff,border_color=0xffffffff;
 uint32_t default_shadow_color=0,default_border_color=0;
};
struct FontEffectPlan {
 enum Kind { Plain,Antialiased,Shadow,Border,BorderShadow } kind=Plain;
 int32_t native_effect=1,supersample=1;
 uint32_t shadow_color=0,border_color=0;
 int32_t shadow_thickness=0,border_thickness=0;
 // Top-down GDI coordinates, in supersampled pixels. Border mask uses a
 // rectangular dilation leftward and downward from mask_glyph_origin_hi.
 std::pair<int32_t,int32_t> color_glyph_origin_hi{},mask_glyph_origin_hi{};
 int32_t border_dilate_left_hi=0,border_dilate_down_hi=0;
 std::pair<int32_t,int32_t> shadow_mask_translation_hi{};
 // Input is actual GetTextExtentPoint32A at the selected supersampling scale.
 std::pair<int32_t,int32_t> canvas(int32_t measured_width,int32_t measured_height)const{
  if(measured_width<0||measured_height<0)throw std::runtime_error("negative glyph extent");
  auto dimension=[&](int32_t n){int64_t size=n;if(kind==Antialiased)size/=4;else if(kind==Shadow)size=size/4+shadow_thickness;else if(kind==Border)size=(size+3)/4+border_thickness;else if(kind==BorderShadow){size=size/4+int64_t(border_thickness)*4;size=(size+3)/4*4;}if(size<0||size>INT32_MAX)throw std::runtime_error("glyph effect canvas overflow");return int32_t(size);};return {dimension(measured_width),dimension(measured_height)};
 }
};
inline FontEffectPlan resolve_font_effect(const FontEffectInput& in){
 FontEffectPlan plan;plan.native_effect=in.native_effect<1?in.default_native_effect:in.native_effect;
 plan.shadow_color=in.shadow_color==0xffffffff?in.default_shadow_color:in.shadow_color;
 plan.border_color=in.border_color==0xffffffff?in.default_border_color:in.border_color;
 const auto effect=plan.native_effect;
 if(effect==2)plan.kind=FontEffectPlan::Antialiased;
 else if(effect==3)plan.kind=FontEffectPlan::Shadow;
 else if(effect==4)plan.kind=FontEffectPlan::Border;
 else if(effect==5)plan.kind=FontEffectPlan::BorderShadow;
 if(plan.kind==FontEffectPlan::Plain)return plan;
 plan.supersample=4;
 auto scale=[](int32_t n){if(n<0||n>INT32_MAX/12)throw std::runtime_error("invalid native glyph effect thickness");return n*4;};
 if(effect==3||effect==5){const auto s=scale(in.shadow_thickness);plan.shadow_thickness=in.shadow_thickness;plan.shadow_mask_translation_hi={s,effect==5?s-2:s};}
 if(effect==4||effect==5){const auto b=scale(in.border_thickness);plan.border_thickness=in.border_thickness;plan.border_dilate_left_hi=plan.border_dilate_down_hi=b;
  if(effect==4){plan.color_glyph_origin_hi={b/2,b/2};plan.mask_glyph_origin_hi={b,0};}
  else{const int32_t t=(in.border_thickness/2)*4;plan.color_glyph_origin_hi={t*2,t*3};plan.mask_glyph_origin_hi={t*3,t*2};}
 }
 return plan;
}
// Sprite+50: any nonzero high byte means palette indirection, not only 0xff.
inline uint32_t resolve_glyph_foreground(uint32_t color,const std::function<uint32_t(uint32_t)>& palette){
 if(color<=0xffffff)return color;if(!palette)throw std::runtime_error("glyph palette color requires resolver");return palette(color&0xffffff);
}
}

#include "yukari/font_effect.hpp"
#include <cassert>
#include <iostream>
using namespace yukari;
int main(){
 FontEffectInput in;in.default_native_effect=font_effect_from_script(0);assert(resolve_font_effect(in).kind==FontEffectPlan::Plain);assert(font_effect_from_script(INT32_MAX)==INT32_MIN);
 in.default_native_effect=font_effect_from_script(1);auto plan=resolve_font_effect(in);assert(plan.kind==FontEffectPlan::Antialiased&&plan.supersample==4&&(plan.canvas(39,81)==std::pair<int32_t,int32_t>{9,20}));
 in.default_native_effect=font_effect_from_script(2);in.shadow_thickness=3;plan=resolve_font_effect(in);assert(plan.kind==FontEffectPlan::Shadow&&(plan.shadow_mask_translation_hi==std::pair<int32_t,int32_t>{12,12})&&(plan.canvas(39,81)==std::pair<int32_t,int32_t>{12,23}));
 in.default_native_effect=font_effect_from_script(3);in.border_thickness=3;plan=resolve_font_effect(in);assert(plan.kind==FontEffectPlan::Border&&(plan.color_glyph_origin_hi==std::pair<int32_t,int32_t>{6,6})&&plan.border_dilate_left_hi==12&&(plan.canvas(39,81)==std::pair<int32_t,int32_t>{13,24}));
 in.default_native_effect=font_effect_from_script(4);in.shadow_thickness=2;plan=resolve_font_effect(in);assert(plan.kind==FontEffectPlan::BorderShadow&&(plan.color_glyph_origin_hi==std::pair<int32_t,int32_t>{8,12})&&(plan.mask_glyph_origin_hi==std::pair<int32_t,int32_t>{12,8})&&(plan.shadow_mask_translation_hi==std::pair<int32_t,int32_t>{8,6})&&(plan.canvas(39,81)==std::pair<int32_t,int32_t>{24,32}));
 in.native_effect=2;in.default_native_effect=5;in.shadow_thickness=-1;in.border_thickness=-1;plan=resolve_font_effect(in);assert(plan.kind==FontEffectPlan::Antialiased); // Unused thickness is not validated or consumed.
 in.native_effect=-1;in.shadow_thickness=2;in.border_thickness=2;in.shadow_color=0xffffffff;in.border_color=0x123456;in.default_shadow_color=0xabcdef;plan=resolve_font_effect(in);assert(plan.native_effect==5&&plan.shadow_color==0xabcdef&&plan.border_color==0x123456);
 in.default_shadow_color=0xffffffff;plan=resolve_font_effect(in);assert(plan.kind==FontEffectPlan::BorderShadow&&plan.shadow_color==0xffffffff); // Inheritance sentinel does not disable effects.
 in.native_effect=99;plan=resolve_font_effect(in);assert(plan.kind==FontEffectPlan::Plain&&plan.supersample==1&&(plan.canvas(39,81)==std::pair<int32_t,int32_t>{39,81}));
 assert(resolve_glyph_foreground(0x123456,{})==0x123456);uint32_t seen=0;assert(resolve_glyph_foreground(0x01000007,[&](uint32_t index){seen=index;return uint32_t(0xaabbcc);})==0xaabbcc&&seen==7);
 std::cout<<"native/script effect numbering, supersampling, shadow/border extents, combined pass origins and color inheritance passed\n";
}

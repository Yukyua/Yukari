#include "yukari/native_glyph_effect.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace yukari;
NativeGlyphPlane plane(int w,int h,uint32_t c){return {w,h,std::vector<uint32_t>(size_t(w)*h,c)};}
FontEffectPlan effect(int n,int b=2,int s=2){FontEffectInput in;in.native_effect=n;in.border_thickness=b;in.shadow_thickness=s;in.border_color=0xff0000;in.shadow_color=0x00ff00;return resolve_font_effect(in);}
uint32_t at(const NativeGlyphPlane& p,int x,int y){return p.pixels[size_t(y)*p.width+x];}
int main(){
 assert(native_glyph_colorref_to_dib(0x00123456)==0x00563412);
 assert(native_glyph_colorref_to_dib(0xff123456)==0x00563412);
 // Arithmetic uses all four channels, floor rather than nearest; no alpha
 // premultiplication. One black sample among fifteen white samples = 239.
 auto high=plane(4,4,0x00ffffff);high.pixels[0]=0;
 assert(native_glyph_downsample4(high).pixels[0]==0x00efefef);
 high=plane(4,4,0);high.pixels[0]=0xffffffff;assert(native_glyph_downsample4(high).pixels[0]==0x0f0f0f0f);
 high=plane(8,8,0);for(int y=0;y<4;++y)for(int x=0;x<4;++x)high.pixels[y*8+x]=0x00123456;
 auto low=native_glyph_downsample4(high);assert(at(low,0,0)==0x00123456&&at(low,0,1)==0&&at(low,1,0)==0);
 // A single black source sample expands into an inclusive 5x5 rectangle for
 // b=1 (four high-res steps), left and DOWN in top-down coordinates.
 auto color=plane(12,12,0x00123456),mask=plane(12,12,0x00ffffff);
 mask.pixels[2*12+6]=0;auto out=native_glyph_finish(effect(4,1),color,mask);
 auto expected=plane(12,12,0x00ffffff);for(int y=2;y<=6;++y)for(int x=2;x<=6;++x)expected.pixels[y*12+x]=0;
 assert(out.transparency.pixels==native_glyph_downsample4(expected).pixels);
 // Predicate is whole DWORD != white, not threshold/min-filter: gray source
 // becomes pure-black expansion, but original source sample remains gray.
 mask=plane(12,12,0x00ffffff);mask.pixels[2*12+6]=0x00808080;
 out=native_glyph_finish(effect(4,1),color,mask);expected=plane(12,12,0x00ffffff);
 for(int y=2;y<=6;++y)for(int x=2;x<=6;++x)expected.pixels[y*12+x]=0;expected.pixels[2*12+6]=0x00808080;
 assert(out.transparency.pixels==native_glyph_downsample4(expected).pixels);
 // Combined shadow is +8,+6 high-res for s=2, not the usual +8,+8.
 // b=0 isolates that pass. Fully black destination keeps its old color,
 // while a partially covered destination is overwritten with shadow color.
 color=plane(24,24,0x00ff0000);mask=plane(24,24,0x00ffffff);mask.pixels[4*24+2]=0;
 out=native_glyph_finish(effect(5,0,2),color,mask);expected=mask;expected.pixels[10*24+10]=0;
 auto expected_color=color;expected_color.pixels[10*24+10]=0x0000ff00;
 assert(out.transparency.pixels==native_glyph_downsample4(expected).pixels);
 assert(out.color.pixels==native_glyph_downsample4(expected_color).pixels);
 mask.pixels[10*24+10]=0;color.pixels[10*24+10]=0x000000ff;
 out=native_glyph_finish(effect(5,0,2),color,mask);assert(at(out.color,2,2)==at(native_glyph_downsample4(color),2,2));
 mask.pixels[10*24+10]=0x00808080;out=native_glyph_finish(effect(5,0,2),color,mask);
 expected_color=color;expected_color.pixels[10*24+10]=0x0000ff00;assert(at(out.color,2,2)==at(native_glyph_downsample4(expected_color),2,2));
 // From actual binary GDI black-on-white mask: native border origins and
 // odd thickness survive the complete compose/downsample path.
 auto glyph=plane(9,9,0x00ffffff);glyph.pixels[4*9+4]=0;
 out=native_glyph_effect_binary(effect(4,3),glyph,0x000000ff);
 assert(out.color.width==6&&out.color.height==6&&at(out.color,2,2)==0x000f00ef);
 assert(at(out.transparency,1,1)==0x00000000); // full 4x4 coverage in expansion
 auto aa=native_glyph_effect_binary(effect(2),glyph,0x000000ff);assert(aa.color.width==2&&aa.color.height==2&&at(aa.color,1,1)==0x00ff0000&&at(aa.transparency,1,1)==0x00efefef);
 glyph.pixels[0]=0x00808080;bool rejected=false;try{native_glyph_effect_binary(effect(2),glyph,0);}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 // Packed 16-bit sums retain all four bytes, including maximal alpha and
 // cross-lane carry boundaries, against the scalar native averaging rule.
 uint32_t seed=0x8239ad17;auto random=plane(128,128,0);
 for(auto& p:random.pixels){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;p=seed;}
 auto packed=native_glyph_downsample4(random);
 for(int y=0;y<32;++y)for(int x=0;x<32;++x){uint32_t expected=0;
  for(int c=0;c<4;++c){uint32_t sum=0;for(int dy=0;dy<4;++dy)for(int dx=0;dx<4;++dx)sum+=(random.pixels[(y*4+dy)*128+x*4+dx]>>(c*8))&255;expected|=(sum/16)<<(c*8);}
  assert(packed.pixels[y*32+x]==expected);
 }
 assert(native_glyph_downsample4(plane(4,4,0xffffffff)).pixels[0]==0xffffffff);
 // Invalid original zero-shadow config is clipped to allocated storage.
 native_glyph_finish(effect(5,0,0),plane(4,4,0),plane(4,4,0));
 std::cout<<"native glyph byte averaging, bottom-up in-place dilation, combined shadow, color planes and binary compose passed\n";
}

#include "yukari/gdi_cache.hpp"
#include <iostream>
#include <stdexcept>
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
int main(){try{
 yukari::GdiGlyphCache c;c.height=-27;c.width=-13;c.weight=200;c.charset=128;c.quality=0;c.scale=4;c.acp=932;c.extent_width=2;c.extent_height=1;c.bitmap_width=4;c.bitmap_height=3;c.origin_x=1;c.origin_y=1;c.family="ＭＳ ゴシック";c.text="日";c.actual_family=c.family;c.host="windows-native-test";c.font_sha256=std::string(64,'a');c.bgra.assign(4*3*4,255);c.bgra[20]=c.bgra[21]=c.bgra[22]=0;
 c.color_bgra=c.bgra;c.color_bgra[20]=255;auto bytes=yukari::encode_gdi_cache(c);auto d=yukari::decode_gdi_cache(bytes);check(d.height== -27&&d.width== -13&&d.text==c.text&&d.family==c.family&&d.bgra==c.bgra&&d.origin_x==1&&d.color_bgra==c.color_bgra,"GDI cache round trip");
 for(auto broken:{yukari::Bytes(bytes.begin(),bytes.end()-1),yukari::Bytes(bytes.begin(),bytes.begin()+127)}){bool bad=false;try{yukari::decode_gdi_cache(broken);}catch(const std::runtime_error&){bad=true;}check(bad,"truncated cache accepted");}
 bytes[8+18*4]=0;bool bad=false;try{yukari::decode_gdi_cache(bytes);}catch(const std::runtime_error&){bad=true;}check(bad,"invalid stride accepted");
 c.native_effect=4;c.border_thickness=2;c.shadow_thickness=2;c.bitmap_width=4;c.bitmap_height=4;c.bgra.assign(64,255);c.color_bgra=c.bgra;c.origin_x=c.origin_y=0;c.extent_width=c.extent_height=4;auto scratch=yukari::decode_gdi_cache(yukari::encode_gdi_cache(c));check(scratch.native_effect==4&&scratch.border_thickness==2&&scratch.bgra==c.bgra,"GDI scratch cache3 round trip");
 c.bgra[3]=0;bad=false;try{yukari::encode_gdi_cache(c);}catch(const std::runtime_error&){bad=true;}check(bad,"undefined GDI alpha accepted");
 std::cout<<"GDI source cache codec tests passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

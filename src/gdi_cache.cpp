#include "yukari/gdi_cache.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace yukari { namespace {
void validate(const GdiGlyphCache& c){
 if(c.bitmap_width<=0||c.bitmap_height<=0||c.bitmap_width>32768||c.bitmap_height>32768||uint64_t(c.bitmap_width)*c.bitmap_height>64000000||c.bgra.size()!=uint64_t(c.bitmap_width)*c.bitmap_height*4||c.color_bgra.size()!=c.bgra.size())throw std::runtime_error("invalid GDI cache bitmap");
 if(c.height==0||c.height< -512||c.height>512||c.width< -512||c.width>512||(c.scale!=1&&c.scale!=2&&c.scale!=4)||c.charset>255||c.quality>255||c.out_precision>255||c.clip_precision>255||c.pitch>255||c.extent_width<0||c.extent_height<0||c.origin_x<0||c.origin_y<0||(!c.native_effect&&(int64_t(c.extent_width)+c.origin_x>c.bitmap_width||int64_t(c.extent_height)+c.origin_y>c.bitmap_height)))throw std::runtime_error("invalid GDI cache font/geometry");
 for(const auto* s:{&c.family,&c.text,&c.actual_family,&c.host})if(s->size()>65536||s->find('\0')!=std::string::npos)throw std::runtime_error("invalid GDI cache string");
 if(c.font_sha256.size()!=64||!std::all_of(c.font_sha256.begin(),c.font_sha256.end(),[](char x){return (x>='0'&&x<='9')||(x>='a'&&x<='f');}))throw std::runtime_error("GDI cache requires lowercase SHA256 font-file hash");
 if(c.native_effect&&(c.native_effect<1||c.native_effect>5||c.border_thickness<0||c.border_thickness>512||c.shadow_thickness<0||c.shadow_thickness>512||c.border_color>0xffffff||c.shadow_color>0xffffff||c.scale!=(c.native_effect==1?1U:4U)||c.origin_x||c.origin_y||c.bitmap_width%int(c.scale)||c.bitmap_height%int(c.scale)))throw std::runtime_error("invalid GDI native scratch descriptor");
 if(c.foreground>0xffffff||c.background>0xffffff)throw std::runtime_error("invalid GDI cache COLORREF");
 if(c.host.empty()||c.actual_family.empty())throw std::runtime_error("GDI cache requires host and actual family");
 for(size_t i=3;i<c.bgra.size();i+=4)if(c.bgra[i]!=255||c.color_bgra[i]!=255)throw std::runtime_error("GDI cache source has undefined alpha");
}
}
Bytes encode_gdi_cache(const GdiGlyphCache& c){
 validate(c);const bool scratch=c.native_effect!=0;Bytes out(scratch?168:128);std::memcpy(out.data(),scratch?"YKGDI003":"YKGDI002",8);
 auto word=[&](size_t i,uint32_t n){for(size_t k=0;k<4;++k)out[8+i*4+k]=uint8_t(n>>(8*k));};
 const uint32_t fields[]={scratch?3U:2U,scratch?168U:128U,scratch?7U:3U,uint32_t(c.height),uint32_t(c.width),uint32_t(c.weight),c.charset,c.quality,c.scale,c.out_precision,c.clip_precision,c.pitch,uint32_t(c.extent_width),uint32_t(c.extent_height),uint32_t(c.bitmap_width),uint32_t(c.bitmap_height),uint32_t(c.origin_x),uint32_t(c.origin_y),uint32_t(c.bitmap_width)*4,c.acp,uint32_t(c.family.size()),uint32_t(c.text.size()),uint32_t(c.actual_family.size()),uint32_t(c.host.size()),uint32_t(c.font_sha256.size()),uint32_t(c.bgra.size()),c.foreground,c.background,uint32_t(c.color_bgra.size())};
 for(size_t i=0;i<sizeof(fields)/sizeof(*fields);++i)word(i,fields[i]);
 if(scratch){word(30,uint32_t(c.native_effect));word(31,uint32_t(c.border_thickness));word(32,uint32_t(c.shadow_thickness));word(33,c.border_color);word(34,c.shadow_color);word(35,uint32_t(c.bitmap_width)/c.scale);word(36,uint32_t(c.bitmap_height)/c.scale);}
 for(const auto* s:{&c.family,&c.text,&c.actual_family,&c.host,&c.font_sha256}){out.insert(out.end(),s->begin(),s->end());}
 out.insert(out.end(),c.bgra.begin(),c.bgra.end());out.insert(out.end(),c.color_bgra.begin(),c.color_bgra.end());return out;
}
GdiGlyphCache decode_gdi_cache(const Bytes& b){
 if(b.size()<128||(std::memcmp(b.data(),"YKGDI002",8)&&std::memcmp(b.data(),"YKGDI003",8)))throw std::runtime_error("invalid GDI cache header");
 auto word=[&](size_t i){uint32_t n=0;for(size_t k=0;k<4;++k)n|=uint32_t(b[8+i*4+k])<<(8*k);return n;};
 auto integer=[&](size_t i){uint32_t n=word(i);int32_t out;std::memcpy(&out,&n,4);return out;};
 const bool scratch=std::memcmp(b.data(),"YKGDI003",8)==0;
 if((scratch&&(b.size()<168||word(0)!=3||word(1)!=168||word(2)!=7))||(!scratch&&(word(0)!=2||word(1)!=128||word(2)!=3)))throw std::runtime_error("unsupported GDI cache version/flags");
 if(word(29))throw std::runtime_error("unsupported GDI cache reserved fields");
 GdiGlyphCache c;c.height=integer(3);c.width=integer(4);c.weight=integer(5);c.charset=word(6);c.quality=word(7);c.scale=word(8);c.out_precision=word(9);c.clip_precision=word(10);c.pitch=word(11);c.extent_width=integer(12);c.extent_height=integer(13);c.bitmap_width=integer(14);c.bitmap_height=integer(15);c.origin_x=integer(16);c.origin_y=integer(17);c.acp=word(19);
 if(c.bitmap_width<=0||word(18)!=uint64_t(c.bitmap_width)*4)throw std::runtime_error("invalid GDI cache stride");
 if(scratch){c.native_effect=integer(30);c.border_thickness=integer(31);c.shadow_thickness=integer(32);c.border_color=word(33);c.shadow_color=word(34);if(!c.scale||c.bitmap_width<=0||c.bitmap_height<=0||word(35)!=uint32_t(c.bitmap_width)/c.scale||word(36)!=uint32_t(c.bitmap_height)/c.scale||word(37)||word(38)||word(39))throw std::runtime_error("invalid GDI scratch output dimensions");}
 size_t at=scratch?168:128;auto str=[&](size_t field){size_t n=word(field);if(n>65536||n>b.size()-at)throw std::runtime_error("truncated GDI cache string");std::string s(b.begin()+at,b.begin()+at+n);at+=n;return s;};c.family=str(20);c.text=str(21);c.actual_family=str(22);c.host=str(23);c.font_sha256=str(24);
 size_t n=word(25),color_n=word(28);if(uint64_t(n)+color_n!=b.size()-at)throw std::runtime_error("invalid GDI cache pixel payload");c.bgra.assign(b.begin()+at,b.begin()+at+n);c.color_bgra.assign(b.begin()+at+n,b.end());c.foreground=word(26);c.background=word(27);validate(c);return c;
}
}

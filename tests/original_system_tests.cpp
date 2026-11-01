#include "yukari/original_system.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace yukari;
namespace {
void put(Bytes& b,size_t at,uint32_t n){for(unsigned i=0;i<4;++i)b.at(at+i)=uint8_t(n>>(i*8));}
void rejects(const std::function<void()>& f){bool threw=false;try{f();}catch(const std::runtime_error&){threw=true;}assert(threw);}
Bytes fixture(){Bytes b(0x7c+120000);put(b,0x10,0x7c);put(b,0x18,0x7c);put(b,0x1c,40000);put(b,0x50,0x7c+40000);put(b,0x54,80000);put(b,0x60,0x12345678);return b;}
}
int main(){auto b=fixture();auto s=decode_original_system(b);assert(s.flags.size()==10000&&s.strings.size()==10000);s.flags[150]=1;s.flags[9999]=-45;s.strings[0]="Test Game";s.strings[3]="日本語";auto out=encode_original_system(s);auto r=decode_original_system(out);assert(r.flags==s.flags&&r.strings==s.strings);assert(u32(out,0x60)==0x12345678&&s.template_bytes==b);assert(out.size()>b.size());
 auto corrupt=b;put(corrupt,0x54,79999);rejects([&]{decode_original_system(corrupt);});corrupt=b;put(corrupt,0x7c+40000,0xfffffff0);put(corrupt,0x7c+40000+4,100);rejects([&]{decode_original_system(corrupt);});
 corrupt=b;put(corrupt,0x7c+40000,0x7c);put(corrupt,0x7c+40000+4,4);put(corrupt,0x7c,0x41414141);rejects([&]{decode_original_system(corrupt);});s.strings[1]="😀";rejects([&]{encode_original_system(s);});OriginalSystemState fresh;rejects([&]{encode_original_system(fresh);});std::cout<<"original system banks, template preservation and malformed inputs passed\n";}

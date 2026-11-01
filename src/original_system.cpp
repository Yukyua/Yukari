#include "yukari/original_system.hpp"
#include "yukari/original_scene.hpp"
#include <limits>
#include <stdexcept>

namespace yukari {
namespace {
void validate(const Bytes& b) {
    require_range(b.size(),0,0x7c,"original system header");
    if(u32(b,0x10)!=0x7c) throw std::runtime_error("unsupported original System.dat header");
    const auto fa=u32(b,0x18),fs=u32(b,0x1c),sa=u32(b,0x50),ss=u32(b,0x54);
    if(fs!=40000 || ss!=80000 || fa<0x7c || sa<0x7c) throw std::runtime_error("unsupported original System.dat bank dimensions");
    require_range(b.size(),fa,fs,"original global flags");require_range(b.size(),sa,ss,"original global string index");
}
void put(Bytes& b,size_t at,uint32_t value) {
    require_range(b.size(),at,4,"original system output");
    for(unsigned i=0;i<4;++i)b[at+i]=static_cast<uint8_t>(value>>(i*8));
}
uint32_t position(const Bytes& b) {
    if(b.size()>std::numeric_limits<uint32_t>::max())throw std::runtime_error("original System.dat exceeds 32-bit offsets");
    return static_cast<uint32_t>(b.size());
}
}
OriginalSystemState decode_original_system(const Bytes& b,const char* encoding) {
    validate(b);OriginalSystemState out;
    out.flags.resize(10000);out.strings.resize(10000);out.encoding=encoding;
    const auto flags=u32(b,0x18),table=u32(b,0x50);
    uint64_t string_bytes=0;
    for(size_t i=0;i<10000;++i) {
        out.flags[i]=static_cast<int32_t>(u32(b,size_t(flags)+i*4));
        const auto at=u32(b,size_t(table)+i*8),size=u32(b,size_t(table)+i*8+4);
        if(!size){if(at)throw std::runtime_error("invalid original global string descriptor");continue;}
        string_bytes+=size;
        if(at<0x7c || size>1024*1024 || string_bytes>64*1024*1024)throw std::runtime_error("original global strings exceed limit");
        require_range(b.size(),at,size,"original global string");
        auto first=b.begin()+at,end=first,last=first+size;
        while(end!=last && *end)++end;
        if(end==last)throw std::runtime_error("unterminated original global string");
        out.strings[i]=decode(std::string(first,end),encoding);
    }
    out.template_bytes=b;return out;
}
Bytes encode_original_system(const OriginalSystemState& state) {
    if(state.template_bytes.empty())throw std::runtime_error("native System.dat writing requires an original template");
    validate(state.template_bytes);
    if(state.flags.size()>10000 || state.strings.size()>10000)throw std::runtime_error("original global bank exceeds 10000 entries");
    Bytes out=state.template_bytes;
    const auto flags=position(out);
    if(uint64_t(flags)+40000>std::numeric_limits<uint32_t>::max())throw std::runtime_error("original System.dat output exceeds limit");
    out.resize(out.size()+40000,0);
    for(size_t i=0;i<state.flags.size();++i)put(out,size_t(flags)+i*4,static_cast<uint32_t>(state.flags[i]));
    Bytes table(80000);uint64_t string_bytes=0;
    for(size_t i=0;i<state.strings.size();++i) {
        if(state.strings[i].empty())continue;
        const auto encoded=encode_original_text(state.strings[i],state.encoding.c_str());
        string_bytes+=encoded.size()+2;
        if(encoded.size()>1024*1024 || string_bytes>64*1024*1024)throw std::runtime_error("original global strings output exceeds limit");
        const auto at=position(out);
        if(uint64_t(at)+encoded.size()+2>std::numeric_limits<uint32_t>::max())throw std::runtime_error("original System.dat output exceeds limit");
        put(table,i*8,at);put(table,i*8+4,static_cast<uint32_t>(encoded.size()+2));
        out.insert(out.end(),encoded.begin(),encoded.end());out.push_back(0);out.push_back(0);
    }
    const auto strings=position(out);
    if(uint64_t(strings)+table.size()>std::numeric_limits<uint32_t>::max())throw std::runtime_error("original System.dat output exceeds limit");
    out.insert(out.end(),table.begin(),table.end());
    put(out,0x18,flags);put(out,0x1c,40000);put(out,0x50,strings);put(out,0x54,80000);
    return out;
}
}

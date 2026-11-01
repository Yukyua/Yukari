#include "yukari/original_scene.hpp"
#include <iconv.h>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace yukari {
namespace {
class Reader {
public:
    Reader(const Bytes& bytes, size_t base) : bytes_(bytes), base_(base) {
        range(0, 0x48);
        if (bytes_[base] != 'Y' || bytes_[base+1] != 'u' || bytes_[base+2] != 'k' || bytes_[base+3] != 'a')
            throw std::runtime_error("invalid original scene container");
    }
    void range(uint64_t offset, uint64_t size) const {
        if (offset > bytes_.size() - base_) throw std::runtime_error("original scene offset out of range");
        require_range(bytes_.size(), base_ + offset, size, "original scene");
    }
    uint32_t word(size_t at) const { range(at,4); return u32(bytes_,base_+at); }
    int integer(size_t at) const { return static_cast<int32_t>(word(at)); }
    uint8_t byte(size_t at) const { range(at,1); return bytes_[base_+at]; }
    Bytes slice(uint32_t at, uint32_t size) const {
        range(at,size);
        return {bytes_.begin()+base_+at,bytes_.begin()+base_+at+size};
    }
    std::string string(uint32_t at, uint32_t size) const {
        if (!size) return {};
        if (!at || size > 1024*1024) throw std::runtime_error("invalid original scene string descriptor");
        range(at,size);
        auto first=bytes_.begin()+base_+at, last=first+size;
        auto end=first;
        while (end!=last && *end) ++end;
        if (end==last) throw std::runtime_error("unterminated original scene string");
        return decode(std::string(first,end));
    }
private:
    const Bytes& bytes_;
    size_t base_;
};
}
OriginalScene decode_original_scene(const Bytes& bytes,size_t base) {
    if (base > bytes.size()) throw std::runtime_error("original scene base out of range");
    Reader r(bytes,base);
    auto state=r.word(0x20), state_size=r.word(0x24);
    r.range(state,state_size);
    if (state_size<0x160 || r.word(state)<0x160) throw std::runtime_error("unsupported original state header");
    OriginalScene out;
    uint64_t total_sprites=0, total_payload=0;
    uint32_t at=r.word(state+0x24), count=r.word(state+0x28);
    if (count>65536) throw std::runtime_error("original layer count exceeds limit");
    std::unordered_set<uint32_t> seen;
    while (count--) {
        if (!at || !seen.insert(at).second) throw std::runtime_error("invalid original layer chain");
        r.range(at,0x140);
        if (r.word(at)!=0x140) throw std::runtime_error("unsupported original layer header");
        OriginalLayer layer;
        layer.native_header=r.slice(at,0x140);
        layer.id=r.integer(at+8); layer.image_type=r.integer(at+0xc);
        layer.x=r.integer(at+0x20); layer.y=r.integer(at+0x24); layer.visible=r.byte(at+0x28)!=0;
        layer.transmittance=r.integer(at+0x74);
        layer.resource=r.string(r.word(at+0x5c),r.word(at+0x60));
        layer.mask=r.string(r.word(at+0x64),r.word(at+0x68));
        const auto sprites=r.word(at+0x6c), sprite_count=r.word(at+0x70);
        if (sprite_count>65536) throw std::runtime_error("original sprite count exceeds limit");
        total_sprites+=sprite_count;
        if (total_sprites>200000) throw std::runtime_error("original total sprite count exceeds limit");
        r.range(sprites,uint64_t(sprite_count)*0x7c);
        for (uint32_t i=0;i<sprite_count;++i) {
            const size_t p=uint64_t(sprites)+uint64_t(i)*0x7c;
            if (r.word(p)!=0x7c) throw std::runtime_error("unsupported original sprite header");
            OriginalSprite s;
            s.native_header=r.slice(static_cast<uint32_t>(p),0x7c);
            s.kind=r.integer(p+8); s.visible=r.byte(p+0xc)!=0;
            s.x=r.integer(p+0x10);s.y=r.integer(p+0x14);s.width=r.integer(p+0x18);s.height=r.integer(p+0x1c);
            s.left=r.integer(p+0x20);s.top=r.integer(p+0x24);s.right=r.integer(p+0x28);s.bottom=r.integer(p+0x2c);
            s.wait=r.integer(p+0x30);s.source_layer=r.integer(p+0x34);s.mode=r.integer(p+0x38);
            s.encoded_character=static_cast<uint16_t>(r.word(p+0x3c));s.font=r.integer(p+0x40);s.color=r.word(p+0x44);
            s.group=r.integer(p+0x48);s.frame=r.integer(p+0x4c);if(s.kind==3){s.font_effect=r.integer(p+0x48);s.shadow_color=r.word(p+0x4c);s.border_color=r.word(p+0x50);}s.slot=r.integer(p+0x5c);s.named=r.byte(p+0x60)!=0;
            const auto payload=r.word(p+0x70), size=r.word(p+0x74);
            if (size>64*1024*1024) throw std::runtime_error("original sprite payload exceeds limit");
            total_payload+=size;
            if (total_payload>256*1024*1024) throw std::runtime_error("original total sprite payload exceeds limit");
            if (size && !payload) throw std::runtime_error("invalid original sprite payload");
            if (size) s.payload=r.slice(payload,size);
            if ((s.kind==0 || s.kind==1) && size) {
                if (size%32) throw std::runtime_error("invalid original sprite animation payload");
                for (size_t j=0;j<size;j+=32) s.animation.push_back({u32(s.payload,j),u32(s.payload,j+4),u32(s.payload,j+8),u32(s.payload,j+12),u32(s.payload,j+16),u32(s.payload,j+20),u32(s.payload,j+24),u32(s.payload,j+28)});
            }
            layer.sprites.push_back(std::move(s));
        }
        const auto next=r.word(at+4);
        if (count && next<=at) throw std::runtime_error("invalid original layer next offset");
        out.layers.push_back(std::move(layer)); at=next;
    }
    at=r.word(state+0x1c);count=r.word(state+0x20);seen.clear();
    if (count>65536) throw std::runtime_error("original audio count exceeds limit");
    while (count--) {
        if (!at || !seen.insert(at).second) throw std::runtime_error("invalid original audio chain");
        r.range(at,0x20);
        if (r.word(at)!=0x20) throw std::runtime_error("unsupported original audio header");
        OriginalSound sound;
        sound.native_header=r.slice(at,0x20);
        sound.id=r.integer(at+8);sound.playing=r.byte(at+0xc)!=0;sound.loop=r.byte(at+0xd)!=0;sound.mode=r.byte(at+0xe);
        sound.resource=r.string(r.word(at+0x10),r.word(at+0x14));
        sound.volume=r.integer(at+0x18);sound.parameter=r.integer(at+0x1c);
        const auto next=r.word(at+4);
        if (count && next<=at) throw std::runtime_error("invalid original audio next offset");
        out.sounds.push_back(std::move(sound));at=next;
    }
    return out;
}

namespace {
void put_word(Bytes& b,size_t p,uint32_t value) {
    require_range(b.size(),p,4,"original scene output");
    for (unsigned i=0;i<4;++i) b[p+i]=static_cast<uint8_t>(value>>(8*i));
}
Bytes header(const Bytes& original,size_t size) {
    if (!original.empty() && original.size()!=size) throw std::runtime_error("invalid original scene output header size");
    Bytes out=original.empty()?Bytes(size):original;
    put_word(out,0,static_cast<uint32_t>(size));
    return out;
}
}
std::string encode_original_text(const std::string& input,const char* encoding) {
    if (input.size()>1024*1024 || input.find('\0')!=std::string::npos) throw std::runtime_error("invalid original resource name");
    iconv_t codec=iconv_open(encoding,"UTF-8");
    if (codec==reinterpret_cast<iconv_t>(-1)) throw std::runtime_error("original text encoding unavailable");
    std::string output(input.size()*2+16,'\0');
    char* source=const_cast<char*>(input.data());char* dest=output.data();
    size_t source_size=input.size(), dest_size=output.size();
    const auto result=iconv(codec,&source,&source_size,&dest,&dest_size);
    iconv_close(codec);
    if (result==size_t(-1) || source_size) throw std::runtime_error("text cannot be encoded in original encoding");
    output.resize(output.size()-dest_size);
    return output;
}
namespace {
class Writer {
public:
    Writer(Bytes& bytes,size_t base):bytes_(bytes),base_(base) {}
    uint32_t position() const {
        if (bytes_.size()<base_ || bytes_.size()-base_>std::numeric_limits<uint32_t>::max()) throw std::runtime_error("original scene output exceeds 32-bit offsets");
        return static_cast<uint32_t>(bytes_.size()-base_);
    }
    uint32_t append(const Bytes& b) {
        auto at=position();
        if (uint64_t(at)+b.size()>std::numeric_limits<uint32_t>::max()) throw std::runtime_error("original scene output exceeds 32-bit offsets");
        bytes_.insert(bytes_.end(),b.begin(),b.end());return at;
    }
    void word(uint32_t at,uint32_t value) {put_word(bytes_,base_+at,value);}
    void string(uint32_t descriptor,const std::string& s) {
        if(s.empty()){word(descriptor,0);word(descriptor+4,0);return;}
        const auto encoded=encode_original_text(s);Bytes data(encoded.begin(),encoded.end());data.push_back(0);
        word(descriptor,append(data));word(descriptor+4,static_cast<uint32_t>(data.size()));
    }
private:
    Bytes& bytes_;size_t base_;
};
}
void encode_original_scene(Bytes& bytes,const OriginalScene& scene,size_t base) {
    if (base>bytes.size()) throw std::runtime_error("original scene output base out of range");
    Reader reader(bytes,base);
    const auto state=reader.word(0x20);
    reader.range(state,0x160);
    if(reader.word(state)<0x160) throw std::runtime_error("unsupported original state output header");
    if(scene.layers.size()>65536 || scene.sounds.size()>65536) throw std::runtime_error("original scene output count exceeds limit");
    Bytes output=bytes;Writer w(output,base);
    w.word(state+0x24,scene.layers.empty()?0:w.position());w.word(state+0x28,static_cast<uint32_t>(scene.layers.size()));
    uint64_t total_sprites=0,total_payload=0;
    for(const auto& layer:scene.layers) {
        total_sprites+=layer.sprites.size();
        if(layer.sprites.size()>65536 || total_sprites>200000) throw std::runtime_error("original scene output sprite count exceeds limit");
        auto h=header(layer.native_header,0x140);
        if(layer.native_header.empty()){put_word(h,0x30,0xffffff);put_word(h,0x80,0xffffffff);put_word(h,0x84,0xffffffff);}
        put_word(h,8,layer.id);put_word(h,0xc,layer.image_type);put_word(h,0x20,layer.x);put_word(h,0x24,layer.y);h[0x28]=layer.visible;
        put_word(h,0x74,layer.transmittance);
        const auto start=w.append(h);w.string(start+0x5c,layer.resource);w.string(start+0x64,layer.mask);
        Bytes records;
        for(const auto& sprite:layer.sprites) {
            auto sh=header(sprite.native_header,0x7c);
            put_word(sh,8,sprite.kind);sh[0xc]=sprite.visible;
            put_word(sh,0x10,sprite.x);put_word(sh,0x14,sprite.y);put_word(sh,0x18,sprite.width);put_word(sh,0x1c,sprite.height);
            put_word(sh,0x20,sprite.left);put_word(sh,0x24,sprite.top);put_word(sh,0x28,sprite.right);put_word(sh,0x2c,sprite.bottom);
            put_word(sh,0x30,sprite.wait);put_word(sh,0x34,sprite.source_layer);put_word(sh,0x38,sprite.mode);
            sh[0x3c]=static_cast<uint8_t>(sprite.encoded_character);sh[0x3d]=static_cast<uint8_t>(sprite.encoded_character>>8);
            put_word(sh,0x40,sprite.font);put_word(sh,0x44,sprite.color);put_word(sh,0x48,sprite.kind==3?sprite.font_effect:sprite.group);put_word(sh,0x4c,sprite.kind==3?sprite.shadow_color:uint32_t(sprite.frame));if(sprite.kind==3)put_word(sh,0x50,sprite.border_color);
            put_word(sh,0x5c,sprite.slot);sh[0x60]=sprite.named;
            Bytes payload=sprite.payload;
            if(!sprite.animation.empty()) {
                if(sprite.animation.size()>64*1024*1024/32) throw std::runtime_error("original sprite output animation exceeds limit");
                payload.assign(sprite.animation.size()*32,0);size_t p=0;
                for(const auto& f:sprite.animation) {
                    for(auto v:{f.x,f.y,f.w,f.h,f.duration,f.type,f.unknown,f.action}){put_word(payload,p,v);p+=4;}
                }
            }
            total_payload+=payload.size();
            if(payload.size()>64*1024*1024 || total_payload>256*1024*1024) throw std::runtime_error("original sprite output payload exceeds limit");
            if((sprite.kind==0 || sprite.kind==1) && payload.size()%32) throw std::runtime_error("invalid original sprite output animation");
            put_word(sh,0x70,payload.empty()?0:w.append(payload));put_word(sh,0x74,static_cast<uint32_t>(payload.size()));
            records.insert(records.end(),sh.begin(),sh.end());
        }
        w.word(start+0x6c,records.empty()?0:w.append(records));w.word(start+0x70,static_cast<uint32_t>(layer.sprites.size()));
        w.word(start+4,w.position());
    }
    w.word(state+0x1c,scene.sounds.empty()?0:w.position());w.word(state+0x20,static_cast<uint32_t>(scene.sounds.size()));
    for(const auto& sound:scene.sounds) {
        auto h=header(sound.native_header,0x20);
        put_word(h,8,sound.id);h[0xc]=sound.playing;h[0xd]=sound.loop;h[0xe]=sound.mode;
        put_word(h,0x18,sound.volume);put_word(h,0x1c,sound.parameter);
        const auto start=w.append(h);w.string(start+0x10,sound.resource);w.word(start+4,w.position());
    }
    w.word(0x24,w.position()-state);
    bytes.swap(output);
}
}

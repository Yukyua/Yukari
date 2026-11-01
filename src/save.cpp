#include "yukari/save.hpp"
#include <algorithm>
#include <cstring>
#include <iconv.h>
#include <limits>
#include <stdexcept>

namespace yukari {
namespace {
constexpr size_t maximum = 128 * 1024 * 1024;
constexpr char magic[12] = {'Y','U','K','A','R','I','S','A','V','E',0,0};
uint64_t checksum(const Bytes& bytes, size_t offset) {
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = offset; i < bytes.size(); ++i) hash = (hash ^ bytes[i]) * 1099511628211ULL;
    return hash;
}
struct Writer {
    Bytes bytes;
    void number(uint64_t n, unsigned count = 4) {
        if (bytes.size() > maximum - count) throw std::runtime_error("save exceeds 128 MiB limit");
        for (unsigned i = 0; i < count; ++i) bytes.push_back(uint8_t(n >> (i*8)));
    }
    void blob(const Bytes& value) {
        if (value.size() > maximum - 4 || bytes.size() > maximum - 4 - value.size())
            throw std::runtime_error("save exceeds 128 MiB limit");
        number(value.size());
        bytes.insert(bytes.end(), value.begin(), value.end());
    }
};
struct Reader {
    const Bytes& bytes;
    size_t position{};
    uint64_t number(unsigned count = 4) {
        require_range(bytes.size(), position, count, "save number");
        uint64_t result = 0;
        for (unsigned i = 0; i < count; ++i) result |= uint64_t(bytes[position++]) << (i*8);
        return result;
    }
    Bytes blob() {
        auto size = number();
        require_range(bytes.size(), position, size, "save section");
        Bytes result(bytes.begin() + position, bytes.begin() + position + size);
        position += size;
        return result;
    }
};
std::string encoded(const std::string& text, const char* encoding) {
    if (text.size() > 16 * 1024 * 1024 || text.find('\0') != std::string::npos)
        throw std::runtime_error("original save text exceeds limits or contains a NUL");
    const auto converter = iconv_open(encoding, "UTF-8");
    if (converter == reinterpret_cast<iconv_t>(-1)) throw std::runtime_error("original save encoding unavailable");
    std::string result(text.size()*4 + 4, 0);
    char* source = const_cast<char*>(text.data()); char* target = result.data();
    size_t input = text.size(), output = result.size();
    const auto converted = iconv(converter, &source, &input, &target, &output);
    iconv_close(converter);
    if (converted == size_t(-1)) throw std::runtime_error("text cannot be represented in original save encoding");
    result.resize(result.size() - output); return result;
}
}
Bytes encode_save(const SaveGame& game) {
    if (game.comment.size() > 1024 * 1024) throw std::runtime_error("save comment exceeds 1 MiB limit");
    Writer payload;
    payload.number(uint64_t(game.timestamp), 8);
    payload.blob(Bytes(game.comment.begin(), game.comment.end()));
    payload.blob(game.thumbnail);
    payload.blob(game.vm);
    payload.blob(game.runtime);
    Writer output;
    output.bytes.assign(magic, magic + sizeof(magic));
    output.number(1); output.number(40);
    output.number(payload.bytes.size(), 8);
    output.number(checksum(payload.bytes, 0), 8);
    output.number(0);
    if (payload.bytes.size() > maximum - output.bytes.size()) throw std::runtime_error("save exceeds 128 MiB limit");
    output.bytes.insert(output.bytes.end(), payload.bytes.begin(), payload.bytes.end());
    return output.bytes;
}
SaveGame decode_save(const Bytes& bytes) {
    if (bytes.size() > maximum) throw std::runtime_error("save exceeds 128 MiB limit");
    if (bytes.size() < 40 || std::memcmp(bytes.data(), magic, sizeof(magic)))
        throw std::runtime_error("not a Yukari save (original Yuka saves require a separate importer)");
    Reader input{bytes, 12};
    if (input.number() != 1 || input.number() != 40) throw std::runtime_error("unsupported Yukari save version");
    const auto length = input.number(8), hash = input.number(8);
    if (input.number() || length != bytes.size() - 40) throw std::runtime_error("invalid Yukari save header");
    if (checksum(bytes, 40) != hash) throw std::runtime_error("Yukari save checksum mismatch");
    SaveGame result;
    const auto timestamp = input.number(8);
    std::memcpy(&result.timestamp, &timestamp, sizeof(timestamp));
    auto comment = input.blob();
    if (comment.size() > 1024 * 1024) throw std::runtime_error("save comment exceeds 1 MiB limit");
    result.comment.assign(comment.begin(), comment.end());
    result.thumbnail = input.blob(); result.vm = input.blob(); result.runtime = input.blob();
    if (input.position != bytes.size()) throw std::runtime_error("trailing Yukari save data");
    return result;
}
OriginalSaveInfo inspect_original_save(const Bytes& bytes) {
    if (bytes.size() < 72 || std::memcmp(bytes.data(), "Yuka  \0\0", 8))
        throw std::runtime_error("unsupported original Yuka save signature");
    OriginalSaveInfo result;
    result.header_size = u32(bytes, 8);
    if (result.header_size != 72) throw std::runtime_error("unobserved original Yuka save header size");
    for (uint32_t descriptor = 0x10; descriptor < 0x48; descriptor += 8) {
        OriginalSaveRegion region{descriptor, u32(bytes, descriptor), u32(bytes, descriptor + 4)};
        require_range(bytes.size(), region.offset, region.size, "original save header region");
        result.regions.push_back(region);
    }
    return result;
}
OriginalSaveMetadata read_original_save_metadata(const Bytes& bytes, const char* encoding) {
    inspect_original_save(bytes);
    OriginalSaveMetadata result;
    auto text = [&](size_t at) {
        const auto offset=u32(bytes,at),size=u32(bytes,at+4);
        if(!size)return std::string{};
        if(size>16*1024*1024)throw std::runtime_error("original save metadata string exceeds limit");
        const auto begin=bytes.begin()+offset,end=begin+size;
        auto nul=std::find(begin,end,uint8_t(0));
        if(nul==end)throw std::runtime_error("unterminated original save metadata string");
        return decode(std::string(begin,nul),encoding);
    };
    result.comment1=text(0x10);result.comment2=text(0x18);result.title=text(0x40);
    const auto at=u32(bytes,0x30),size=u32(bytes,0x34);
    result.thumbnail_bmp.assign(bytes.begin()+at,bytes.begin()+at+size);
    return result;
}
Bytes rewrite_original_save_metadata(const Bytes& bytes, const OriginalSaveMetadataChange& change) {
    inspect_original_save(bytes);
    if(bytes.size()>maximum)throw std::runtime_error("original save exceeds 128 MiB");
    auto comment_bytes=[&](const std::optional<std::string>& comment){Bytes result;if(comment){auto value=encoded(*comment,change.encoding);result.assign(value.begin(),value.end());result.push_back(0);}return result;};
    const auto comment1=comment_bytes(change.comment1),comment2=comment_bytes(change.comment2);
    if(change.thumbnail_bmp){
        const auto& bmp=*change.thumbnail_bmp;
        if(bmp.size()<54||bmp[0]!='B'||bmp[1]!='M')throw std::runtime_error("replacement original thumbnail must be a BMP");
        const auto file_size=u32(bmp,2),pixels=u32(bmp,10),header=u32(bmp,14);
        if(file_size<54||file_size>bmp.size()||header<40||uint64_t(header)+14>pixels||pixels>file_size)
            throw std::runtime_error("invalid replacement original thumbnail BMP bounds");
        const uint16_t planes=uint16_t(bmp[26]|unsigned(bmp[27])<<8),bits=uint16_t(bmp[28]|unsigned(bmp[29])<<8);
        const auto width=u32(bmp,18),raw_height=u32(bmp,22);
        if(planes!=1||!width||width>uint32_t(INT32_MAX)||!raw_height||
           (bits!=1&&bits!=4&&bits!=8&&bits!=16&&bits!=24&&bits!=32))
            throw std::runtime_error("invalid replacement original thumbnail BMP geometry");
        if(u32(bmp,30)==0){
            const uint64_t height=(raw_height&0x80000000U)?uint32_t(0U-raw_height):raw_height;
            const uint64_t stride=(uint64_t(width)*bits+31)/32*4;
            if(stride*height>uint64_t(file_size)-pixels)throw std::runtime_error("truncated replacement original thumbnail BMP pixels");
        }
    }
    Bytes output=bytes;
    auto append = [&](const Bytes& payload,uint32_t descriptor) {
        if(payload.size()>maximum||output.size()>maximum-payload.size())throw std::runtime_error("rewritten original save exceeds 128 MiB");
        const uint32_t offset=uint32_t(output.size()),size=uint32_t(payload.size());
        output.insert(output.end(),payload.begin(),payload.end());
        for(unsigned i=0;i<4;++i){output[descriptor+i]=uint8_t(offset>>(i*8));output[descriptor+4+i]=uint8_t(size>>(i*8));}
    };
    if(change.comment1)append(comment1,0x10);
    if(change.comment2)append(comment2,0x18);
    if(change.thumbnail_bmp)append(*change.thumbnail_bmp,0x30);
    inspect_original_save(output);
    return output;
}
OriginalSaveState decode_original_vm(const Bytes& bytes, uint32_t base, const char* encoding) {
    auto word = [&](uint64_t offset) { return u32(bytes, size_t(uint64_t(base) + offset)); };
    auto signed_word = [&](uint64_t offset) {
        const auto raw = word(offset); int32_t result;
        std::memcpy(&result, &raw, sizeof(result)); return result;
    };
    auto region = [&](uint64_t offset, uint64_t length) {
        require_range(bytes.size(), uint64_t(base) + offset, length, "original VM region");
    };
    auto string = [&](uint32_t offset, uint32_t length) {
        if (!length) return std::string{};
        if (length > 16 * 1024 * 1024) throw std::runtime_error("original save string exceeds limit");
        region(offset, length);
        const auto at = size_t(base) + offset;
        const auto end = std::find(bytes.begin() + at, bytes.begin() + at + length, uint8_t{0});
        if (end == bytes.begin() + at + length) throw std::runtime_error("unterminated original save string");
        return decode(std::string(bytes.begin() + at, end), encoding);
    };
    region(0, 72);
    if (std::memcmp(bytes.data() + base, "Yuka  \0\0", 8) || word(8) != 72)
        throw std::runtime_error("unsupported original save container");
    const auto state = word(0x20), state_size = word(0x24);
    region(state, state_size);
    if (state_size < 0x160 || word(state) != 0x160) throw std::runtime_error("unsupported original VM state header");
    const auto flags = word(state + 0x14), flag_size = word(state + 0x18);
    if (flag_size != 0x40000) throw std::runtime_error("unsupported original Flag bank size");
    region(flags, flag_size);
    OriginalSaveState result;
    result.state_header.assign(bytes.begin() + base + state, bytes.begin() + base + state + 0x160);
    result.flags.reserve(65536);
    for (uint32_t i = 0; i < 65536; ++i) result.flags.push_back(signed_word(uint64_t(flags) + i*4));
    const auto strings = word(state + 0x110), string_size = word(state + 0x114);
    if (string_size != 0x80000) throw std::runtime_error("unsupported original String table size");
    region(strings, string_size);
    for (uint32_t i = 0; i < 65536; ++i) {
        const auto offset = word(uint64_t(strings) + i*8), length = word(uint64_t(strings) + i*8 + 4);
        if (length) result.strings.emplace(int32_t(i), string(offset, length));
    }
    uint32_t frame = word(state + 0x2c), count = word(state + 0x30);
    if (count > 256) throw std::runtime_error("original save frame depth exceeds limit");
    for (uint32_t i = 0; i < count; ++i) {
        region(frame, 0x50);
        if (word(frame) != 0x50) throw std::runtime_error("unsupported original script frame header");
        const auto next_frame = word(frame + 4);
        if (next_frame <= frame + 0x4f) throw std::runtime_error("invalid original script frame chain");
        region(frame, uint64_t(next_frame) - frame);
        OriginalFrame saved;
        saved.header.assign(bytes.begin() + base + frame, bytes.begin() + base + frame + 0x50);
        saved.name = string(word(frame + 8), word(frame + 0x0c));
        if (saved.name.empty()) throw std::runtime_error("original script frame has no source name");
        saved.current_word = word(frame + 0x28); saved.next_word = word(frame + 0x2c);
        saved.target_index = word(frame + 0x30); saved.target_active = word(frame + 0x34) != 0;
        saved.mode = word(frame + 0x38);
        const auto local_offset = word(frame + 0x40), local_count = word(frame + 0x44);
        if (local_count > 65536) throw std::runtime_error("original local table exceeds limit");
        region(local_offset, uint64_t(local_count) * 12);
        for (uint32_t n = 0; n < local_count; ++n) {
            const uint64_t at = uint64_t(local_offset) + n*12;
            switch (word(at)) {
            case 0: saved.locals.emplace_back(); break;
            case 1: saved.locals.emplace_back(signed_word(at + 4)); break;
            case 2: saved.locals.emplace_back(string(word(at + 4), word(at + 8))); break;
            default: throw std::runtime_error("unsupported original local value tag");
            }
        }
        const auto code = word(frame + 0x10), code_count = word(frame + 0x14);
        const auto index = word(frame + 0x18), index_count = word(frame + 0x1c);
        const auto data = word(frame + 0x20), data_size = word(frame + 0x24);
        saved.embedded_script = code_count || index_count || data_size;
        if (saved.embedded_script) {
            if (code_count > 4000000 || index_count > 1000000 || data_size > maximum)
                throw std::runtime_error("original embedded script exceeds limits");
            region(code, uint64_t(code_count) * 4); region(index, uint64_t(index_count) * 16); region(data, data_size);
            const uint64_t index_at = 48ULL + uint64_t(code_count)*4;
            const uint64_t data_at = index_at + uint64_t(index_count)*16;
            if (data_at + data_size > maximum) throw std::runtime_error("original embedded script exceeds 128 MiB");
            Bytes script(size_t(data_at + data_size), 0);
            std::memcpy(script.data(), "YKS001", 6);
            auto set = [&](size_t at, uint32_t value) {
                for (unsigned n = 0; n < 4; ++n) script[at+n] = uint8_t(value >> (n*8));
            };
            set(8,48); set(16,48); set(20,code_count); set(24,uint32_t(index_at)); set(28,index_count);
            set(32,uint32_t(data_at)); set(36,data_size); set(40,local_count);
            std::copy_n(bytes.begin() + size_t(base) + code, size_t(code_count)*4, script.begin() + 48);
            std::copy_n(bytes.begin() + size_t(base) + index, size_t(index_count)*16, script.begin() + size_t(index_at));
            std::copy_n(bytes.begin() + size_t(base) + data, data_size, script.begin() + size_t(data_at));
            for (uint32_t n = 0; n < index_count; ++n) {
                const auto at = size_t(index_at) + size_t(n)*16;
                if (u32(script, at) == 11 || u32(script, at) == 12) set(at, 10);
            }
            // The original may contain runtime-local tags 11/12. Text encoding
            // is explicit: existing user saves may have been written by a
            // localized program even though only the original EXE is analyzed.
            Script parsed;
            parsed.max_locals = local_count;
            parsed.data.assign(script.begin() + size_t(data_at), script.end());
            auto text = [&](uint32_t at) {
                require_range(parsed.data.size(), at, 1, "original script string");
                const auto end = std::find(parsed.data.begin() + at, parsed.data.end(), uint8_t{0});
                if (end == parsed.data.end()) throw std::runtime_error("unterminated original script string");
                return decode(std::string(parsed.data.begin() + at, end), encoding);
            };
            for (uint32_t n = 0; n < index_count; ++n) {
                const auto at = size_t(index_at) + size_t(n)*16;
                Element e{u32(script,at),u32(script,at+4),u32(script,at+8),u32(script,at+12),{},0};
                switch(e.type) {
                case 0: case 1: case 7: case 8: case 9: e.text = text(e.a); break;
                case 5: e.text = text(e.b); break;
                case 4: case 10: break;
                default: throw std::runtime_error("unobserved original script element type " + std::to_string(e.type));
                }
                if (e.type == 1 || e.type == 4 || e.type == 8 || e.type == 9) {
                    const auto raw = u32(parsed.data, (e.type == 8 || e.type == 9) ? e.c : e.b);
                    std::memcpy(&e.value, &raw, sizeof(raw));
                }
                parsed.elements.push_back(std::move(e));
            }
            for (uint32_t n = 0; n < code_count;) {
                const auto start = n; const auto element = u32(script, 48 + size_t(n++)*4);
                if (element >= index_count) throw std::runtime_error("original instruction index out of range");
                Instruction instruction{start,element,{}};
                if (parsed.elements[element].type == 0) {
                    if (n == code_count) throw std::runtime_error("missing original call argument count");
                    const auto count = u32(script,48 + size_t(n++)*4);
                    if (count > code_count-n) throw std::runtime_error("invalid original call argument count");
                    for (uint32_t arg = 0; arg < count; ++arg) {
                        const auto value = u32(script,48 + size_t(n++)*4);
                        if (value >= index_count) throw std::runtime_error("original call argument out of range");
                        instruction.arguments.push_back(value);
                    }
                }
                parsed.instructions.push_back(std::move(instruction));
            }
            saved.script = std::move(parsed);
        }
        result.frames.push_back(std::move(saved));
        frame = next_frame;
    }
    return result;
}
Bytes encode_original_save(const OriginalSaveState& state, const OriginalSaveWriteOptions& options) {
    if (state.flags.size() != 65536 || state.frames.size() > 256)
        throw std::runtime_error("invalid original VM bank/frame counts");
    Bytes output = options.template_bytes;
    if (output.empty()) output.resize(72);
    else inspect_original_save(output);
    if (output.size() > maximum) throw std::runtime_error("original save template exceeds 128 MiB");
    auto patch = [&](uint64_t offset, uint32_t n) {
        require_range(output.size(), offset, 4, "original save output field");
        for (unsigned i = 0; i < 4; ++i) output[size_t(offset)+i] = uint8_t(n >> (i*8));
    };
    auto reserve = [&](size_t size) {
        if (size > maximum || output.size() > maximum - size) throw std::runtime_error("original save exceeds 128 MiB");
        const auto at = uint32_t(output.size()); output.resize(output.size() + size, 0); return at;
    };
    auto append = [&](const Bytes& data) {
        const auto at = reserve(data.size()); std::copy(data.begin(), data.end(), output.begin() + at); return at;
    };
    auto text = [&](const std::string& value) {
        auto data = encoded(value, options.encoding); data.push_back(0);
        return std::make_pair(append(Bytes(data.begin(), data.end())), uint32_t(data.size()));
    };
    auto pair = [&](uint64_t at, uint32_t offset, uint32_t size) { patch(at, offset); patch(at+4, size); };
    std::memcpy(output.data(), "Yuka  \0\0", 8); patch(8,72); patch(12,0);
    const auto comment1 = text(options.comment1), comment2 = text(options.comment2), title = text(options.title);
    pair(0x10,comment1.first,comment1.second); pair(0x18,comment2.first,comment2.second); pair(0x40,title.first,title.second);
    Bytes thumbnail = options.thumbnail_bmp;
    if (thumbnail.empty()) {
        // The original game's configured 126x70, 24-bit BMP thumbnail layout.
        // Runtime callers should supply an actual scene thumbnail when possible.
        thumbnail.assign(26654, 0); thumbnail[0]='B'; thumbnail[1]='M';
        auto field = [&](size_t at, uint32_t value) {
            for (unsigned n = 0; n < 4; ++n) thumbnail[at+n] = uint8_t(value >> (n*8));
        };
        field(2,uint32_t(thumbnail.size())); field(10,54); field(14,40); field(18,126); field(22,70);
        thumbnail[26]=1; thumbnail[28]=24; field(34,26600);
    }
    if (thumbnail.size() < 54 || thumbnail[0] != 'B' || thumbnail[1] != 'M')
        throw std::runtime_error("original thumbnail must be a BMP");
    const auto thumbnail_at = append(thumbnail); pair(0x30, thumbnail_at, uint32_t(thumbnail.size()));
    const auto header = reserve(0x160);
    if (!state.state_header.empty()) {
        if (state.state_header.size() != 0x160) throw std::runtime_error("invalid original state header template");
        std::copy(state.state_header.begin(), state.state_header.end(), output.begin() + header);
    }
    patch(header,0x160); pair(0x20,header,0);
    if (options.template_bytes.empty()) {
        // These additional native strings are not interpreted by this VM codec.
        // Retain their pointers only when their original backing bytes survive.
        for (uint32_t at : {0x4cU,0x124U,0x13cU,0x144U}) pair(header+at,0,0);
        pair(header+0x1c,0,0); pair(header+0x24,0,0);
    }
    const auto flags = reserve(0x40000);
    // The complete bank is already bounds-checked by reserve(). Write the
    // contiguous block directly rather than checking a DWORD 65,536 times.
    auto* flag_bytes=output.data()+flags;
    for(size_t n=0;n<65536;++n){const auto v=uint32_t(state.flags[n]);for(unsigned i=0;i<4;++i)flag_bytes[n*4+i]=uint8_t(v>>(8*i));}
    pair(header+0x14,flags,0x40000);
    std::map<int32_t, std::pair<uint32_t,uint32_t>> strings;
    for (const auto& item : state.strings) {
        if (item.first < 0 || item.first >= 65536) throw std::runtime_error("original String index out of range");
        strings.emplace(item.first, text(item.second));
    }
    const auto string_table = reserve(0x80000);
    for (const auto& item : strings) pair(uint64_t(string_table)+uint32_t(item.first)*8,item.second.first,item.second.second);
    pair(header+0x110,string_table,0x80000);
    const auto frames = uint32_t(output.size());
    for (const auto& frame : state.frames) {
        if (frame.locals.size() > 65536) throw std::runtime_error("original locals exceed limit");
        const auto at = reserve(0x50);
        if (!frame.header.empty()) {
            if (frame.header.size() != 0x50) throw std::runtime_error("invalid original frame header template");
            std::copy(frame.header.begin(), frame.header.end(), output.begin() + at);
        }
        patch(at,0x50);
        const auto name = text(frame.name); pair(at+8,name.first,name.second);
        if (frame.embedded_script) {
            if (frame.script.data.empty()) throw std::runtime_error("original writer requires a native script data section");
            uint64_t word_count=0;
            for(const auto& ins:frame.script.instructions){
                if(ins.word_offset!=word_count||ins.element>=frame.script.elements.size())throw std::runtime_error("invalid native script instruction layout");
                if(frame.script.elements[ins.element].type==0){
                    if(ins.arguments.size()>128)throw std::runtime_error("native argument limit exceeded");
                    for(auto arg:ins.arguments)if(arg>=frame.script.elements.size())throw std::runtime_error("native argument index out of range");
                    word_count+=2+ins.arguments.size();
                }else ++word_count;
            }
            if(word_count>maximum/4)throw std::runtime_error("native code exceeds save limit");
            const auto code=reserve(size_t(word_count)*4);size_t cursor=code;
            auto word=[&](uint32_t n){for(unsigned i=0;i<4;++i)output[cursor++]=uint8_t(n>>(i*8));};
            for(const auto& ins:frame.script.instructions){word(ins.element);if(frame.script.elements[ins.element].type==0){word(uint32_t(ins.arguments.size()));for(auto arg:ins.arguments)word(arg);}}
            pair(at+0x10,code,uint32_t(word_count));
            if(frame.script.elements.size()>maximum/16)throw std::runtime_error("native element index exceeds save limit");
            const auto index=reserve(frame.script.elements.size()*16);size_t element_at=index;
            for (const auto& element : frame.script.elements) {
                uint32_t type = element.type;
                if (type == 10) {
                    if (element.b >= frame.locals.size()) throw std::runtime_error("native local index out of range");
                    if (std::holds_alternative<int32_t>(frame.locals[element.b])) type = 11;
                    else if (std::holds_alternative<std::string>(frame.locals[element.b])) type = 12;
                }
                const auto e=element_at;element_at+=16;
                const uint32_t words[4]={type,element.a,element.b,type==0?0:element.c};
                for(unsigned n=0;n<4;++n)for(unsigned i=0;i<4;++i)output[e+n*4+i]=uint8_t(words[n]>>(i*8));
            }
            pair(at+0x18,index,uint32_t(frame.script.elements.size()));
            const auto data = append(frame.script.data); pair(at+0x20,data,uint32_t(frame.script.data.size()));
        } else {
            pair(at+0x10,0,0); pair(at+0x18,0,0); pair(at+0x20,0,0);
        }
        std::vector<std::pair<uint32_t,uint32_t>> local_strings(frame.locals.size());
        for (size_t n = 0; n < frame.locals.size(); ++n)
            if (auto string = std::get_if<std::string>(&frame.locals[n])) local_strings[n] = text(*string);
        const auto locals = reserve(frame.locals.size()*12);
        for (size_t n = 0; n < frame.locals.size(); ++n) {
            const auto entry = uint64_t(locals)+n*12;
            if (auto value = std::get_if<int32_t>(&frame.locals[n])) { patch(entry,1); patch(entry+4,uint32_t(*value)); }
            else if (std::holds_alternative<std::string>(frame.locals[n])) {
                patch(entry,2); pair(entry+4,local_strings[n].first,local_strings[n].second);
            }
        }
        pair(at+0x40,locals,uint32_t(frame.locals.size()));
        patch(at+0x28,frame.current_word); patch(at+0x2c,frame.next_word);
        patch(at+0x30,frame.target_index); patch(at+0x34,frame.target_active ? 1 : 0); patch(at+0x38,frame.mode);
        patch(at+4,uint32_t(output.size()));
    }
    pair(header+0x2c,frames,uint32_t(state.frames.size()));
    if (options.append_scene) options.append_scene(output,header);
    if (output.size() > maximum || output.size() < header + 0x160) throw std::runtime_error("invalid appended original scene");
    patch(0x24,uint32_t(output.size())-header);
    const auto history = append(options.history); pair(0x28,history,uint32_t(options.history.size()));
    const auto previous = append(options.previous); pair(0x38,previous,uint32_t(options.previous.size()));
    inspect_original_save(output);
    return output;
}
}

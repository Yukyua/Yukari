// C++ implementation informed by YukaTool2 (AtomCrafty, GPL-3.0).
// Modified implementation: 2026-10-31. See docs/evidence.md and LICENSE.
#include "yukari/formats.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iconv.h>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <stdexcept>

namespace yukari {
namespace {
constexpr uint64_t max_blob = 512ULL * 1024 * 1024;
std::string resource_key(std::string name) {
    // Original archive reader uses __stricmp (0x42a190). Match ASCII case
    // without applying locale-dependent transformations to UTF-8 bytes.
    for (char& c : name) if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    return name;
}
Bytes read_region(std::ifstream& f, uint64_t total, uint64_t at, uint64_t size) {
    require_range(total, at, size, "file region");
    if (size > max_blob) throw std::runtime_error("resource exceeds 512 MiB limit");
    Bytes b(static_cast<size_t>(size));
    f.seekg(static_cast<std::streamoff>(at));
    if (!f || (size && !f.read(reinterpret_cast<char*>(b.data()), static_cast<std::streamsize>(size))))
        throw std::runtime_error("short read");
    return b;
}
bool magic(const Bytes& b, const char* s, size_t n) {
    return b.size() >= n && std::memcmp(b.data(), s, n) == 0;
}
Bytes slice(const Bytes& b, uint64_t at, uint64_t size) {
    require_range(b.size(), at, size, "section");
    return Bytes(b.begin() + static_cast<size_t>(at), b.begin() + static_cast<size_t>(at + size));
}
std::string cstring(const Bytes& b, uint32_t offset) {
    require_range(b.size(), offset, 1, "string offset");
    auto begin = b.begin() + offset;
    auto end = std::find(begin, b.end(), 0);
    if (end == b.end()) throw std::runtime_error("unterminated string");
    return decode(std::string(begin, end));
}
std::string escaped(const std::string& text) {
    std::ostringstream out;
    out << '"';
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 32 || c == 127) out << "\\x" << std::hex << std::setw(2) << std::setfill('0') << unsigned(c) << std::dec;
        else out << c;
    }
    out << '"';
    return out.str();
}
std::string element_text(const Element& e) {
    switch (e.type) {
    case 0: return escaped(e.text);
    case 1: return ":" + escaped(e.text) + " link=" + std::to_string(e.value);
    case 4: return std::to_string(e.value);
    case 5: return escaped(e.text);
    case 7: return escaped(e.text);
    case 8: case 9: return escaped(e.text) + ":" + std::to_string(e.value);
    case 10: return "$" + std::to_string(e.b);
    default: throw std::runtime_error("unsupported element");
    }
}
}
void require_range(uint64_t size, uint64_t at, uint64_t length, const char* label) {
    if (at > size || length > size - at) throw std::runtime_error(std::string(label) + ": out of bounds");
}
uint32_t u32(const Bytes& b, size_t at) {
    require_range(b.size(), at, 4, "u32");
    return uint32_t(b[at]) | (uint32_t(b[at+1]) << 8) | (uint32_t(b[at+2]) << 16) | (uint32_t(b[at+3]) << 24);
}
Bytes read_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path.string());
    return read_region(f, std::filesystem::file_size(path), 0, std::filesystem::file_size(path));
}
void write_file(const std::filesystem::path& path, const Bytes& b) {
    // Extraction is explicit and never silently overwrites existing files.
    if (std::filesystem::exists(path)) throw std::runtime_error("output already exists: " + path.string());
    std::ofstream f(path, std::ios::binary);
    if (!f || !f.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size())))
        throw std::runtime_error("cannot write " + path.string());
}
std::string decode(const std::string& input, const char* encoding) {
    iconv_t cd = iconv_open("UTF-8", encoding);
    if (cd == reinterpret_cast<iconv_t>(-1)) throw std::runtime_error("encoding unavailable");
    std::string output(input.size() * 4 + 4, '\0');
    char* src = const_cast<char*>(input.data());
    char* dst = output.data();
    size_t remaining = input.size(), space = output.size();
    size_t result = iconv(cd, &src, &remaining, &dst, &space);
    iconv_close(cd);
    if (result == size_t(-1)) throw std::runtime_error(std::string("invalid ") + encoding + " text");
    output.resize(output.size() - space);
    return output;
}
Archive::Archive(const std::filesystem::path& path) : path_(path) {
    source_=ResourceSource::file(path);
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path.string());
    file_size_ = std::filesystem::file_size(path);
    auto h = read_region(f, file_size_, 0, 24);
    if (!magic(h, "YKC001\0\0", 8) && !magic(h, "YKC002\0\0", 8))
        throw std::runtime_error("unsupported YKC signature/version");
    const auto at = u32(h, 16), length = u32(h, 20);
    if (length % 20 || length / 20 > 1000000) throw std::runtime_error("invalid YKC index length");
    auto index = read_region(f, file_size_, at, length);
    for (size_t i = 0; i < index.size(); i += 20) {
        const auto name_length = u32(index, i+4);
        if (!name_length || name_length > 65536) throw std::runtime_error("invalid entry name length");
        auto name = read_region(f, file_size_, u32(index, i), name_length);
        auto end = std::find(name.begin(), name.end(), 0);
        if (end == name.end()) throw std::runtime_error("unterminated archive name");
        Entry e{decode(std::string(name.begin(), end), h[5] == '1' ? "CP932" : "UTF-8"),
                u32(index, i+8), u32(index, i+12), u32(index, i+16)};
        require_range(file_size_, e.offset, e.size, "entry data");
        // Original reader scans entries from the start; first match wins.
        lookup_.emplace(resource_key(e.name), entries_.size());
        entries_.push_back(std::move(e));
    }
}
ResourceSource Archive::open(const std::string& name)const{
    const auto it=lookup_.find(resource_key(name));if(it==lookup_.end())throw std::runtime_error("entry not found: "+name);const auto& entry=entries_[it->second];return source_.slice(entry.offset,entry.size);
}
Bytes Archive::read(const std::string& name) const {return open(name).read_all();}
bool Archive::contains(const std::string& name) const {
    return lookup_.find(resource_key(name)) != lookup_.end();
}
Resources::Resources(const std::filesystem::path& directory) {
    if (!std::filesystem::is_directory(directory)) throw std::runtime_error("game directory not found");
    // 0x42a2f0 scans 0x4bc228; 0x429400 initializes these names.
    for (const char* name : {"data04.ykc", "data01.ykc", "data02.ykc", "data03.ykc", "data05.ykc",
                             "data06.ykc", "data07.ykc", "data08.ykc", "data09.ykc"}) {
        const auto path = directory / name;
        if (std::filesystem::exists(path)) archives_.emplace_back(path);
    }
    if (archives_.empty()) throw std::runtime_error("no original dataXX.ykc archives found");
}
std::vector<ResourceEntry> Resources::entries() const {
    std::vector<ResourceEntry> result;std::unordered_set<std::string> seen;
    for(const auto& archive:archives_)for(const auto& entry:archive.entries())
        if(seen.insert(resource_key(entry.name)).second)result.push_back({archive.path().filename().string(),entry});
    return result;
}
ResourceSource Resources::open(const std::string& name)const{
    for(const auto& archive:archives_)if(archive.contains(name))return archive.open(name);
    throw std::runtime_error("resource not found in original archives: "+name);
}
bool Resources::contains(const std::string& name)const{for(const auto& archive:archives_)if(archive.contains(name))return true;return false;}
Bytes Resources::read(const std::string& name) const {
    return open(name).read_all();
}
std::vector<Frame> parse_graphic_frames(const Bytes& b){
    require_range(b.size(),0,64,"YKG header");if(!magic(b,"YKG000\0\0",8))throw std::runtime_error("unsupported YKG version/encryption");
    auto offset=u32(b,0x28),length=u32(b,0x2c);if(!offset)offset=u32(b,8);if(offset<0x30||offset>b.size())throw std::runtime_error("invalid YKG color offset");if(!length)length=uint32_t(b.size()-offset);require_range(b.size(),offset,length,"YKG color");if(u32(b,0x34))require_range(b.size(),u32(b,0x30),u32(b,0x34),"YKG alpha");
    std::vector<Frame> frames;
    const auto frames_at = u32(b, 0x38), frames_size = u32(b, 0x3c);
    require_range(b.size(), frames_at, frames_size, "YKG frames");
    if (frames_size % 32) throw std::runtime_error("invalid YKG frame section length");
    for (uint64_t i = frames_at; i < uint64_t(frames_at) + frames_size; i += 32)
        frames.push_back({u32(b,i), u32(b,i+4), u32(b,i+8), u32(b,i+12), u32(b,i+16), u32(b,i+20), u32(b,i+24), u32(b,i+28)});
    return frames;
}
Graphic parse_graphic(const Bytes& b) {
    require_range(b.size(), 0, 64, "YKG header");
    if (!magic(b, "YKG000\0\0", 8)) throw std::runtime_error("unsupported YKG version/encryption");
    auto offset = u32(b, 0x28), length = u32(b, 0x2c);
    if (!offset) offset = u32(b, 8);
    if (offset < 0x30 || offset > b.size()) throw std::runtime_error("invalid YKG color offset");
    if (!length) length = static_cast<uint32_t>(b.size() - offset);
    Graphic g{slice(b, offset, length), {}, {}};
    if (u32(b, 0x34)) g.alpha = slice(b, u32(b, 0x30), u32(b, 0x34));
    const auto frames_at = u32(b, 0x38), frames_size = u32(b, 0x3c);
    require_range(b.size(), frames_at, frames_size, "YKG frames");
    if (frames_size % 32) throw std::runtime_error("invalid YKG frame section length");
    for (uint64_t i = frames_at; i < uint64_t(frames_at) + frames_size; i += 32)
        g.frames.push_back({u32(b,i), u32(b,i+4), u32(b,i+8), u32(b,i+12), u32(b,i+16), u32(b,i+20), u32(b,i+24), u32(b,i+28)});
    return g;
}
Script parse_script(const Bytes& b) {
    require_range(b.size(), 0, 48, "YKS header");
    if (!magic(b, "YKS001", 6) || b[7] || b[6] > 1 || u32(b,8) != 48)
        throw std::runtime_error("unsupported YKS header/version/encryption");
    uint32_t code_at = u32(b,16), code_count = u32(b,20), index_at = u32(b,24), index_count = u32(b,28);
    uint32_t data_at = u32(b,32), data_size = u32(b,36);
    require_range(b.size(), code_at, uint64_t(code_count)*4, "YKS code");
    require_range(b.size(), index_at, uint64_t(index_count)*16, "YKS index");
    if (index_count > 1000000 || code_count > 4000000) throw std::runtime_error("YKS table limit exceeded");
    auto data = slice(b, data_at, data_size);
    if (b[6]) for (auto& v : data) v ^= 0xaa;
    Script s;
    s.encrypted = b[6] != 0;
    s.max_locals = u32(b,40);
    for (uint32_t i = 0; i < index_count; ++i) {
        size_t at = size_t(index_at) + size_t(i)*16;
        Element e{u32(b,at), u32(b,at+4), u32(b,at+8), u32(b,at+12), {}, 0};
        switch (e.type) {
        case 0: case 1: case 7: case 8: case 9: e.text = cstring(data, e.a); break;
        case 4: case 5: case 10: break;
        default: throw std::runtime_error("unsupported YKS element type " + std::to_string(e.type) + " at index " + std::to_string(i));
        }
        if (e.type == 5) e.text = cstring(data, e.b);
        if (e.type == 1 || e.type == 4 || e.type == 8 || e.type == 9) {
            uint32_t raw = u32(data, (e.type == 8 || e.type == 9) ? e.c : e.b);
            std::memcpy(&e.value, &raw, sizeof(raw));
        }
        s.elements.push_back(std::move(e));
    }
    auto word = [&](uint32_t i) { return u32(b, size_t(code_at) + size_t(i)*4); };
    for (uint32_t i = 0; i < code_count;) {
        Instruction ins{i, word(i), {}};
        ++i;
        if (ins.element >= index_count) throw std::runtime_error("YKS instruction index out of bounds");
        auto type = s.elements[ins.element].type;
        if (type == 0) {
            if (i == code_count) throw std::runtime_error("missing YKS argument count");
            auto count = word(i++);
            if (count > code_count-i) throw std::runtime_error("truncated YKS argument list");
            for (uint32_t j = 0; j < count; ++j) {
                auto arg = word(i++);
                if (arg >= index_count) throw std::runtime_error("YKS argument index out of bounds");
                ins.arguments.push_back(arg);
            }
        } else if (type == 5) throw std::runtime_error("unsupported YKS string constant assignment");
        s.instructions.push_back(std::move(ins));
    }
    s.data = std::move(data);
    return s;
}
void disassemble(const Script& s, std::ostream& out) {
    out << "# YKS001 encrypted=" << s.encrypted << " locals=" << s.max_locals << " elements=" << s.elements.size() << '\n';
    for (const auto& ins : s.instructions) {
        const auto& e = s.elements[ins.element];
        out << std::setw(8) << std::setfill('0') << std::hex << ins.word_offset << std::dec << "  ";
        if (e.type == 0) {
            out << "call " << element_text(e) << '(';
            for (size_t i = 0; i < ins.arguments.size(); ++i) {
                if (i) out << ", ";
                out << element_text(s.elements[ins.arguments[i]]);
            }
            out << ')';
        } else out << (e.type == 1 ? "label " : "target ") << element_text(e);
        out << "  # index=" << ins.element << '\n';
    }
}
}

#pragma once
#include <cstdint>
#include "yukari/resource_source.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace yukari {
using Bytes = std::vector<uint8_t>;
Bytes read_file(const std::filesystem::path& path);
void write_file(const std::filesystem::path& path, const Bytes& bytes);
void require_range(uint64_t size, uint64_t offset, uint64_t length, const char* label);
uint32_t u32(const Bytes& b, size_t at);
std::string decode(const std::string& bytes, const char* encoding = "CP932");
struct Entry {
    std::string name;
    uint32_t offset, size, reserved;
};
// Keeps only the index in memory; payloads are read on demand.
class Archive {
public:
    explicit Archive(const std::filesystem::path& path);
    const std::vector<Entry>& entries() const { return entries_; }
    const std::filesystem::path& path() const { return path_; }
    bool contains(const std::string& name) const;
    Bytes read(const std::string& name) const;
    ResourceSource open(const std::string& name) const;
private:
    std::filesystem::path path_;
    ResourceSource source_;
    uint64_t file_size_{};
    std::vector<Entry> entries_;
    std::unordered_map<std::string, size_t> lookup_;
};
struct ResourceEntry { std::string archive; Entry entry; };
// Original reference game profile: archive order observed in its executable.
// Only original .ykc packages are mounted; no translation patch inputs.
class Resources {
public:
    explicit Resources(const std::filesystem::path& directory);
    // Effective entries, with the same case-insensitive precedence as open().
    std::vector<ResourceEntry> entries() const;
    bool contains(const std::string& name)const;
    Bytes read(const std::string& name) const;
    ResourceSource open(const std::string& name) const;
private:
    std::vector<Archive> archives_;
};
struct Frame { uint32_t x, y, w, h, duration, type, unknown, action; };
struct Graphic {
    Bytes color, alpha;
    std::vector<Frame> frames;
};
Graphic parse_graphic(const Bytes& bytes);
std::vector<Frame> parse_graphic_frames(const Bytes& bytes);
struct Element {
    uint32_t type, a, b, c;
    std::string text;
    int32_t value{};
};
struct Instruction {
    uint32_t word_offset, element;
    std::vector<uint32_t> arguments;
};
struct Script {
    uint32_t max_locals{};
    bool encrypted{};
    std::vector<Element> elements;
    std::vector<Instruction> instructions;
    // Decrypted data bytes. Each VM context owns its mutable copy: integer
    // constants, control links and variable IDs can reference the same bytes.
    Bytes data;
};
Script parse_script(const Bytes& bytes);
void disassemble(const Script& script, std::ostream& out);
// Color-plane preview, not an implementation of engine animation/compositing.
void preview(const Bytes& bytes, const std::string& title, unsigned frame_limit);
}

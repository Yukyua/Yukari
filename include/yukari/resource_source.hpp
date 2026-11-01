#pragma once
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>
namespace yukari {
struct ResourceIoStats {uint64_t calls{},bytes{};};
// Immutable seekable byte range. Independent cursors share a read-only descriptor;
// pread never changes another decoder's position and cannot cross the range.
class ResourceSource {
public:
    ResourceSource()=default;
    static ResourceSource file(const std::filesystem::path& path);
    static ResourceSource memory(std::shared_ptr<const std::vector<uint8_t>> bytes);
    ResourceSource slice(uint64_t offset,uint64_t length)const;
    uint64_t size()const{return length_;}
    size_t read_at(uint64_t offset,void* output,size_t count)const;
    std::vector<uint8_t> read_all()const;
    ResourceIoStats io_stats()const;
    explicit operator bool()const{return bool(impl_);}
private:
    struct Impl;std::shared_ptr<Impl> impl_;uint64_t offset_{},length_{};
};
}

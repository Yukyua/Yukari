#pragma once
#include "yukari/formats.hpp"
#include <map>

namespace yukari {
// Native Already.dat stores one byte per YKS code DWORD, including parameter
// words. The VM visits instruction starts and reads the old byte before marking.
class AlreadyData {
public:
    AlreadyData()=default;
    AlreadyData(const AlreadyData& other):entries_(other.entries_),active_(other.active_){}
    AlreadyData(AlreadyData&& other) noexcept:entries_(std::move(other.entries_)),active_(std::move(other.active_)){other.cached_bytes_=nullptr;}
    AlreadyData& operator=(const AlreadyData& other){if(this!=&other){cached_bytes_=nullptr;entries_=other.entries_;active_=other.active_;}return *this;}
    AlreadyData& operator=(AlreadyData&& other) noexcept{if(this!=&other){cached_bytes_=other.cached_bytes_=nullptr;entries_=std::move(other.entries_);active_=std::move(other.active_);}return *this;}
    void bind(const std::string& script,uint32_t code_words);
    bool read(const std::string& script,uint32_t word) const;
    bool mark(const std::string& script,uint32_t word);
    bool mark(const std::string& script,uint32_t word,uint32_t code_words);
    const std::map<std::pair<std::string,uint32_t>,Bytes>& entries() const { return entries_; }
private:
    std::map<std::pair<std::string,uint32_t>,Bytes> entries_;
    std::map<std::string,uint32_t> active_;
    // Map nodes remain stable on insertion. Copies/moves and an explicit
    // rebind discard this cursor, so no pointer can address another bank.
    std::string cached_name_;uint32_t cached_count_{};Bytes* cached_bytes_{};
    friend AlreadyData decode_already(const Bytes&,const char*);
};
AlreadyData decode_already(const Bytes& bytes,const char* encoding = "CP932");
Bytes encode_already(const AlreadyData& data,const char* encoding = "CP932");
}

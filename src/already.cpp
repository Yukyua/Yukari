#include "yukari/already.hpp"
#include <cstring>
#include <iconv.h>
#include <limits>
#include <stdexcept>

namespace yukari {
namespace {
constexpr size_t max_bytes=128*1024*1024;
constexpr uint32_t max_entries=65536;
std::string canonical(std::string name) { for(char& c:name)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return name; }
void put(Bytes& b,size_t at,uint32_t value) {
    require_range(b.size(),at,4,"Already.dat output");
    for(unsigned i=0;i<4;++i)b[at+i]=static_cast<uint8_t>(value>>(i*8));
}
std::string encoded_name(const std::string& name,const char* encoding) {
    if(name.empty()||name.size()>1024*1024||name.find('\0')!=std::string::npos)throw std::runtime_error("invalid Already.dat script name");
    auto codec=iconv_open(encoding,"UTF-8");
    if(codec==reinterpret_cast<iconv_t>(-1))throw std::runtime_error("Already.dat name encoding unavailable");
    std::string out(name.size()*2+16,'\0');char* src=const_cast<char*>(name.data());char* dst=out.data();size_t remaining=name.size(),space=out.size();
    auto result=iconv(codec,&src,&remaining,&dst,&space);iconv_close(codec);
    if(result==size_t(-1)||remaining)throw std::runtime_error("Already.dat script name cannot be encoded");
    out.resize(out.size()-space);return out;
}
}
void AlreadyData::bind(const std::string& name,uint32_t count) {
    cached_bytes_=nullptr;
    if(name.empty()||name.size()>1024*1024||name.find('\0')!=std::string::npos)throw std::runtime_error("invalid already script name");
    if(count>max_bytes)throw std::runtime_error("already code word count exceeds limit");
    auto normalized=canonical(name);auto k=std::make_pair(normalized,count);auto it=entries_.find(k);
    if(it!=entries_.end()){active_[normalized]=count;return;}
    if(it==entries_.end()&&entries_.size()>=max_entries)throw std::runtime_error("already script count exceeds limit");
    uint64_t total=count;
    for(const auto& entry:entries_)total+=entry.second.size();
    if(total>max_bytes)throw std::runtime_error("already maps exceed limit");
    // Native 0x4019d0 keys by case-insensitive name AND code word count.
    entries_[k]=Bytes(count,0);active_[normalized]=count;
}
bool AlreadyData::read(const std::string& name,uint32_t word) const {
    auto normalized=canonical(name);auto active=active_.find(normalized);if(active==active_.end())return false;
    auto it=entries_.find({normalized,active->second});return it!=entries_.end()&&word<it->second.size()&&it->second[word]!=0;
}
bool AlreadyData::mark(const std::string& name,uint32_t word) {
    auto normalized=canonical(name);auto active=active_.find(normalized);
    if(active==active_.end())throw std::runtime_error("already script is not bound");
    auto it=entries_.find({normalized,active->second});
    if(it==entries_.end()||word>=it->second.size())throw std::runtime_error("already instruction word out of bounds");
    bool previous=it->second[word]!=0;it->second[word]=1;return previous;
}
bool AlreadyData::mark(const std::string& name,uint32_t word,uint32_t count) {
    if(!cached_bytes_||cached_count_!=count||cached_name_!=name){
        bind(name,count);cached_name_=name;cached_count_=count;cached_bytes_=&entries_.at({canonical(name),count});
    }
    if(word>=cached_bytes_->size())throw std::runtime_error("already instruction word out of bounds");
    const bool previous=(*cached_bytes_)[word]!=0;(*cached_bytes_)[word]=1;return previous;
}
AlreadyData decode_already(const Bytes& b,const char* encoding) {
    require_range(b.size(),0,20,"Already.dat header");
    if(b.size()>max_bytes||std::memcmp(b.data(),"YKALREADY01",11))throw std::runtime_error("invalid Already.dat signature/size");
    const auto count=u32(b,12),table=u32(b,16);
    if(count>max_entries||table<20)throw std::runtime_error("invalid Already.dat index");
    require_range(b.size(),table,uint64_t(count)*16,"Already.dat index");
    AlreadyData out;uint64_t total=0;
    for(uint32_t i=0;i<count;++i) {
        size_t p=size_t(table)+size_t(i)*16;auto name_at=u32(b,p),name_size=u32(b,p+4),size=u32(b,p+8),at=u32(b,p+12);
        if(!name_size||name_size>1024*1024||name_at<20||at<20)throw std::runtime_error("invalid Already.dat record");
        require_range(b.size(),name_at,name_size,"Already.dat script name");require_range(b.size(),at,size,"Already.dat read map");
        auto begin=b.begin()+name_at,end=begin,last=begin+name_size;
        while(end!=last&&*end)++end;
        if(end==last||end==begin)throw std::runtime_error("unterminated/empty Already.dat script name");
        auto name=canonical(decode(std::string(begin,end),encoding));total+=size;
        if(total>max_bytes||!out.entries_.emplace(std::make_pair(name,size),Bytes(b.begin()+at,b.begin()+at+size)).second)throw std::runtime_error("duplicate/oversized Already.dat maps");
        out.active_[name]=size;
    }
    return out;
}
Bytes encode_already(const AlreadyData& data,const char* encoding) {
    Bytes out(20,0),index;
    std::memcpy(out.data(),"YKALREADY01",11);uint32_t count=0;
    for(const auto& entry:data.entries()) {
        if(entry.second.empty())continue;
        const auto name=encoded_name(entry.first.first,encoding);
        if(++count>max_entries||uint64_t(out.size())+name.size()+1+entry.second.size()+uint64_t(count)*16>max_bytes)throw std::runtime_error("Already.dat output exceeds limit");
        const auto name_at=static_cast<uint32_t>(out.size());out.insert(out.end(),name.begin(),name.end());out.push_back(0);
        const auto at=static_cast<uint32_t>(out.size());out.insert(out.end(),entry.second.begin(),entry.second.end());
        auto p=index.size();index.resize(p+16);put(index,p,name_at);put(index,p+4,static_cast<uint32_t>(name.size()+1));put(index,p+8,static_cast<uint32_t>(entry.second.size()));put(index,p+12,at);
    }
    put(out,12,count);put(out,16,static_cast<uint32_t>(out.size()));out.insert(out.end(),index.begin(),index.end());return out;
}
}

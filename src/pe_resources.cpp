#include "yukari/pe_resources.hpp"
#include <algorithm>
#include <stdexcept>
namespace yukari {
namespace {
uint16_t w(const Bytes& b,size_t p){require_range(b.size(),p,2,"PE WORD");return uint16_t(b[p]|unsigned(b[p+1])<<8);}
struct PeResources {
 const Bytes& b;size_t section_table{},base{};uint16_t sections{};
 size_t offset(uint32_t rva,size_t length)const{
  for(size_t i=0;i<sections;++i){size_t p=section_table+i*40;uint32_t start=u32(b,p+12),size=u32(b,p+16);if(rva>=start&&uint64_t(rva)-start+length<=size){size_t at=size_t(u32(b,p+20))+rva-start;require_range(b.size(),at,length,"PE resource bytes");return at;}}
  throw std::runtime_error("PE resource RVA is not backed by file data");
 }
 explicit PeResources(const Bytes& bytes):b(bytes){
  if(w(b,0)!=0x5a4d)throw std::runtime_error("resource source is not a DOS executable");size_t pe=u32(b,0x3c);require_range(b.size(),pe,24,"PE header");if(u32(b,pe)!=0x4550)throw std::runtime_error("invalid PE signature");sections=w(b,pe+6);auto optional=pe+24;auto size=w(b,pe+20);require_range(b.size(),optional,size,"PE optional header");if(w(b,optional)!=0x10b||size<120||u32(b,optional+92)<3)throw std::runtime_error("PE32 resource directory unavailable");section_table=optional+size;require_range(b.size(),section_table,size_t(sections)*40,"PE sections");base=offset(u32(b,optional+112),16);
 }
 uint32_t entry(uint32_t directory,uint16_t id,bool any=false)const{
  size_t p=base+directory;require_range(b.size(),p,16,"PE resource directory");size_t count=size_t(w(b,p+12))+w(b,p+14);require_range(b.size(),p+16,count*8,"PE resource entries");for(size_t i=0;i<count;++i){auto at=p+16+i*8;auto name=u32(b,at);if((any||name==id)&&!(name&0x80000000))return u32(b,at+4);}throw std::runtime_error("PE numeric resource not found");
 }
 Bytes get(uint16_t type,uint16_t id)const{
  auto t=entry(0,type);if(!(t&0x80000000))throw std::runtime_error("invalid PE resource type directory");auto n=entry(t&0x7fffffff,id);if(!(n&0x80000000))throw std::runtime_error("invalid PE resource name directory");auto language=entry(n&0x7fffffff,0,true);if(language&0x80000000)throw std::runtime_error("invalid PE resource language leaf");auto data=base+language;require_range(b.size(),data,16,"PE resource data entry");auto length=u32(b,data+4);auto p=offset(u32(b,data),length);return Bytes(b.begin()+p,b.begin()+p+length);
 }
};
void put16(Bytes& b,size_t p,uint16_t v){b[p]=uint8_t(v);b[p+1]=uint8_t(v>>8);}
void put32(Bytes& b,size_t p,uint32_t v){for(int i=0;i<4;++i)b[p+i]=uint8_t(v>>(i*8));}
}
Bytes pe_resource(const Bytes& executable,uint16_t type,uint16_t id){return PeResources(executable).get(type,id);}
Bytes pe_icon(const Bytes& executable,uint16_t id){
 PeResources r(executable);auto group=r.get(14,id);require_range(group.size(),0,6,"PE icon group");if(w(group,0)||w(group,2)!=1)throw std::runtime_error("invalid PE icon group");auto count=w(group,4);if(!count||count>256)throw std::runtime_error("invalid PE icon count");require_range(group.size(),6,size_t(count)*14,"PE icon entries");Bytes out(6+size_t(count)*16);put16(out,2,1);put16(out,4,count);
 for(size_t i=0;i<count;++i){auto g=6+i*14,p=6+i*16;auto data=r.get(3,w(group,g+12));std::copy_n(group.begin()+g,8,out.begin()+p);put32(out,p+8,uint32_t(data.size()));if(out.size()>UINT32_MAX-data.size())throw std::runtime_error("PE icon exceeds size limit");put32(out,p+12,uint32_t(out.size()));out.insert(out.end(),data.begin(),data.end());}return out;
}
}

#include "yukari/cd_media.hpp"
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOCDMedia.h>
#include <CoreFoundation/CoreFoundation.h>
#include <algorithm>
#include <map>
#include <stdexcept>
namespace yukari {
std::vector<CdTrack> decode_cd_toc(const Bytes& bytes){
 if(bytes.size()<4)throw std::runtime_error("truncated CD table of contents");
 const auto size=size_t((unsigned(bytes[0])<<8)|bytes[1])+2;
 if(size<4||size>bytes.size()||(size-4)%11)throw std::runtime_error("invalid CD table of contents size");
 struct Entry{unsigned session,number,sector;bool audio;};std::vector<Entry> entries;std::map<unsigned,unsigned> ends;
 auto sector=[&](size_t p){if(bytes[p+1]>=60||bytes[p+2]>=75)throw std::runtime_error("invalid CD TOC time");unsigned frames=(unsigned(bytes[p])*60+bytes[p+1])*75+bytes[p+2];return frames>=150?frames-150:0;};
 for(size_t p=4;p<size;p+=11){const unsigned adr=bytes[p+1]>>4,point=bytes[p+3];if(adr!=1)continue;if(point==0xa2){ends[bytes[p]]=sector(p+8);continue;}if(point<1||point>99)continue;entries.push_back({bytes[p],point,sector(p+8),(bytes[p+1]&4)==0});}
 std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.number<b.number;});std::vector<CdTrack> result;
 for(size_t i=0;i<entries.size();++i){const auto& e=entries[i];if(i&&entries[i-1].number==e.number)throw std::runtime_error("duplicate CD track number");unsigned end;if(i+1<entries.size()&&entries[i+1].session==e.session)end=entries[i+1].sector;else{auto found=ends.find(e.session);if(found==ends.end())throw std::runtime_error("CD TOC session lacks lead-out");end=found->second;}if(end<e.sector)throw std::runtime_error("CD track ends before its start");result.push_back({e.number,e.sector,end,e.audio});}
 return result;
}
std::vector<CdMedia> enumerate_cd_media(){
 io_iterator_t iterator=IO_OBJECT_NULL;const auto status=IOServiceGetMatchingServices(kIOMainPortDefault,IOServiceMatching(kIOCDMediaClass),&iterator);if(status!=KERN_SUCCESS)throw std::runtime_error("cannot enumerate CD media (IOKit "+std::to_string(status)+")");
 struct Iterator{io_iterator_t object;~Iterator(){if(object)IOObjectRelease(object);}} cleanup{iterator};std::vector<CdMedia> result;
 for(io_service_t object=IOIteratorNext(iterator);object;object=IOIteratorNext(iterator)){
  struct Service{io_service_t object;~Service(){IOObjectRelease(object);}} service{object};
  auto name=IORegistryEntryCreateCFProperty(object,CFSTR(kIOBSDNameKey),kCFAllocatorDefault,0);auto toc=IORegistryEntryCreateCFProperty(object,CFSTR(kIOCDMediaTOCKey),kCFAllocatorDefault,0);
  struct Values{CFTypeRef name,toc;~Values(){if(name)CFRelease(name);if(toc)CFRelease(toc);}} values{name,toc};
  if(!name||CFGetTypeID(name)!=CFStringGetTypeID()||!toc||CFGetTypeID(toc)!=CFDataGetTypeID())continue;
  char bsd[1024];if(!CFStringGetCString(static_cast<CFStringRef>(name),bsd,sizeof(bsd),kCFStringEncodingUTF8))continue;auto data=static_cast<CFDataRef>(toc);const auto length=CFDataGetLength(data);if(length<4||length>65537)throw std::runtime_error("invalid optical-disc TOC property");const auto* bytes=CFDataGetBytePtr(data);result.push_back({std::string("/dev/r")+bsd,decode_cd_toc(Bytes(bytes,bytes+length))});
 }
 return result;
}
}

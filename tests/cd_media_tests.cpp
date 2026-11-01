#include "yukari/cd_media.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace yukari;
int main(){
 Bytes toc(37);toc[1]=35;toc[2]=toc[3]=1;
 auto descriptor=[&](size_t p,uint8_t track,bool data,uint8_t minute,uint8_t second){toc[p]=1;toc[p+1]=data?0x14:0x10;toc[p+3]=track;toc[p+8]=minute;toc[p+9]=second;};
 descriptor(4,1,false,0,2);descriptor(15,2,true,1,2);descriptor(26,0xa2,false,2,2);auto tracks=decode_cd_toc(toc);assert(tracks.size()==2&&tracks[0].audio&&!tracks[1].audio);assert(tracks[0].start_sector==0&&tracks[0].end_sector==4500&&tracks[0].duration_ms()==60000);assert(tracks[1].end_sector==9000);
 auto bad=toc;bad.pop_back();bool rejected=false;try{decode_cd_toc(bad);}catch(const std::runtime_error&){rejected=true;}assert(rejected);bad=toc;bad[13]=60;rejected=false;try{decode_cd_toc(bad);}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 const auto discs=enumerate_cd_media();std::cout<<"CD TOC audio/data tracks and read-only device enumeration passed; present discs="<<discs.size()<<'\n';
}

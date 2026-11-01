#include "yukari/pe_resources.hpp"
#include <cassert>
#include <iostream>
using namespace yukari;
int main(){
 Bytes pe(0x500);auto w=[&](size_t p,unsigned v){pe[p]=v;pe[p+1]=v>>8;};auto d=[&](size_t p,uint32_t v){for(int i=0;i<4;++i)pe[p+i]=v>>(8*i);};
 w(0,0x5a4d);d(0x3c,0x80);d(0x80,0x4550);w(0x86,1);w(0x94,224);w(0x98,0x10b);d(0x98+92,16);d(0x98+112,0x1000);d(0x178+12,0x1000);d(0x178+16,0x300);d(0x178+20,0x200);
 w(0x20e,1);d(0x210,10);d(0x214,0x80000020);w(0x22e,1);d(0x230,27);d(0x234,0x80000040);w(0x24e,1);d(0x250,1041);d(0x254,0x60);d(0x260,0x1100);d(0x264,3);pe[0x300]=1;pe[0x301]=2;pe[0x302]=3;
 assert(pe_resource(pe,10,27)==Bytes({1,2,3}));bool failed=false;try{pe_resource(pe,10,28);}catch(...){failed=true;}assert(failed);
 d(0x264,0x1000);failed=false;try{pe_resource(pe,10,27);}catch(...){failed=true;}assert(failed);std::cout<<"PE resource language fallback and RVA bounds passed\n";
}

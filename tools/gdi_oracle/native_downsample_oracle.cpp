// Controlled original-x86 oracle: no game entrypoint/TLS/import execution.
// Build only as i686; see native-downsample-report.md for the isolated run.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wincrypt.h>
#include "yukari/native_glyph_effect.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr char original_sha[]="08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267";
constexpr uintptr_t game_base=0x400000,helper_va=0x405250;
constexpr uint32_t guard=0x5ac3e719;
size_t allocations=0,frees=0;unsigned char* allocation=nullptr;size_t allocation_size=0;
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
[[noreturn]] void hook_failure(const char* message){std::fprintf(stderr,"ORACLE_HOOK_FAILURE %s\n",message);TerminateProcess(GetCurrentProcess(),65);std::abort();}
extern "C" void* __cdecl fixture_malloc(size_t size){
 if(allocation||size>1024*1024)hook_failure("unexpected allocation count/size");
 allocation=static_cast<unsigned char*>(std::malloc(size+32));if(!allocation)hook_failure("fixture allocation failed");allocation_size=size;
 std::memset(allocation,0xa7,16);std::memset(allocation+16+size,0x7a,16);++allocations;return allocation+16;
}
extern "C" void __cdecl fixture_free(void* p){
 if(!allocation||p!=allocation+16)hook_failure("unexpected free pointer");
 for(size_t i=0;i<16;++i)if(allocation[i]!=0xa7||allocation[16+allocation_size+i]!=0x7a)hook_failure("native scratch overrun");
 std::free(allocation);allocation=nullptr;allocation_size=0;++frees;
}
LONG WINAPI fault(EXCEPTION_POINTERS* pointers){std::fprintf(stderr,"ORACLE_NATIVE_FAULT code=%08lx address=%p\n",pointers->ExceptionRecord->ExceptionCode,pointers->ExceptionRecord->ExceptionAddress);TerminateProcess(GetCurrentProcess(),66);return EXCEPTION_EXECUTE_HANDLER;}
std::vector<unsigned char> read_original(const wchar_t* path){
 HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);require(h!=INVALID_HANDLE_VALUE,"cannot open original EXE read-only");
 LARGE_INTEGER length{};if(!GetFileSizeEx(h,&length)||length.QuadPart<4096||length.QuadPart>16*1024*1024){CloseHandle(h);throw std::runtime_error("original EXE size outside fixture bound");}
 std::vector<unsigned char> data(static_cast<size_t>(length.QuadPart));DWORD got=0;const bool ok=ReadFile(h,data.data(),DWORD(data.size()),&got,nullptr)!=0;CloseHandle(h);require(ok&&got==data.size(),"cannot read complete original EXE");return data;
}
std::string sha256(const std::vector<unsigned char>& data){
 HCRYPTPROV provider=0;HCRYPTHASH hash=0;require(CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)!=0,"SHA256 provider unavailable");
 if(!CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)){CryptReleaseContext(provider,0);throw std::runtime_error("cannot create SHA256");}
 bool ok=CryptHashData(hash,data.data(),DWORD(data.size()),0)!=0;unsigned char digest[32]{};DWORD count=sizeof digest;ok=ok&&CryptGetHashParam(hash,HP_HASHVAL,digest,&count,0)!=0&&count==32;CryptDestroyHash(hash);CryptReleaseContext(provider,0);require(ok,"SHA256 failed");
 const char* hex="0123456789abcdef";std::string result;for(auto c:digest){result+=hex[c>>4];result+=hex[c&15];}return result;
}
uintptr_t execution_va=0;
struct Mapping {
 unsigned char* memory{};
 explicit Mapping(const std::vector<unsigned char>& file){
  require(file.size()>sizeof(IMAGE_DOS_HEADER),"truncated DOS header");const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());require(dos->e_magic==IMAGE_DOS_SIGNATURE&&dos->e_lfanew>=0&&size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)<=file.size(),"invalid PE header");const auto* pe=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
  require(pe->Signature==IMAGE_NT_SIGNATURE&&pe->FileHeader.Machine==IMAGE_FILE_MACHINE_I386&&pe->OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR32_MAGIC&&pe->OptionalHeader.ImageBase==game_base,"not the expected PE32 image");
  const auto section_offset=size_t(dos->e_lfanew)+24+pe->FileHeader.SizeOfOptionalHeader;require(section_offset+size_t(pe->FileHeader.NumberOfSections)*sizeof(IMAGE_SECTION_HEADER)<=file.size(),"invalid section header table");
  constexpr size_t length=0x405644-helper_va;const unsigned char* source=nullptr;const auto* sections=reinterpret_cast<const IMAGE_SECTION_HEADER*>(file.data()+section_offset);
  for(unsigned i=0;i<pe->FileHeader.NumberOfSections;++i){const auto& section=sections[i];const uint32_t rva=uint32_t(helper_va-game_base);if(rva>=section.VirtualAddress&&uint64_t(rva)+length<=uint64_t(section.VirtualAddress)+section.SizeOfRawData){const auto at=uint64_t(section.PointerToRawData)+rva-section.VirtualAddress;require(at+length<=file.size(),"kernel source exceeds original file");source=file.data()+size_t(at);break;}}
  require(source,"software function is not backed by source bytes");
  const unsigned char cmp[]={0x83,0x3d,0x94,0xd8,0x4c,0x00,0x00};require(std::memcmp(source+0x58,cmp,sizeof cmp)==0&&source[length-1]==0xc3,"software branch signature mismatch");
  auto call_target=[&](size_t offset){require(source[offset]==0xe8,"expected native direct call missing");uint32_t displacement;std::memcpy(&displacement,source+offset+1,4);return uint32_t(helper_va+offset+5)+displacement;};
  require(call_target(0xb4)==0x464301&&call_target(0x3df)==0x4642fc,"audited native allocation calls differ");
  // Extract only 405250..405643. No PE loading, imports, TLS or entrypoint.
  // Its unavailable DirectDraw branch lands on a trap-filled byte, and its
  // selector reads an immutable zero DWORD in this same private RX page.
  memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));require(memory,"private kernel allocation failed");
  try{
   std::memset(memory,0xcc,4096);std::memcpy(memory,source,length);const uint32_t zero=0;std::memcpy(memory+0x800,&zero,4);
   const uint32_t selector=uint32_t(uintptr_t(memory+0x800));std::memcpy(memory+0x5a,&selector,4);
   patch_call(0xb4,uintptr_t(&fixture_malloc));patch_call(0x3df,uintptr_t(&fixture_free));
   // Verify all other native software bytes remain exactly original.
   for(size_t i=0;i<length;++i)if(!((i>=0x5a&&i<0x5e)||(i>=0xb5&&i<0xb9)||(i>=0x3e0&&i<0x3e4)))require(memory[i]==source[i],"noninstrumented native instruction changed");
   DWORD old=0;require(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old)!=0,"cannot seal private kernel RX");FlushInstructionCache(GetCurrentProcess(),memory,4096);execution_va=uintptr_t(memory);
  }catch(...){VirtualFree(memory,0,MEM_RELEASE);memory=nullptr;throw;}
 }
 void patch_call(size_t offset,uintptr_t target){const auto relative=uint32_t(target)-uint32_t(uintptr_t(memory+offset+5));std::memcpy(memory+offset+1,&relative,4);}
 ~Mapping(){execution_va=0;if(memory)VirtualFree(memory,0,MEM_RELEASE);}
};
struct Surface {
 std::array<unsigned char,0x40> bytes{};
 Surface(uint32_t* pixels,int width,int height){const auto pointer=uint32_t(uintptr_t(pixels));std::memcpy(bytes.data()+8,&pointer,4);std::memcpy(bytes.data()+0x24,&height,4);std::memcpy(bytes.data()+0x28,&width,4);}
};
uint32_t random_state=0x94d3057b;
uint32_t random_word(){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
uint64_t digest=14695981039346656037ULL;size_t compared_pixels=0;
void test(int w,int h,unsigned pattern){
 const size_t count=size_t(w)*h;std::vector<uint32_t> input(count+8,guard),output(size_t(w/4)*(h/4)+8,guard);
 for(size_t i=0;i<count;++i){uint32_t value=random_word();if(pattern==0)value=0;else if(pattern==1)value=0xffffffff;else if(pattern==2)value=(i&1)?0:0xffffffff;else if(pattern==3)value=uint32_t(i&255)*0x01010101u;input[i+4]=value;}
 const auto original=input;yukari::NativeGlyphPlane source{w,h,std::vector<uint32_t>(input.begin()+4,input.end()-4)};const auto expected=yukari::native_glyph_downsample4(source);
 Surface src(input.data()+4,w,h),dst(output.data()+4,w/4,h/4);const auto src_header=src.bytes,dst_header=dst.bytes;const auto before_allocations=allocations,before_frees=frees;
 using Function=uint32_t(__cdecl*)(void*,void*);const auto result=reinterpret_cast<Function>(execution_va)(dst.bytes.data(),src.bytes.data());
 require((result&255)==1,"native return AL was not 1");require(allocations==before_allocations+1&&frees==before_frees+1&&!allocation,"native allocation/free pair mismatch");require(src.bytes==src_header&&dst.bytes==dst_header,"native helper changed surface headers");require(input==original,"native helper changed source or source guard");
 for(size_t i=0;i<4;++i)require(output[i]==guard&&output[output.size()-1-i]==guard,"native destination guard changed");
 for(size_t i=0;i<expected.pixels.size();++i){if(output[i+4]!=expected.pixels[i]){std::fprintf(stderr,"MISMATCH src=%dx%d pattern=%u pixel=%lu native=%08lx yukari=%08lx\n",w,h,pattern,static_cast<unsigned long>(i),static_cast<unsigned long>(output[i+4]),static_cast<unsigned long>(expected.pixels[i]));throw std::runtime_error("native x86 downsampling differs from Yukari");}digest^=output[i+4];digest*=1099511628211ULL;++compared_pixels;}
}
}
int wmain(int argc,wchar_t** argv){
 static_assert(sizeof(void*)==4,"this oracle must be built as PE32 i686");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(fault);
 try{
  require(argc==2,"usage: native-downsample-oracle32.exe ORIGINAL-game.exe");const auto file=read_original(argv[1]);require(sha256(file)==original_sha,"original EXE SHA256 mismatch; nothing mapped or executed");Mapping mapped(file);

  size_t cases=0;for(const auto& extent:std::vector<std::pair<int,int>>{{0,0},{0,4},{4,0},{4,4},{4,8},{8,4},{12,28},{28,12},{124,128},{128,124},{256,4},{4,256}})for(unsigned pattern=0;pattern<5;++pattern){test(extent.first,extent.second,pattern);++cases;}
  for(size_t i=0;i<2048;++i){const int w=int(1+random_word()%64)*4,h=int(1+random_word()%64)*4;test(w,h,4);++cases;}
  std::cout<<"{\"schema\":1,\"original_sha256\":\""<<original_sha<<"\",\"helper_va\":\"0x405250\",\"backend\":\"software\",\"game_entry_requested\":false,\"load_flags\":\"PRIVATE_SOFTWARE_KERNEL_NO_LOADER\",\"cases\":"<<cases<<",\"compared_bgra_pixels\":"<<compared_pixels<<",\"allocations\":"<<allocations<<",\"frees\":"<<frees<<",\"fnv64_words\":\""<<std::hex<<digest<<"\",\"status\":\"PASS\"}\n";return 0;
 }catch(const std::exception& e){std::cerr<<"ORACLE_FAIL "<<e.what()<<'\n';return 1;}
}

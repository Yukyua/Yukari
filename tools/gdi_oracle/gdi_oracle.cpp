// Standalone GDI sampling tool. Does not execute, inject into or modify a game.
// Raw TextOutA planes are oracle input, not an implementation of Yuka effects.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include "yukari/gdi_cache.hpp"
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#include "yukari/font_effect.hpp"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void require(bool yes,const char* what){if(!yes)throw std::runtime_error(std::string(what)+" Win32="+std::to_string(GetLastError()));}
std::string hex(const void* bytes,size_t n){std::ostringstream out;for(size_t i=0;i<n;++i)out<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(static_cast<const unsigned char*>(bytes)[i]);return out.str();}
std::string unhex(const std::string& value){require(value.size()%2==0,"odd hexadecimal length");std::string out;for(size_t i=0;i<value.size();i+=2){auto digit=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("invalid hexadecimal input");};out+=char(digit(value[i])*16+digit(value[i+1]));}return out;}
std::string quoted(const std::wstring& value){std::ostringstream out;out<<'"';for(wchar_t c:value){if(c=='"'||c=='\\')out<<'\\'<<char(c);else if(c>=32&&c<127)out<<char(c);else out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<unsigned(c);}out<<'"';return out.str();}
std::wstring wide(const std::string& s,UINT cp){if(s.empty())return {};int n=MultiByteToWideChar(cp,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);require(n>0,"MultiByteToWideChar");std::wstring out(n,0);require(MultiByteToWideChar(cp,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),out.data(),n)==n,"MultiByteToWideChar copy");return out;}
std::string narrow(const std::wstring& s,UINT cp){if(s.empty())return {};BOOL fallback=FALSE;int n=WideCharToMultiByte(cp,0,s.data(),int(s.size()),nullptr,0,nullptr,cp==CP_UTF8?nullptr:&fallback);require(n>0&&!fallback,"WideCharToMultiByte");std::string out(n,0);require(WideCharToMultiByte(cp,0,s.data(),int(s.size()),out.data(),n,nullptr,cp==CP_UTF8?nullptr:&fallback)==n&&!fallback,"WideCharToMultiByte copy");return out;}
std::string hash_file(const std::string& path){
 HANDLE file=CreateFileW(wide(path,CP_UTF8).c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);require(file!=INVALID_HANDLE_VALUE,"open font file for hash");
 HCRYPTPROV provider{};require(CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT),"CryptAcquireContext");HCRYPTHASH hash{};require(CryptCreateHash(provider,CALG_SHA_256,0,0,&hash),"CryptCreateHash");
 unsigned char buf[65536];DWORD got{};for(;;){require(ReadFile(file,buf,sizeof buf,&got,nullptr),"read font file");if(!got)break;require(CryptHashData(hash,buf,got,0),"CryptHashData");}CloseHandle(file);
 unsigned char digest[32];DWORD size=sizeof digest;require(CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0)&&size==32,"CryptGetHashParam");CryptDestroyHash(hash);CryptReleaseContext(provider,0);return hex(digest,size);
}
void put(const std::string& path,const void* data,size_t n){std::ofstream out(path,std::ios::binary);require(bool(out),"open output");out.write(static_cast<const char*>(data),std::streamsize(n));require(bool(out),"write output");}
struct Dib {
 HDC dc{};HBITMAP bitmap{};HGDIOBJ old{};unsigned char* pixels{};int width{},height{};
 Dib(int w,int h):width(w),height(h){
  require(w>0&&h>0&&uint64_t(w)*h<=64000000,"DIB bounds");dc=CreateCompatibleDC(nullptr);require(dc,"CreateCompatibleDC");
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,reinterpret_cast<void**>(&pixels),nullptr,0);require(bitmap,"CreateDIBSection");old=SelectObject(dc,bitmap);require(old&&old!=HGDI_ERROR,"SelectObject DIB");
 }
 ~Dib(){if(dc&&old)SelectObject(dc,old);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);}
 Dib(const Dib&)=delete;
 void save(const std::string& prefix){
  GdiFlush(); // Must complete GDI work before directly inspecting DIB memory.
  std::vector<unsigned char> rgb(size_t(width)*height*3);
  for(size_t i=0;i<size_t(width)*height;++i){rgb[i*3]=pixels[i*4+2];rgb[i*3+1]=pixels[i*4+1];rgb[i*3+2]=pixels[i*4];}
  std::ostringstream header;header<<"P6\n"<<width<<' '<<height<<"\n255\n";auto s=header.str();std::ofstream ppm(prefix+".ppm",std::ios::binary);ppm<<s;ppm.write(reinterpret_cast<const char*>(rgb.data()),std::streamsize(rgb.size()));require(bool(ppm),"write PPM");
  // BI_RGB's fourth byte is undefined by GDI. Set alpha only in the BMP copy;
  // the RGB PPM is authoritative and never treats that byte as coverage.
  std::vector<unsigned char> bgra(pixels,pixels+size_t(width)*height*4);for(size_t i=3;i<bgra.size();i+=4)bgra[i]=255;
  BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof file+sizeof(BITMAPINFOHEADER);file.bfSize=DWORD(file.bfOffBits+bgra.size());
  BITMAPINFOHEADER info{};info.biSize=sizeof info;info.biWidth=width;info.biHeight=-height;info.biPlanes=1;info.biBitCount=32;info.biCompression=BI_RGB;info.biSizeImage=DWORD(bgra.size());
  std::ofstream bmp(prefix+".bmp",std::ios::binary);bmp.write(reinterpret_cast<const char*>(&file),sizeof file);bmp.write(reinterpret_cast<const char*>(&info),sizeof info);bmp.write(reinterpret_cast<const char*>(bgra.data()),std::streamsize(bgra.size()));require(bool(bmp),"write BMP");
 }
};
void draw_effect(Dib& dib,HFONT font,const std::string& text,COLORREF foreground,COLORREF background,const std::vector<std::pair<int,int>>& origins,int mode){
 auto old=SelectObject(dib.dc,font);require(old&&old!=HGDI_ERROR,"SelectObject effect font");RECT area{0,0,dib.width,dib.height};auto brush=CreateSolidBrush(background);require(brush,"CreateSolidBrush effect");require(FillRect(dib.dc,&area,brush),"FillRect effect");DeleteObject(brush);
 require(SetMapMode(dib.dc,MM_TEXT),"effect SetMapMode");require(SetTextAlign(dib.dc,TA_LEFT|TA_TOP|TA_NOUPDATECP)!=GDI_ERROR,"effect SetTextAlign");require(SetBkMode(dib.dc,mode),"effect SetBkMode");SetBkColor(dib.dc,background);SetTextColor(dib.dc,foreground);for(auto p:origins)require(TextOutA(dib.dc,p.first,p.second,text.data(),int(text.size())),"effect TextOutA");GdiFlush();SelectObject(dib.dc,old);
}
void draw(Dib& dib,HFONT font,const std::string& text,COLORREF foreground,COLORREF background,int origin){
 auto old=SelectObject(dib.dc,font);require(old&&old!=HGDI_ERROR,"SelectObject font");
 RECT area{0,0,dib.width,dib.height};auto brush=CreateSolidBrush(background);require(brush,"CreateSolidBrush");require(FillRect(dib.dc,&area,brush),"FillRect");DeleteObject(brush);
 require(SetMapMode(dib.dc,MM_TEXT),"SetMapMode");require(SetTextAlign(dib.dc,TA_LEFT|TA_TOP|TA_NOUPDATECP)!=GDI_ERROR,"SetTextAlign");
 require(SetBkMode(dib.dc,OPAQUE),"SetBkMode");SetBkColor(dib.dc,background);SetTextColor(dib.dc,foreground);
 require(TextOutA(dib.dc,origin,origin,text.data(),int(text.size())),"TextOutA");GdiFlush();SelectObject(dib.dc,old);
}
}
int wmain(int argc,wchar_t** wide_argv){try{
 std::vector<std::string> arguments;for(int i=0;i<argc;++i)arguments.push_back(narrow(wide_argv[i],CP_UTF8));std::vector<char*> argv;for(auto& arg:arguments)argv.push_back(arg.data());
 // All integers are decimal except raw hex text/face/color values.
 // lfWidth below follows native 405cd0: trunc((baseWidth*scale)/2).
 std::string cache_path,font_file,host_label;std::string prefix="gdi-oracle",face="MS Gothic",text=unhex("93fa967b8cea8142414282a082a2");
 int height=27,width=0,weight=200,scale=4,quality=DEFAULT_QUALITY,charset=SHIFTJIS_CHARSET,margin=8,native_effect=0,border=2,shadow=2;DWORD border_color=0,shadow_color=0;
 DWORD color=0xffffff,background=0;BYTE out_precision=OUT_DEFAULT_PRECIS,clip_precision=CLIP_DEFAULT_PRECIS,pitch=DEFAULT_PITCH;
 for(int i=1;i<argc;++i){std::string key=argv[i];if(key=="--help"){std::cout<<"gdi_oracle --out PREFIX --face-hex CP932HEX --text-hex CP932HEX --height 27 --width 0 --weight 200 --scale 1|2|4 --quality N --charset N --margin 8 --color RRGGBB --background RRGGBB --out-precision N --clip-precision N --pitch N --cache FILE --font-file UTF8PATH --text-utf8 TEXT --face-utf8 FAMILY --host LABEL --native-effect 1..5 --border N --shadow N --border-color RRGGBB --shadow-color RRGGBB\n";return 0;}require(i+1<argc,"missing option value");std::string v=argv[++i];int n=0;if(key!="--out"&&key!="--face-hex"&&key!="--text-hex"&&key!="--color"&&key!="--background"&&key!="--cache"&&key!="--font-file"&&key!="--text-utf8"&&key!="--face-utf8"&&key!="--host"&&key!="--border-color"&&key!="--shadow-color"){size_t used{};n=std::stoi(v,&used);require(used==v.size(),"invalid integer");}
  if(key=="--cache")cache_path=v;else if(key=="--font-file")font_file=v;else if(key=="--host")host_label=v;else if(key=="--text-utf8")text=narrow(wide(v,CP_UTF8),932);else if(key=="--face-utf8")face=narrow(wide(v,CP_UTF8),932);else if(key=="--out")prefix=v;else if(key=="--face-hex")face=unhex(v);else if(key=="--text-hex")text=unhex(v);else if(key=="--height")height=n;else if(key=="--width")width=n;else if(key=="--weight")weight=n;else if(key=="--scale")scale=n;else if(key=="--native-effect")native_effect=n;else if(key=="--border")border=n;else if(key=="--shadow")shadow=n;else if(key=="--quality")quality=n;else if(key=="--charset")charset=n;else if(key=="--margin")margin=n;else if(key=="--out-precision")out_precision=BYTE(n);else if(key=="--clip-precision")clip_precision=BYTE(n);else if(key=="--pitch")pitch=BYTE(n);else if(key=="--color"||key=="--background"||key=="--border-color"||key=="--shadow-color"){require(v.size()==6,"RGB requires six hex digits");auto b=unhex(v);DWORD c=RGB(uint8_t(b[0]),uint8_t(b[1]),uint8_t(b[2]));if(key=="--color")color=c;else if(key=="--border-color")border_color=c;else if(key=="--shadow-color")shadow_color=c;else background=c;}else throw std::runtime_error("unknown option "+key);
 }
 require((scale==1||scale==2||scale==4)&&height>=-512&&height<=512&&height!=0&&width>= -512&&width<=512&&margin>=0&&margin<=1024&&text.size()<=4096&&face.size()<LF_FACESIZE&&face.find('\0')==std::string::npos,"invalid font/input bounds");
 require(native_effect>=0&&native_effect<=5&&border>=0&&border<=512&&shadow>=0&&shadow<=512,"invalid native effect settings");
 if(native_effect)scale=native_effect==1?1:4;
 LOGFONTA font{};font.lfHeight=height*scale;font.lfWidth=width*scale/2;font.lfWeight=weight;font.lfCharSet=BYTE(charset);font.lfQuality=BYTE(quality);font.lfOutPrecision=out_precision;font.lfClipPrecision=clip_precision;font.lfPitchAndFamily=pitch;std::memcpy(font.lfFaceName,face.data(),face.size());
 HFONT selected=CreateFontIndirectA(&font);require(selected,"CreateFontIndirectA");HDC metrics=CreateCompatibleDC(nullptr);require(metrics,"metrics DC");auto old=SelectObject(metrics,selected);
 SIZE extent{};TEXTMETRICW tm{};require(GetTextExtentPoint32A(metrics,text.data(),int(text.size()),&extent),"GetTextExtentPoint32A");require(GetTextMetricsW(metrics,&tm),"GetTextMetricsW");
 wchar_t chosen[LF_FACESIZE]{};require(GetTextFaceW(metrics,LF_FACESIZE,chosen)>0,"GetTextFaceW");LOGFONTW actual{};require(GetObjectW(selected,sizeof actual,&actual)==sizeof actual,"GetObjectW");
 yukari::FontEffectPlan plan;int w=std::max(1,int(extent.cx))+2*margin,h=std::max(1,int(extent.cy))+2*margin;
 if(native_effect){yukari::FontEffectInput input;input.native_effect=native_effect;input.border_thickness=border;input.shadow_thickness=shadow;input.border_color=border_color;input.shadow_color=shadow_color;plan=yukari::resolve_font_effect(input);auto logical=plan.canvas(extent.cx,extent.cy);w=logical.first*plan.supersample;h=logical.second*plan.supersample;margin=0;background=plan.kind==yukari::FontEffectPlan::Shadow?shadow_color:(plan.kind==yukari::FontEffectPlan::Border||plan.kind==yukari::FontEffectPlan::BorderShadow?border_color:color);}
 Dib mask(w,h),ink(w,h);
 if(native_effect){
  const int color_mode=plan.kind==yukari::FontEffectPlan::BorderShadow?TRANSPARENT:OPAQUE;
  draw_effect(ink,selected,text,color,background,{plan.color_glyph_origin_hi},color_mode);
  std::vector<std::pair<int,int>> origins{plan.mask_glyph_origin_hi};
  const bool shadow_only=plan.kind==yukari::FontEffectPlan::Shadow;
  if(shadow_only)origins.insert(origins.begin(),plan.shadow_mask_translation_hi);
  const int mask_mode=shadow_only||plan.kind==yukari::FontEffectPlan::BorderShadow?TRANSPARENT:OPAQUE;
  draw_effect(mask,selected,text,0,0xffffff,origins,mask_mode);
 }else{draw(mask,selected,text,RGB(0,0,0),RGB(255,255,255),margin);draw(ink,selected,text,color,background,margin);}
 mask.save(prefix+"-mask");ink.save(prefix+"-ink");
 std::string font_hash=font_file.empty()?std::string{}:hash_file(font_file);
 auto wine_proc=GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"wine_get_version");const char* (__cdecl* wine_version)()=nullptr;static_assert(sizeof(wine_version)==sizeof(wine_proc));std::memcpy(&wine_version,&wine_proc,sizeof(wine_version));
 const std::string detected_host=wine_version?std::string("wine:")+wine_version():"windows-native";
 if(!host_label.empty())host_label=detected_host+"; "+host_label;else host_label=detected_host;
 if(!cache_path.empty()){
  require(!font_hash.empty(),"--cache requires --font-file for reproducible font SHA256");
  yukari::GdiGlyphCache c;c.height=height;c.width=width;c.weight=weight;c.charset=charset;c.quality=quality;c.scale=scale;c.out_precision=out_precision;c.clip_precision=clip_precision;c.pitch=pitch;c.acp=GetACP();c.extent_width=extent.cx;c.extent_height=extent.cy;c.bitmap_width=w;c.bitmap_height=h;c.origin_x=margin;c.origin_y=margin;c.family=narrow(wide(face,932),CP_UTF8);c.text=narrow(wide(text,932),CP_UTF8);c.actual_family=narrow(chosen,CP_UTF8);c.host=host_label;c.font_sha256=font_hash;c.foreground=color;c.background=background;c.native_effect=native_effect;c.border_thickness=border;c.shadow_thickness=shadow;c.border_color=border_color;c.shadow_color=shadow_color;
  GdiFlush();c.bgra.assign(mask.pixels,mask.pixels+size_t(w)*h*4);c.color_bgra.assign(ink.pixels,ink.pixels+size_t(w)*h*4);for(size_t i=3;i<c.bgra.size();i+=4){c.bgra[i]=255;c.color_bgra[i]=255;}auto encoded=yukari::encode_gdi_cache(c);put(cache_path,encoded.data(),encoded.size());
 }
 std::ostringstream json;json<<"{\n  \"kind\": \"raw-TextOutA-GDI-planes\",\n  \"host\": "<<quoted(wide(host_label,CP_UTF8))<<",\n  \"font_file_sha256\": \""<<font_hash<<"\",\n  \"acp\": "<<GetACP()<<",\n  \"oemcp\": "<<GetOEMCP()<<",\n  \"thread_lcid\": "<<GetThreadLocale()<<",\n  \"face_hex\": \""<<hex(face.data(),face.size())<<"\",\n  \"text_hex\": \""<<hex(text.data(),text.size())<<"\",\n  \"selected_face\": "<<quoted(chosen)<<",\n  \"logical_font\": {\"height\": "<<font.lfHeight<<", \"width\": "<<font.lfWidth<<", \"weight\": "<<font.lfWeight<<", \"charset\": "<<unsigned(font.lfCharSet)<<", \"quality\": "<<unsigned(font.lfQuality)<<", \"out_precision\": "<<unsigned(font.lfOutPrecision)<<", \"clip_precision\": "<<unsigned(font.lfClipPrecision)<<", \"pitch\": "<<unsigned(font.lfPitchAndFamily)<<"},\n  \"native_effect\": "<<native_effect<<",\n  \"border_thickness\": "<<border<<",\n  \"shadow_thickness\": "<<shadow<<",\n  \"requested_scale\": "<<scale<<",\n  \"margin\": "<<margin<<",\n  \"extent\": ["<<extent.cx<<", "<<extent.cy<<"],\n  \"metrics\": {\"height\": "<<tm.tmHeight<<", \"ascent\": "<<tm.tmAscent<<", \"descent\": "<<tm.tmDescent<<", \"internal_leading\": "<<tm.tmInternalLeading<<", \"external_leading\": "<<tm.tmExternalLeading<<", \"average_width\": "<<tm.tmAveCharWidth<<", \"max_width\": "<<tm.tmMaxCharWidth<<", \"weight\": "<<tm.tmWeight<<", \"charset\": "<<unsigned(tm.tmCharSet)<<"},\n  \"canvas\": ["<<w<<", "<<h<<"],\n  \"notes\": \"Raw RGB source planes only; no native Yuka effect, downsample or alpha inference. GDI alpha byte is undefined. Label host and font hash externally.\"\n}\n";auto data=json.str();put(prefix+".json",data.data(),data.size());std::cout<<data;
 SelectObject(metrics,old);DeleteDC(metrics);DeleteObject(selected);return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

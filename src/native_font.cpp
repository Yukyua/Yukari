#include "yukari/native_font.hpp"
#include "yukari/gdi_cache.hpp"
#include <cctype>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SYNTHESIS_H
#include <algorithm>
#include <map>
#include <tuple>
#include <stdexcept>
#include <cmath>
namespace yukari {
bool NativeFontSpec::operator<(const NativeFontSpec& b)const{return std::tie(family,height,width,weight,charset,quality,effect,border_thickness,shadow_thickness,border_color,shadow_color,inherit_effect,inherit_border,inherit_shadow)<std::tie(b.family,b.height,b.width,b.weight,b.charset,b.quality,b.effect,b.border_thickness,b.shadow_thickness,b.border_color,b.shadow_color,b.inherit_effect,b.inherit_border,b.inherit_shadow);}
namespace {
std::vector<uint32_t> unicode(const std::string& text){
 std::vector<uint32_t> out;for(size_t i=0;i<text.size();){uint32_t c=uint8_t(text[i++]),minimum=0;unsigned n=0;if(c<128){}else if(c>=0xc2&&c<=0xdf){c&=31;n=1;minimum=128;}else if(c>=0xe0&&c<=0xef){c&=15;n=2;minimum=2048;}else if(c>=0xf0&&c<=0xf4){c&=7;n=3;minimum=65536;}else throw std::runtime_error("invalid native font UTF-8");while(n--){if(i==text.size()||(uint8_t(text[i])&192)!=128)throw std::runtime_error("invalid native font UTF-8");c=(c<<6)|(uint8_t(text[i++])&63);}if(c<minimum||c>0x10ffff||(c>=0xd800&&c<=0xdfff))throw std::runtime_error("invalid native font code point");out.push_back(c);}return out;
}
std::string family_key(std::string name){if(name=="ＭＳ ゴシック")return "msgothic";if(name=="ＭＳ 明朝")return "msmincho";std::string out;for(unsigned char c:name)if(c!=' '&&c!='-'&&c!='_')out+=char(c>='A'&&c<='Z'?c+32:c);return out;}
}
struct NativeFontRasterizer::Impl {
 FT_Library library{};NativeFontOptions options;
 using FaceKey=std::pair<std::filesystem::path,long>;
 std::map<FaceKey,FT_Face> faces;
 std::map<std::string,FaceKey> registered;
 using Key=std::tuple<std::string,std::filesystem::path,NativeFontSpec,uint32_t>;
 std::map<Key,std::shared_ptr<NativeFontBitmap>> cache;
 struct Capture{GdiGlyphCache descriptor;std::filesystem::path path;};std::vector<Capture> captures;
 using EffectKey=std::tuple<int,int,int,uint32_t,uint32_t,uint32_t>;
 struct Source {NativeGlyphPlane mask;int advance,cell_height;std::map<EffectKey,std::weak_ptr<NativeFontBitmap>> effects;};
 using SourceKey=std::tuple<FaceKey,std::string,int,int,int,int>;
 std::map<SourceKey,std::shared_ptr<Source>> sources;
 size_t source_bytes=0;uint64_t source_hits=0,source_rasters=0;
 size_t cache_bytes=0;uint64_t hits=0,rasters=0;
 Impl(){if(FT_Init_FreeType(&library))throw std::runtime_error("cannot initialize native font source");}
 ~Impl(){for(auto& [k,f]:faces)FT_Done_Face(f);FT_Done_FreeType(library);}
 FT_Face face(FaceKey key){auto it=faces.find(key);if(it!=faces.end())return it->second;FT_Face f{};if(FT_New_Face(library,key.first.c_str(),key.second,&f))throw std::runtime_error("cannot load native font: "+key.first.string());faces[key]=f;return f;}
 void configure(NativeFontOptions value){
  options=std::move(value);cache.clear();cache_bytes=0;sources.clear();source_bytes=0;registered.clear();captures.clear();
  if(!options.font_directory.empty()){
   if(!std::filesystem::is_directory(options.font_directory))throw std::runtime_error("native font directory does not exist");
   std::vector<std::filesystem::path> paths;for(const auto& e:std::filesystem::directory_iterator(options.font_directory)){auto ext=e.path().extension().string();std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return std::tolower(c);});if(e.is_regular_file()&&(ext==".ttf"||ext==".ttc"||ext==".otf"))paths.push_back(e.path());}std::sort(paths.begin(),paths.end());
   for(const auto& p:paths){FT_Face probe{};if(FT_New_Face(library,p.c_str(),-1,&probe))continue;long count=probe->num_faces;FT_Done_Face(probe);for(long index=0;index<count;++index){auto f=face({p,index});if(f->family_name)registered.emplace(family_key(f->family_name),FaceKey{p,index}); }}
  }
  if(!options.gdi_cache_directory.empty()){
   if(!std::filesystem::is_directory(options.gdi_cache_directory))throw std::runtime_error("GDI glyph cache directory does not exist");
   std::vector<std::filesystem::path> paths;for(const auto& e:std::filesystem::directory_iterator(options.gdi_cache_directory))if(e.is_regular_file()&&e.path().extension()==".ygdi")paths.push_back(e.path());std::sort(paths.begin(),paths.end());
   for(const auto& path:paths){auto capture=decode_gdi_cache(read_file(path));if(!capture.native_effect)continue;capture.bgra.clear();capture.color_bgra.clear();captures.push_back({std::move(capture),path});}
  }

 }
 std::shared_ptr<NativeFontBitmap> captured(const std::string& text,const NativeFontSpec& spec,uint32_t foreground){
  auto plan=resolve_font_effect({spec.effect,2,spec.shadow_thickness,spec.border_thickness,spec.shadow_color,spec.border_color,0,0});
  for(const auto& entry:captures){const auto& c=entry.descriptor;
   if(c.family!=spec.family||c.text!=text||c.height!=spec.height||c.width!=spec.width||c.weight!=spec.weight||c.charset!=spec.charset||c.quality!=spec.quality||c.out_precision||c.clip_precision||c.pitch||c.scale!=uint32_t(plan.supersample)||c.native_effect!=(plan.kind==FontEffectPlan::Plain?1:plan.native_effect)||c.border_thickness!=spec.border_thickness||c.shadow_thickness!=spec.shadow_thickness||c.foreground!=foreground||c.border_color!=spec.border_color||c.shadow_color!=spec.shadow_color)continue;
   auto full=decode_gdi_cache(read_file(entry.path));const auto expected=plan.canvas(full.extent_width,full.extent_height);if(expected.first*int(full.scale)!=full.bitmap_width||expected.second*int(full.scale)!=full.bitmap_height)throw std::runtime_error("GDI capture scratch does not match measured native canvas");auto plane=[&](const Bytes& bytes){NativeGlyphPlane p{full.bitmap_width,full.bitmap_height,{}};p.pixels.resize(size_t(p.width)*p.height);for(size_t i=0;i<p.pixels.size();++i)p.pixels[i]=uint32_t(bytes[i*4])|(uint32_t(bytes[i*4+1])<<8)|(uint32_t(bytes[i*4+2])<<16);return p;};
   auto out=std::make_shared<NativeFontBitmap>();out->planes=native_glyph_finish(plan,plane(full.color_bgra),plane(full.bgra));out->advance=full.extent_width/int(full.scale);out->cell_height=full.extent_height/int(full.scale);out->actual_family=full.actual_family;out->source=full.host;out->captured=true;return out;
  }return {};
 }
 std::shared_ptr<NativeFontBitmap> fallback(const std::string& text,const std::filesystem::path& path,const NativeFontSpec& spec,uint32_t foreground){
  const auto plan=resolve_font_effect({spec.effect,2,spec.shadow_thickness,spec.border_thickness,spec.shadow_color,spec.border_color,0,0});
  auto selected=registered.find(family_key(spec.family));const auto face_key=selected==registered.end()?FaceKey{path,0}:selected->second;auto f=face(face_key);
  if(spec.height==INT32_MIN||std::abs(spec.height)>512||spec.width==INT32_MIN||std::abs(spec.width)>1024)throw std::runtime_error("native logical font size exceeds limit");
  const int scale=plan.supersample;
  // Effects and colors use the same high-resolution monochrome source.
  // Key the actual selected face, so fallback family aliases can share ink
  // while independently registered fonts and 1x/4x geometry remain distinct.
  const SourceKey key{face_key,text,spec.height,spec.width,spec.weight,scale};
  auto found=sources.find(key);std::shared_ptr<Source> source;
  if(found!=sources.end()){source=found->second;++source_hits;}else{
  const auto* os2=static_cast<TT_OS2*>(FT_Get_Sfnt_Table(f,ft_sfnt_os2));
  // Positive LOGFONT height includes internal leading; negative height is em.
  const int metric=spec.height>0&&os2&&int(os2->usWinAscent)+os2->usWinDescent>0?int(os2->usWinAscent)+os2->usWinDescent:std::max<int>(1,f->units_per_EM);
  const int requested=std::max(1,std::abs(spec.height));
  auto em=FT_F26Dot6(std::max<int64_t>(64,(int64_t(requested)*scale*f->units_per_EM*64+metric/2)/metric));
  if(FT_Set_Char_Size(f,0,em,72,72))throw std::runtime_error("cannot set native font em size");
  double xratio=1.0;
  if(spec.width){const int logical_width=(spec.width*scale)/2;const int average=os2&&os2->xAvgCharWidth>0?os2->xAvgCharWidth:std::max<int>(1,f->units_per_EM/2);const double current=double(average)*f->size->metrics.x_scale/65536.0/64.0;xratio=std::abs(logical_width)/std::max(.01,current);}
  FT_Matrix transform{FT_Fixed(std::llround(xratio*65536.0)),0,0,65536};FT_Set_Transform(f,&transform,nullptr);
  struct Placement{int x,y,w,h,pitch;Bytes bytes;};std::vector<Placement> glyphs;int pen=0;
  const int ascender=int((f->size->metrics.ascender+63)>>6);
  // GDI metric mapping remains a source fallback. Binary high-resolution ink
  // lets the engine's exact dual-plane kernel operate without invented AA RGB.
  for(auto cp:unicode(text)){if(cp=='\r'||cp=='\n')throw std::runtime_error("native glyph cell contains a line break");
   if(FT_Load_Char(f,cp,FT_LOAD_DEFAULT|FT_LOAD_TARGET_MONO))throw std::runtime_error("cannot load native font glyph");
   if(spec.weight>=600&&!(f->style_flags&FT_STYLE_FLAG_BOLD))FT_GlyphSlot_Embolden(f->glyph);
   if(FT_Render_Glyph(f->glyph,FT_RENDER_MODE_MONO))throw std::runtime_error("cannot rasterize native font glyph");auto& b=f->glyph->bitmap;
   Placement p{pen+f->glyph->bitmap_left,ascender-f->glyph->bitmap_top,int(b.width),int(b.rows),int((b.width+7)/8),{}};p.bytes.resize(size_t(p.pitch)*p.h);for(int y=0;y<p.h;++y){auto* row=b.pitch>=0?b.buffer+y*b.pitch:b.buffer+(p.h-y-1)*(-b.pitch);std::copy(row,row+p.pitch,p.bytes.begin()+size_t(y)*p.pitch);}glyphs.push_back(std::move(p));pen+=int((f->glyph->advance.x+32)>>6);if(pen<0||pen>16384)throw std::runtime_error("native glyph source width exceeds limit");
  }
  const int height=spec.height>0?requested*scale:std::max(1,int((f->size->metrics.height+63)>>6));
  if(pen<0||pen>16384||height>4096||uint64_t(pen)*height>16000000)throw std::runtime_error("native glyph source bounds exceed limit");
  NativeGlyphPlane mask{pen,height,std::vector<uint32_t>(size_t(pen)*height,0x00ffffff)};
  for(const auto& p:glyphs)for(int y=0;y<p.h;++y)for(int x=0;x<p.w;++x){const int dx=p.x+x,dy=p.y+y;if(dx>=0&&dy>=0&&dx<pen&&dy<height&&(p.bytes[size_t(y)*p.pitch+x/8]&(128>>(x%8))))mask.pixels[size_t(dy)*pen+dx]=0;}
  source=std::make_shared<Source>(Source{std::move(mask),pen/scale,height/scale,{}});++source_rasters;
  const size_t bytes=source->mask.pixels.size()*sizeof(uint32_t);
  if(bytes<=16*1024*1024){if(source_bytes+bytes>16*1024*1024||sources.size()>=1024){sources.clear();source_bytes=0;}source_bytes+=bytes;sources.emplace(key,source);}
  }
  const EffectKey effect{spec.effect,spec.border_thickness,spec.shadow_thickness,spec.border_color,spec.shadow_color,foreground};
  if(auto old=source->effects.find(effect);old!=source->effects.end())if(auto bitmap=old->second.lock())return bitmap;
  auto out=std::make_shared<NativeFontBitmap>();out->advance=source->advance;out->cell_height=source->cell_height;out->planes=native_glyph_effect_binary(plan,source->mask,foreground);out->actual_family=f->family_name?f->family_name:"";out->source="freetype-mono";
  if(source->effects.size()>=64)source->effects.clear();source->effects[effect]=out;return out;
 }
};
NativeFontRasterizer::NativeFontRasterizer():impl_(std::make_unique<Impl>()){}
NativeFontRasterizer::~NativeFontRasterizer()=default;
void NativeFontRasterizer::configure(NativeFontOptions options){impl_->configure(std::move(options));}
std::shared_ptr<const NativeFontBitmap> NativeFontRasterizer::rasterize(const std::string& text,const std::filesystem::path& fallback,const NativeFontSpec& spec,uint32_t foreground){
 if(text.empty())return std::make_shared<NativeFontBitmap>();if(text.size()>1024*1024)throw std::runtime_error("native glyph text exceeds limit");Impl::Key key{text,fallback,spec,foreground};auto found=impl_->cache.find(key);if(found!=impl_->cache.end()){++impl_->hits;return found->second;}
 auto result=impl_->captured(text,spec,foreground);
 if(!result&&impl_->options.require_gdi_capture)throw std::runtime_error("missing matching GDI glyph capture for "+spec.family+": "+text);
 if(!result)result=impl_->fallback(text,fallback,spec,foreground);++impl_->rasters;const auto bytes=result->planes.color.pixels.size()*8;if(impl_->cache_bytes+bytes>64*1024*1024){impl_->cache.clear();impl_->cache_bytes=0;}impl_->cache_bytes+=bytes;impl_->cache.emplace(std::move(key),result);return result;
}
uint64_t NativeFontRasterizer::cache_hits()const{return impl_->hits;}
uint64_t NativeFontRasterizer::rasterizations()const{return impl_->rasters;}
}

namespace yukari {
uint64_t NativeFontRasterizer::source_cache_hits()const{return impl_->source_hits;}
uint64_t NativeFontRasterizer::source_rasterizations()const{return impl_->source_rasters;}
}

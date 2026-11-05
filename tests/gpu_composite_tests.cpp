#include "yukari/render.hpp"
#include <png.h>
#include <iostream>
#include <stdexcept>
#include <functional>
using namespace yukari;
void check(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
Bytes png(int w,int h,const Bytes& bytes){png_image image{};image.version=PNG_IMAGE_VERSION;image.width=w;image.height=h;image.format=PNG_FORMAT_RGBA;png_alloc_size_t count{};check(png_image_write_to_memory(&image,nullptr,&count,0,bytes.data(),0,nullptr),"PNG size");Bytes out(count);check(png_image_write_to_memory(&image,out.data(),&count,0,bytes.data(),0,nullptr),"PNG encode");out.resize(count);return out;}
Bytes payload(Scene& scene,int id){auto snapshot=scene.layer_pixels(id);auto length=u32(snapshot,20);png_image image{};image.version=PNG_IMAGE_VERSION;check(png_image_begin_read_from_memory(&image,snapshot.data()+24,length),"PNG read");image.format=PNG_FORMAT_RGBA;Bytes out(PNG_IMAGE_SIZE(image));check(png_image_finish_read(&image,nullptr,out.data(),0,nullptr),"PNG decode");png_image_free(&image);return out;}
uint32_t seed=0x39e7;uint8_t random_byte(){seed=seed*1664525+1013904223;return uint8_t(seed>>24);}
Bytes random_png(int w,int h){Bytes data(size_t(w)*h*4);for(auto& b:data)b=random_byte();return png(w,h,data);}
int main(){try{
 SDL_SetHint(SDL_HINT_RENDER_DRIVER,"software");Scene reference(64,48,"CPU reference");SDL_SetHint(SDL_HINT_RENDER_DRIVER,"metal");Scene gpu(64,48,"Metal pixel contracts");check(gpu.renderer_info().name=="metal","Metal is required");
 auto load=[&](int id,const Bytes& bytes){reference.load(id,bytes);gpu.load(id,bytes);};
 auto apply=[&](const std::function<void(Scene&)>& operation){auto before=gpu.render_stats();operation(reference);operation(gpu);check(gpu.render_stats().readbacks==before.readbacks,"GPU operation read back pixels before explicit export");};
 unsigned cases=0;size_t bytes=0;
 auto compare=[&](int id,const std::string& label,int tolerance=0){auto cpu=payload(reference,id),actual=payload(gpu,id);check(cpu.size()==actual.size(),label+" dimensions");for(size_t i=0;i<cpu.size();++i)if(std::abs(int(cpu[i])-actual[i])>tolerance)throw std::runtime_error(label+" byte "+std::to_string(i)+" CPU="+std::to_string(cpu[i])+" GPU="+std::to_string(actual[i]));++cases;bytes+=cpu.size();};
 auto image=random_png(37,29),source=random_png(17,11),mask=random_png(17,11);
 Bytes ykg={'Y','K','G','0','0','0',0,0};ykg.resize(64);auto put=[&](size_t off,uint32_t n){for(unsigned i=0;i<4;++i)ykg[off+i]=uint8_t(n>>(i*8));};put(8,64);put(0x28,64);put(0x2c,unsigned(source.size()));put(0x30,unsigned(64+source.size()));put(0x34,unsigned(mask.size()));ykg.insert(ykg.end(),source.begin(),source.end());ykg.insert(ykg.end(),mask.begin(),mask.end());
 for(int type=1;type<=5;++type)for(auto kernel:{NativePixelKernel::Scalar,NativePixelKernel::Mmx}){
  load(0,image);load(1,ykg);apply([&](Scene& s){s.set_image_type(1,type);s.blit(0,-3,4,1,{1,-2,21,18},kernel);});compare(0,"masked blit type "+std::to_string(type));
 }
 for(int type=1;type<=5;++type)for(auto kernel:{NativePixelKernel::Scalar,NativePixelKernel::Mmx})for(auto offset:{SDL_Point{2,-3},SDL_Point{-2,3},SDL_Point{0,1}}){
  load(0,image);apply([&](Scene& s){s.set_image_type(0,type);s.blit(0,offset.x,offset.y,0,{0,0,30,25},kernel);});compare(0,"ordered overlapping self blit type "+std::to_string(type));
 }
 for(int i=0;i<12;++i){load(0,image);apply([&](Scene& s){s.set_add_color(0,(i*31719)&0xffffff);s.set_sub_color(0,(i*79132)&0xffffff);s.set_monochrome(0,i%2);s.set_negaposi(0,i%3==0);});compare(0,"integer filters");}
 load(0,image);apply([](Scene& s){s.set_sepia(0,true);});compare(0,"sepia",1);
 for(auto size:{SDL_Point{17,11},SDL_Point{31,19},SDL_Point{8,5},SDL_Point{1,1}}){load(0,image);load(1,source);apply([&](Scene& s){s.paint(0,1,{-2,3,size.x,size.y});});compare(0,"scaled straight alpha paint "+std::to_string(size.x));}
 load(0,image);apply([](Scene& s){SDL_Rect region{-3,9,24,80};s.set_glyph_color(0,0x23ef89,&region);});compare(0,"partial PATCOPY");
 load(0,image);apply([](Scene& s){s.set_add_color(0,0x235978);});compare(0,"filtered source");load(1,source);apply([](Scene& s){s.blit(0,3,2,1,{0,0,17,11});});compare(0,"copy into filtered backup");apply([](Scene& s){s.set_add_color(0,0);});compare(0,"restore original backup");
 TextStyle style;style.native_font=NativeFontSpec{};style.native_font->height=27;style.native_font->effect=5;style.native_font->border_thickness=3;style.color={240,180,120,255};
 std::vector<PositionedText> runs={{"日本",4,3,style},{"語",13,9,style},{"あ",-4,18,style}};apply([&](Scene& s){s.positioned_text(7,runs,{0,0,64,48});});compare(7,"overlapping native glyph planes");
 apply([&](Scene& s){s.set_alpha(7,137);s.set_visible(7,false);s.positioned_text(7,runs,{2,1,64,48});});compare(7,"text update");
 auto saved=gpu.layer_pixels(7);gpu.create_canvas(8,64,48);gpu.restore_layer_pixels(8,saved);check(payload(gpu,7)==payload(gpu,8),"GPU changed pixels through save restore");
 auto before_thumbnail=gpu.render_stats();auto thumbnail=gpu.resource_thumbnail(png(2,2,Bytes(16,255)),13,7);check(!thumbnail.empty(),"GPU resource thumbnail missing");check(gpu.render_stats().readbacks==before_thumbnail.readbacks+1&&gpu.render_stats().readback_pixels==before_thumbnail.readback_pixels+13*7,"resource thumbnail read full source or skipped readback accounting");
 // Incremental text must equal full CPU replay, including overlapping ink.
 std::vector<PositionedText> incremental;
 const auto append_before=gpu.render_stats();
 for(int i=0;i<18;++i){incremental.push_back({i%2?"日":"語",(i*9)%53-4,(i*3)%24,style});
  apply([&](Scene& s){s.positioned_text(17,incremental,{0,0,64,48});});compare(17,"incremental overlapping text");}
 check(gpu.render_stats().text_appends==append_before.text_appends+17,"text did not append on GPU");
 // A shared canvas must remain immutable until its copy is replaced.
 apply([](Scene& s){s.copy(18,17,0,0);});compare(18,"shared text copy");auto copy_before=payload(gpu,18);
 incremental.push_back({"あ",15,12,style});apply([&](Scene& s){s.positioned_text(17,incremental,{0,0,64,48});});compare(17,"append shared text");
 check(payload(gpu,18)==copy_before,"append modified shared canvas");
 incremental.resize(3);apply([&](Scene& s){s.positioned_text(17,incremental,{0,0,64,48});});compare(17,"shortened text");
 incremental[0].style.color={32,240,100,255};apply([&](Scene& s){s.positioned_text(17,incremental,{0,0,64,48});});compare(17,"restyled prefix");
 auto metrics=gpu.render_stats();check(metrics.gpu_operations>60,"tested operations did not enter GPU pipeline");std::cout<<cases<<" GPU/CPU cases, "<<bytes<<" RGBA bytes checked; GPU operations="<<metrics.gpu_operations<<". Integer operations exact; sepia tolerance 1.\n";
 auto submissions=gpu.render_stats().gpu_submissions;gpu.create_canvas(30,64,48);load(31,source);
 for(int i=0;i<8;++i)gpu.paint(30,31,{i*4,0,4,4});
 check(gpu.render_stats().gpu_submissions==submissions,"GPU image operations submitted before drawing");gpu.draw();
 check(gpu.render_stats().gpu_submissions==submissions+1,"chained GPU image operations were not submitted as one batch");
 load(40,image);auto filter_operations=gpu.render_stats().gpu_operations;
 gpu.set_add_color(40,0x102030);gpu.set_sub_color(40,0x010203);gpu.set_sepia(40,true);gpu.set_monochrome(40,true);gpu.set_negaposi(40,true);
 check(gpu.render_stats().gpu_operations==filter_operations,"filter setters executed redundant full-image compute");gpu.draw();
 check(gpu.render_stats().gpu_operations==filter_operations+1,"filter setters were not coalesced into one compute operation");
 gpu.create_canvas(50,64,48);load(51,ykg);auto mask_uploads=gpu.render_stats().gpu_uploads;
 for(int i=0;i<8;++i)gpu.blit(50,i,0,51,{0,0,8,8});
 check(gpu.render_stats().gpu_uploads==mask_uploads,"repeated masked blits reuploaded the same GPU mask");
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

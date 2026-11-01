#include "yukari/render.hpp"
#include "yukari/gdi_cache.hpp"
#include <png.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace yukari;
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
Bytes current_pixels(Scene& scene,int id,int& width,int& height){auto saved=scene.layer_pixels(id);uint32_t length=u32(saved,20);png_image png{};png.version=PNG_IMAGE_VERSION;check(png_image_begin_read_from_memory(&png,saved.data()+24,length),"cannot decode native text texture");png.format=PNG_FORMAT_RGBA;Bytes bytes(PNG_IMAGE_SIZE(png));check(png_image_finish_read(&png,nullptr,bytes.data(),0,nullptr),"cannot read native text texture");width=int(png.width);height=int(png.height);png_image_free(&png);return bytes;}
int main(int argc,char** argv){try{
 bool gpu=argc>1&&std::string(argv[1])=="--gpu";int arg=gpu?2:1;if(argc<=arg)throw std::runtime_error("expected GDI capture fixture path");
 if(!gpu){SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);}SDL_SetHint(SDL_HINT_RENDER_DRIVER,gpu?"metal":"software");
 auto fixture=decode_gdi_cache(read_file(argv[arg]));TextStyle style;NativeFontSpec font;font.family=fixture.family;font.height=fixture.height;font.width=fixture.width;font.weight=fixture.weight;font.charset=fixture.charset;font.quality=fixture.quality;font.effect=fixture.native_effect;font.border_thickness=fixture.border_thickness;font.shadow_thickness=fixture.shadow_thickness;font.border_color=fixture.border_color;font.shadow_color=fixture.shadow_color;style.native_font=font;style.font_path="/unavailable-font";style.color={uint8_t(fixture.foreground),uint8_t(fixture.foreground>>8),uint8_t(fixture.foreground>>16),255};
 NativeFontRasterizer source;source.configure({{},std::filesystem::path(argv[arg]).parent_path(),true});auto glyph=source.rasterize(fixture.text,style.font_path,font,fixture.foreground);
 Scene scene(128,96,"Native GDI glyph pixels");scene.configure_native_fonts({{},std::filesystem::path(argv[arg]).parent_path(),true});if(gpu)check(scene.renderer_info().accelerated,"Metal test did not select accelerated renderer");
 auto extent=scene.measure_glyph(fixture.text,style);check(extent.x==glyph->planes.color.width&&extent.y==glyph->planes.color.height,"glyph metrics differ from native canvas");
 scene.positioned_text(1,{{fixture.text,11,9,style}},{0,0,128,96});int w,h;auto pixels=current_pixels(scene,1,w,h);size_t nonzero=0;
 for(int y=0;y<96;++y)for(int x=0;x<128;++x){size_t p=size_t(y*128+x)*4;if(x<11||y<9||x>=11+extent.x||y>=9+extent.y){check(pixels[p+3]==0,"native cell escaped its position");continue;}auto at=size_t(y-9)*extent.x+x-11;auto color=glyph->planes.color.pixels[at],mask=glyph->planes.transparency.pixels[at];unsigned a=255-(mask&255);check(pixels[p+3]==a,"mask coverage changed in SDL texture");if(a){check(pixels[p]==uint8_t(color>>16)&&pixels[p+1]==uint8_t(color>>8)&&pixels[p+2]==uint8_t(color),"native color plane recomputed from coverage");++nonzero;}}
 check(nonzero>50,"captured glyph absent");
 auto rendered_png=scene.thumbnail(128,96);png_image rendered{};rendered.version=PNG_IMAGE_VERSION;check(png_image_begin_read_from_memory(&rendered,rendered_png.data(),rendered_png.size()),"cannot read renderer output");rendered.format=PNG_FORMAT_RGBA;Bytes rendered_pixels(PNG_IMAGE_SIZE(rendered));check(png_image_finish_read(&rendered,nullptr,rendered_pixels.data(),0,nullptr),"cannot decode renderer output");png_image_free(&rendered);
 for(size_t i=0;i<pixels.size();i+=4)for(int c=0;c<3;++c){int expected=int(unsigned(pixels[i+c])*pixels[i+3]/255);check(std::abs(int(rendered_pixels[i+c])-expected)<=2,"renderer changed captured GDI pixel beyond blend rounding");}
auto before=scene.render_stats();scene.positioned_text(1,{{fixture.text,11,9,style}},{0,0,128,96});auto after=scene.render_stats();check(after.glyph_rasterizations==before.glyph_rasterizations&&after.text_layout_reuses==before.text_layout_reuses+1&&after.gpu_operations==before.gpu_operations,"native source rerasterized despite identical key");
 auto full=pixels;scene.text(1,fixture.text,11,9,128,96,style);auto from_text=current_pixels(scene,1,w,h);scene.positioned_text(2,{{fixture.text,0,0,style}},{0,0,128,96});auto at_origin=current_pixels(scene,2,w,h);check(from_text==at_origin,"text path adds legacy integer-radius inset to native GDI cell");
 scene.positioned_text(1,{{fixture.text,11,9,style}},{17,15,23,21});auto clipped=current_pixels(scene,1,w,h);for(int y=0;y<h;++y)for(int x=0;x<w;++x)for(int c=0;c<4;++c)check(clipped[(y*w+x)*4+c]==full[((y+15)*128+x+17)*4+c],"native cell was reshaped during clipping");
 std::cout<<"Captured GDI exact color/mask upload, placement, clipping, cache and text integration passed: "<<scene.renderer_info().name<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

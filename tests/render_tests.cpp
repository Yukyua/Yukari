#include "yukari/render.hpp"
#include <png.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
using yukari::Bytes;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
Bytes png(int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = unsigned(w); image.height = unsigned(h); image.format = PNG_FORMAT_RGBA;
    Bytes pixels(size_t(w) * h * 4);
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i] = r; pixels[i+1] = g; pixels[i+2] = b; pixels[i+3] = a;
    }
    png_alloc_size_t count{};
    require(png_image_write_to_memory(&image, nullptr, &count, 0, pixels.data(), 0, nullptr), "PNG size query failed");
    Bytes result(count);
    require(png_image_write_to_memory(&image, result.data(), &count, 0, pixels.data(), 0, nullptr), "PNG encoding failed");
    result.resize(count);
    return result;
}
Bytes rgba_png(int w,int h,const Bytes& pixels){
    png_image image{};image.version=PNG_IMAGE_VERSION;image.width=unsigned(w);image.height=unsigned(h);image.format=PNG_FORMAT_RGBA;
    png_alloc_size_t count{};require(png_image_write_to_memory(&image,nullptr,&count,0,pixels.data(),0,nullptr),"PNG size query failed");
    Bytes result(count);require(png_image_write_to_memory(&image,result.data(),&count,0,pixels.data(),0,nullptr),"PNG encode failed");result.resize(count);return result;
}
void put32(Bytes& bytes, size_t at, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes[at+i] = uint8_t(value >> (i*8));
}
Bytes ykg(Bytes color, const Bytes& alpha) {
    Bytes result(64);
    std::memcpy(result.data(), "YKG000\0\0", 8);
    put32(result, 8, 64);
    put32(result, 0x28, 64); put32(result, 0x2c, uint32_t(color.size()));
    put32(result, 0x30, uint32_t(64 + color.size())); put32(result, 0x34, uint32_t(alpha.size()));
    result.insert(result.end(), color.begin(), color.end());
    result.insert(result.end(), alpha.begin(), alpha.end());
    return result;
}
Bytes pixels(yukari::Scene& scene, bool draw = true) {
    if (draw) scene.render();
    int w{}, h{};
    require(SDL_GetRendererOutputSize(scene.renderer(), &w, &h) == 0, "renderer dimensions failed");
    require(w == 4 && h == 4, "test requires 4x4 dummy renderer");
    Bytes result(size_t(w)*h*4);
    require(SDL_RenderReadPixels(scene.renderer(), nullptr, SDL_PIXELFORMAT_RGBA32, result.data(), w*4) == 0, "read pixels failed");
    return result;
}
bool near(int actual, int expected) { return actual >= expected - 1 && actual <= expected + 1; }
}

int main(int argc, char** argv) {
    const auto standalone_thumbnail=yukari::Scene::image_thumbnail(png(2,2,100,150,200,128),126,70);
    png_image thumbnail_info{};thumbnail_info.version=PNG_IMAGE_VERSION;
    require(png_image_begin_read_from_memory(&thumbnail_info,standalone_thumbnail.data(),standalone_thumbnail.size()),"standalone thumbnail PNG invalid");
    require(thumbnail_info.width==126&&thumbnail_info.height==70&&SDL_WasInit(SDL_INIT_VIDEO)==0,"image thumbnail required video/window initialization");png_image_free(&thumbnail_info);
    try {
        // Tests are asset independent and run without opening a desktop window.
        const bool gpu = argc == 2 && std::string(argv[1]) == "--gpu";
        if (!gpu) require(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0, "cannot select dummy video driver");
        SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
        yukari::Scene scene(4, 4, "Yukari renderer tests");
        // Exact composition cache: unchanged setters reuse the target; content,
        // geometry and ordering changes must rebuild it, including in-place text.
        scene.load(1,png(2,2,210,20,30));
        const auto first=pixels(scene);auto cache=scene.render_stats();
        scene.set_position(1,0,0);scene.set_alpha(1,255);
        require(pixels(scene)==first,"static composite changed pixels");
        require(scene.render_stats().compositions==cache.compositions &&
                scene.render_stats().composition_reuses==cache.composition_reuses+1,"static composite was redrawn");
        scene.set_position(1,1,0);require(pixels(scene)!=first,"position did not invalidate composite");
        scene.set_visible(1,false);auto hidden=pixels(scene);cache=scene.render_stats();
        scene.set_position(1,2,0);require(pixels(scene)==hidden && scene.render_stats().compositions==cache.compositions,"hidden layer rebuilt composite");
        scene.set_visible(1,true);require(pixels(scene)!=hidden,"visibility did not invalidate composite");
        scene.erase(1);require(pixels(scene)==hidden,"erase retained stale composite");
        yukari::TextStyle cache_style;cache_style.font_size=3;cache_style.border_width=0;
        scene.text(2,"A",0,0,4,4,cache_style);pixels(scene);cache=scene.render_stats();
        scene.text(2,"B",0,0,4,4,cache_style);pixels(scene);
        require(scene.render_stats().compositions==cache.compositions+1,"text update retained stale composite");
        scene.clear();pixels(scene);
        scene.create_canvas(10,2,2,1,1);
        scene.load(11,png(1,1,200,0,0,128));scene.set_visible(11,false);
        const auto paint_stats=scene.render_stats();
        scene.paint(10,11,{0,0,2,2});
        require(scene.render_stats().readbacks==paint_stats.readbacks,"canvas paint used GPU readback");
        auto canvas_pixels=pixels(scene);
        require(near(canvas_pixels[(1*4+1)*4],100)&&canvas_pixels[0]==0,"canvas alpha/placement failed");
        scene.load(12,png(1,1,0,0,200,128));scene.set_visible(12,false);scene.paint(10,12,{1,1,1,1});
        canvas_pixels=pixels(scene);
        require(near(canvas_pixels[(2*4+2)*4],50)&&near(canvas_pixels[(2*4+2)*4+2],100),"straight-alpha canvas over failed");
        const auto canvas_saved=scene.layer_pixels(10);scene.create_canvas(13,2,2,1,1);scene.restore_layer_pixels(13,canvas_saved);
        scene.set_visible(10,false);require(pixels(scene)==canvas_pixels,"canvas pixel snapshot round trip failed");
        scene.set_position(13,0,0);require(near(pixels(scene)[0],100),"painted canvas did not retain movable parent");
        scene.set_alpha(13,0);require(pixels(scene)[0]==0,"painted canvas ignored parent alpha");
        scene.clear();
        scene.load_cached(10,"glyph-mask",ykg(png(2,1,70,80,90),png(2,1,128,128,128)));
        scene.copy(11,10);scene.set_visible(11,false);
        const SDL_Rect glyph_region{1,0,1,1};scene.set_glyph_color(10,0x0000ff,&glyph_region);
        canvas_pixels=pixels(scene);
        require(near(canvas_pixels[0],35)&&near(canvas_pixels[4],127)&&canvas_pixels[5]==0,"native glyph PATCOPY color/mask or region failed");
        scene.set_visible(10,false);scene.set_visible(11,true);
        require(near(pixels(scene)[4],35),"glyph tint mutated a shared original atlas");
        scene.set_image_type(11,3);scene.set_glyph_color(11,0x0000ff);scene.set_image_type(11,2);
        require(near(pixels(scene)[4],35),"native type3 glyph tint must skip");
        scene.clear();
        auto backend = scene.renderer_info();
        std::cout << "renderer=" << backend.name << " accelerated=" << backend.accelerated << " targets=" << backend.target_textures << '\n';
        require(backend.target_textures, "composition targets missing");
        if (gpu) require(backend.accelerated, "GPU test silently fell back to software");
        const SDL_Point first_offsets[]={{0,8},{8,0},{8,-8},{-8,-8},{-4,-4},{-8,-8}};
        for(int mode=1;mode<=6;++mode){
            const auto first=yukari::Scene::effect_offset(mode,8,0);
            require(first.x==first_offsets[mode-1].x&&first.y==first_offsets[mode-1].y,"native Effect first offset differs");
            const auto end=yukari::Scene::effect_offset(mode,8,unsigned(yukari::Scene::effect_steps(mode)));
            require(end.x==0&&end.y==0,"completed Effect must restore origin");
        }
        const SDL_Point table6[]={{-8,-8},{8,-8},{-8,8},{8,8},{-4,-4},{8,8},{8,-8},{-8,-8},{-8,4},{0,-8},{-4,8},{8,-8}};
        for(unsigned step=0;step<12;++step){const auto actual=yukari::Scene::effect_offset(6,8,step);
            require(actual.x==table6[step].x&&actual.y==table6[step].y,"Effect6 coordinate table differs");}
        require(yukari::Scene::effect_step_ms(1,-1,false,false)==100,"Effect1 default delay differs");
        require(yukari::Scene::effect_step_ms(5,-1,false,false)==50,"Effect5 default delay differs");
        require(yukari::Scene::effect_step_ms(3,100,true,false)==25,"whole-duration four-step timing differs");
        require(yukari::Scene::effect_step_ms(6,120,true,false)==10,"whole-duration twelve-step timing differs");
        require(yukari::Scene::effect_step_ms(6,0,true,true)==15,"GPU Effect must retain native minimum sleep");
        const auto forty=scene.transition_timing(2,{40,2,0});
        require(forty.steps==16&&forty.step_ms==20&&forty.total_ms==320,"native forty-unit fade timing differs");
        const auto mask_timing=scene.transition_timing(2,{1000,2,0},true);
        require(mask_timing.steps==256&&mask_timing.total_ms==1000&&mask_timing.total_duration,"native mask deadline timing differs");
        // Cache is a software prefix snapshot; it must survive live prefix edits.
        scene.clear();scene.clear_prefix_cache();scene.load(0,png(4,4,0,0,255));scene.load(1,png(1,1,255,0,0),3,3);
        scene.cache_prefix(0);require(scene.prefix_cached()!=gpu,"GraphicCache backend branch differs");
        scene.load(0,png(4,4,0,255,0));auto cached_pixels=pixels(scene);
        require((gpu?cached_pixels[1]:cached_pixels[2])==255&&cached_pixels[60]==255,"prefix cache did not freeze only its prefix");
        scene.erase(0);cached_pixels=pixels(scene);
        require((gpu?cached_pixels[2]==0:cached_pixels[2]==255),"prefix deletion changed cached content");
        scene.clear_prefix_cache();cached_pixels=pixels(scene);require(cached_pixels[0]==0&&cached_pixels[1]==0&&cached_pixels[2]==0,"cache deletion retained prefix");
        scene.clear();scene.load(10,png(2,2,255,0,0));scene.set_alpha(10,128);scene.load(20,png(2,2,0,255,0));
        require(pixels(scene)[1]==255,"swap setup order differs");scene.swap_layers(10,20);cached_pixels=pixels(scene);
        require(near(cached_pixels[0],128)&&near(cached_pixels[1],127),"GraphicSwap moved z state instead of exchanging native list slots");
        scene.erase(20);scene.swap_layers(10,30);require(scene.size(30).x==2&&pixels(scene)[1]==255,"swap into absent ID lost full node state");
        scene.swap_layers(90,91);bool old_missing=false;try{scene.size(10);}catch(const std::runtime_error&){old_missing=true;}require(old_missing,"swap into absent ID retained old identity");
        // CPU color edits must reach the independently owned GPU texture too.
        scene.clear();scene.load(0,png(4,4,0,0,255));scene.load(1,png(1,1,255,0,0));scene.set_visible(1,false);
        scene.blit(0,1,1,1,{0,0,1,1});cached_pixels=pixels(scene);
        require(cached_pixels[20]==255&&cached_pixels[22]==0,"GraphicCopy did not refresh destination texture");
        const auto copied_bitmap=scene.layer_pixels(0);scene.load(0,png(4,4,0,0,255));scene.restore_layer_pixels(0,copied_bitmap);cached_pixels=pixels(scene);
        require(cached_pixels[20]==255&&cached_pixels[22]==0,"bitmap snapshot lost GraphicCopy pixels");
        auto truncated_bitmap=copied_bitmap;truncated_bitmap.pop_back();bool rejected_bitmap=false;try{scene.restore_layer_pixels(0,truncated_bitmap);}catch(const std::runtime_error&){rejected_bitmap=true;}
        require(rejected_bitmap&&scene.layer_pixels(0)==copied_bitmap,"invalid bitmap snapshot modified layer state");
        if(!gpu){
            scene.clear();scene.load(0,png(4,4,0,0,255));scene.copy(2,0);scene.set_visible(2,false);
            scene.load(1,rgba_png(2,1,{255,0,0,255,0,255,0,255}),100,100);scene.set_alpha(1,0);scene.set_clip(1,{0,0,0,0});
            scene.blit(0,0,0,1,{0,0,2,1});auto image=pixels(scene);
            require(image[0]==255&&image[5]==255,"GraphicCopy incorrectly consumed display alpha/crop/position");
            scene.set_visible(0,false);scene.set_visible(2,true);require(pixels(scene)[2]==255,"blit mutated a shared resource or copied layer");scene.erase(2);scene.set_visible(0,true);
            scene.load(0,png(4,4,0,0,255));scene.blit(0,-1,0,1,{0,0,2,1});image=pixels(scene);require(image[1]==255&&image[6]==255,"negative destination clipping changed source origin");
            scene.load(0,png(4,4,0,0,255));scene.blit(0,0,0,1,{-1,0,3,1});image=pixels(scene);require(image[2]==255&&image[4]==255&&image[9]==255,"negative source clipping changed destination offset");
            scene.load(0,rgba_png(3,1,{255,0,0,255,0,255,0,255,0,0,255,255}));scene.blit(0,1,0,0,{0,0,2,1});image=pixels(scene);
            require(image[0]==255&&image[4]==255&&image[8]==255,"self-blit did not preserve native forward pixel propagation");
            scene.load(0,png(4,4,200,100,50));scene.load(1,ykg(png(1,1,100,150,200),png(1,1,64,128,192)));scene.set_visible(1,false);
            scene.blit(0,0,0,1,{0,0,1,1});image=pixels(scene);require(image[0]==175&&image[1]==125&&image[2]==162,"scalar GraphicCopy lost native per-channel mask/swizzle");
            const auto masked_bitmap=scene.layer_pixels(1);scene.load(1,png(1,1,100,150,200));scene.restore_layer_pixels(1,masked_bitmap);scene.set_visible(1,false);
            scene.load(0,png(4,4,200,100,50));scene.blit(0,0,0,1,{0,0,1,1});image=pixels(scene);
            require(image[0]==175&&image[1]==125&&image[2]==162,"bitmap snapshot lost raw RGB transparency mask");
            scene.load(0,png(4,4,200,100,50));scene.blit(0,0,0,1,{0,0,1,1},yukari::NativePixelKernel::Mmx);image=pixels(scene);
            require(image[0]==125&&image[1]==124&&image[2]==86,"MMX GraphicCopy integer lanes differ");
            for(int type:{4,5}){
                scene.load(0,png(4,4,200,100,50));scene.set_image_type(1,type);scene.blit(0,0,0,1,{0,0,1,1});image=pixels(scene);
                require(type==4?(image[0]==255&&image[1]==250&&image[2]==250):(image[0]==45&&image[1]==0&&image[2]==0),"GraphicCopy ADD/SUB middle-mask arithmetic differs");
                scene.load(1,ykg(png(1,1,100,150,200),png(1,1,255,255,255)));scene.set_visible(1,false);scene.set_image_type(1,type);
                scene.load(0,png(4,4,200,100,50));scene.blit(0,0,0,1,{0,0,1,1});require(pixels(scene)[0]==200,"GraphicCopy ADD/SUB transparent endpoint differs");
                scene.load(1,ykg(png(1,1,100,150,200),png(1,1,0,0,0)));scene.set_visible(1,false);scene.set_image_type(1,type);
                scene.blit(0,0,0,1,{0,0,1,1});require(pixels(scene)[0]==100,"GraphicCopy ADD/SUB opaque endpoint must copy source");
                scene.load(1,ykg(png(1,1,100,150,200),png(1,1,64,128,192)));scene.set_visible(1,false);
            }
            scene.load(0,png(4,4,0,0,255));scene.load(1,rgba_png(2,1,{255,0,0,255,0,255,0,255}));scene.set_visible(1,false);scene.set_image_type(1,3);
            scene.blit(0,0,0,1,{0,0,2,1});image=pixels(scene);require(image[2]==255&&image[5]==255,"native first-DIB-pixel color key was ignored");
            scene.load(0,png(4,4,0,0,255));scene.set_add_color(0,0x0000ff);scene.blit(0,0,0,1,{1,0,1,1});require(pixels(scene)[1]==255,"copy into filtered image did not change current pixels");
            const auto filtered_bitmap=scene.layer_pixels(0);scene.load(0,png(4,4,0,0,255));scene.set_add_color(0,0x0000ff);scene.restore_layer_pixels(0,filtered_bitmap);
            require(pixels(scene)[1]==255,"bitmap snapshot did not retain current filtered/copy result");
            scene.set_add_color(0,0);require(pixels(scene)[2]==255,"copy into filtered image incorrectly rewrote original backup");
            scene.blit(80,0,0,81,{0,0,1,1});
        }
        scene.clear();scene.load(0,png(4,4,255,0,0));auto transition_old=scene.snapshot();
        scene.load(0,png(4,4,0,0,255));
        scene.render_transition(transition_old,1,0.5,png(4,4,0,0,0));auto transition_pixels=pixels(scene,false);
        if(gpu)require(transition_pixels[0]==0&&transition_pixels[2]==255,"GPU mask must use raw blue alpha");
        else require(transition_pixels[0]==255&&transition_pixels[2]==0,"CPU hard mask must keep old below threshold");
        scene.render_transition(transition_old,1,0.5,png(4,4,255,255,255));transition_pixels=pixels(scene,false);
        if(gpu)require(near(transition_pixels[0],127)&&near(transition_pixels[2],128),"GPU mask/diffuse alpha product differs");
        else require(transition_pixels[0]==0&&transition_pixels[2]==255,"CPU hard mask must reveal new above threshold");
        scene.render_transition(transition_old,2,0.5,png(4,4,128,128,128));transition_pixels=pixels(scene,false);
        if(gpu)require(near(transition_pixels[0],64)&&near(transition_pixels[2],191),"GPU UserTransition2 alpha differs");
        else require(transition_pixels[0]==124&&transition_pixels[2]==130,"CPU mask ramp must use signed shift by eight");
        if(gpu){
            const auto before_transition=scene.render_stats();
            for(int mode=1;mode<=32;++mode)scene.render_transition(transition_old,mode,0.5);
            require(scene.render_stats().readbacks==before_transition.readbacks,"accelerated transitions performed screen readback");
            scene.render_transition(transition_old,3,0.5);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]>transition_pixels[12],"native horizontal GPU gradient reversed");
            scene.render_transition(transition_old,5,0.5);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]>transition_pixels[48],"native vertical GPU gradient reversed");
            scene.render_transition(transition_old,19,0.5);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]==0&&transition_pixels[2]==255,"native CPU-only transition must show new GPU scene");
            scene.render_transition(transition_old,29,0.5);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]==255&&transition_pixels[8]==0&&transition_pixels[10]==255,"native old GPU snapshot did not slide left");
            SDL_Rect region{1,1,1,1};scene.render_transition(transition_old,2,0.5,{},&region);transition_pixels=pixels(scene,false);
            require(near(transition_pixels[0],123),"native GPU fade snapshot was incorrectly RECT cropped");
        }
        if(!gpu){
            uint32_t random_state=1;unsigned random_calls=0;
            scene.set_transition_random([&](){++random_calls;random_state=random_state*214013U+2531011U;return (random_state>>16)&32767;});
            const yukari::TransitionSampling total{100,2,2},fine{256,2,0};
            for(int mode=1;mode<=32;++mode){scene.render_transition(transition_old,mode,0.5,{},nullptr,total);
                scene.render_transition(transition_old,mode,1.0,{},nullptr,total);auto end=pixels(scene,false);
                require(end[0]==0&&end[2]==255,"software native transition did not finish");}
            require(random_calls==2,"random block ordering was not generated once per transition");
            {
                yukari::Scene blocks(128,64,"native block order");blocks.load(0,png(128,64,255,0,0));auto old_blocks=blocks.snapshot();blocks.load(0,png(128,64,0,0,255));
                unsigned calls=0;blocks.set_transition_random([&](){++calls;return 0U;});
                blocks.render_transition(old_blocks,19,0.0);Bytes image(128*64*4);
                require(SDL_RenderReadPixels(blocks.renderer(),nullptr,SDL_PIXELFORMAT_RGBA32,image.data(),128*4)==0,"block readback failed");
                require(image[0]==255&&image[64*4+2]==255,"native full-range random swap order differs");
                blocks.render_transition(old_blocks,19,0.1);require(calls==2,"random ordering consumed RNG again during animation");
                blocks.render_transition(old_blocks,19,0.5);require(SDL_RenderReadPixels(blocks.renderer(),nullptr,SDL_PIXELFORMAT_RGBA32,image.data(),128*4)==0,"block readback failed");
                require(image[2]==255&&image[64*4+2]==255,"random block sequence did not accumulate reveals");
            }

            scene.render_transition(transition_old,2,0.5,{},nullptr,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]==127&&transition_pixels[2]==127,"CPU timed fade must use 256 denominator");
            scene.render_transition(transition_old,3,0.5,{},nullptr,fine);transition_pixels=pixels(scene,false);
            require(transition_pixels[2]==255&&transition_pixels[8+2]==255&&transition_pixels[12]==255,"CPU left reveal boundary differs");
            scene.render_transition(transition_old,4,0.5,{},nullptr,fine);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]==255&&transition_pixels[4+2]==255,"CPU right reveal boundary differs");
            scene.render_transition(transition_old,5,0.5,{},nullptr,fine);transition_pixels=pixels(scene,false);
            require(transition_pixels[2]==255&&transition_pixels[48]==255,"CPU top reveal boundary differs");
            scene.render_transition(transition_old,6,0.5,{},nullptr,fine);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]==255&&transition_pixels[16+2]==255,"CPU bottom reveal boundary differs");
            scene.render_transition(transition_old,11,0.5,{},nullptr,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[2]==255&&transition_pixels[4]==255,"CPU staggered column order differs");
            scene.render_transition(transition_old,12,0.5,{},nullptr,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]==255&&transition_pixels[12+2]==255,"CPU reverse staggered column order differs");
            scene.render_transition(transition_old,21,0.5,{},nullptr,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[32]==255&&transition_pixels[36+2]==255&&transition_pixels[2]==255,"CPU quadrant21 motion/reveal differs");
            scene.render_transition(transition_old,22,0.5,{},nullptr,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[44]==255&&transition_pixels[40+2]==255&&transition_pixels[2]==255,"CPU quadrant22 motion/reveal differs");
            scene.render_transition(transition_old,23,0.5,{},nullptr,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[2]==255&&transition_pixels[12]==255&&transition_pixels[32]==255&&transition_pixels[44+2]==255,"CPU split horizontal reveal differs");
            scene.render_transition(transition_old,25,0.5,{},nullptr,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[2]==255&&transition_pixels[48]==255&&transition_pixels[8]==255&&transition_pixels[56+2]==255,"CPU split vertical reveal differs");
            scene.render_transition(transition_old,27,0.0,{},nullptr,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]==255&&transition_pixels[4+2]==255&&transition_pixels[8+2]==255&&transition_pixels[12]==255,"CPU central-column reveal differs");
            scene.render_transition(transition_old,28,0.0,{},nullptr,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[0]==255&&transition_pixels[16+2]==255&&transition_pixels[32+2]==255&&transition_pixels[48]==255,"CPU central-row reveal differs");
            SDL_Rect blend_rect{1,0,1,1};scene.render_transition(transition_old,2,0.5,{},&blend_rect,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[4]==255&&transition_pixels[52]==127&&transition_pixels[54]==127,"native direct-DIB blend row addressing was normalized away");
            scene.render_transition(transition_old,1,0.5,png(4,4,255,255,255),&blend_rect,total);transition_pixels=pixels(scene,false);
            require(transition_pixels[4]==255&&transition_pixels[52]==0&&transition_pixels[54]==255,"native mask DIB row addressing differs");
        }
        scene.render_transition(transition_old,1,1.0,png(4,4,255,255,255));transition_pixels=pixels(scene,false);
        require(transition_pixels[0]==0&&transition_pixels[2]==255,"finished transition retained old frame");
        scene.clear();
        scene.load(1, ykg(png(2, 2, 255, 0, 0), png(2, 2, 255, 255, 255)));
        require(pixels(scene)[0] == 0, "white YKG mask must be transparent");
        scene.load(1, ykg(png(2, 2, 255, 0, 0), png(2, 2, 0, 0, 0)));
        require(pixels(scene)[0] == 255, "black YKG mask must be opaque");
        scene.load(1, ykg(png(2, 2, 255, 0, 0), png(2, 2, 128, 128, 128)));
        require(near(pixels(scene)[0], 127), "gray YKG mask must yield partial opacity");
        scene.load(1, ykg(png(2, 2, 255, 0, 0, 128), png(2, 2, 128, 128, 128)));
        require(near(pixels(scene)[0], 64), "YKG mask must multiply embedded alpha");
        bool rejected{};
        try { scene.load(1, ykg(png(2, 2, 255, 0, 0), png(1, 1, 0, 0, 0))); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "mismatched mask dimensions must fail");

        scene.load(0, png(4, 4, 255, 0, 0));
        scene.load(1, png(2, 2, 0, 255, 0, 128), 1, 1);
        auto result = pixels(scene);
        const size_t center = (1*4+1)*4;
        require(result[0] == 255 && result[1] == 0, "background changed outside sprite");
        require(near(result[center], 127) && near(result[center+1], 128), "embedded alpha blending failed");
        scene.set_visible(1, false);
        require(pixels(scene)[center] == 255, "visibility failed");
        scene.set_visible(1, true); scene.set_z(1, -1);
        require(pixels(scene)[center+1] == 0, "z order failed");
        scene.set_z(1, 2); scene.set_position(1, 0, 0); scene.set_clip(1, {1, 1, 1, 1});
        result = pixels(scene);
        require(result[1] > 0 && result[5] == 0, "sprite crop or position failed");
        require(scene.size(1).x == 2 && scene.size(1).y == 2, "crop must not change source dimensions");
        scene.set_size(1, 3, 3);
        result = pixels(scene);
        require(result[(2*4+2)*4+1] > 0 && result[(3*4+3)*4+1] == 0, "destination resize failed");
        require(scene.size(1).x == 2 && scene.size(1).y == 2, "resize must not change source dimensions");
        scene.set_alpha(1, 0);
        require(pixels(scene)[1] == 0, "layer alpha failed");
        scene.erase(1); scene.clear();
        require(pixels(scene)[0] == 0, "clear failed");
        scene.load(0, png(4, 4, 255, 0, 0));
        auto thumbnail = scene.thumbnail();
        png_image thumb{};
        thumb.version = PNG_IMAGE_VERSION;
        require(png_image_begin_read_from_memory(&thumb, thumbnail.data(), thumbnail.size()), "thumbnail is not PNG");
        require(thumb.width == 126 && thumb.height == 70, "default thumbnail dimensions incorrect");
        thumb.format = PNG_FORMAT_RGBA;
        Bytes rgba(PNG_IMAGE_SIZE(thumb));
        require(png_image_finish_read(&thumb, nullptr, rgba.data(), 0, nullptr), "thumbnail decode failed");
        png_image_free(&thumb);
        require(rgba[0] == 255 && rgba[1] == 0 && rgba[2] == 0 && rgba[3] == 255, "thumbnail pixels incorrect");
        thumbnail = scene.thumbnail(2, 1);
        thumb = {}; thumb.version = PNG_IMAGE_VERSION;
        require(png_image_begin_read_from_memory(&thumb, thumbnail.data(), thumbnail.size()), "custom thumbnail is not PNG");
        require(thumb.width == 2 && thumb.height == 1, "custom thumbnail dimensions incorrect");
        png_image_free(&thumb);
        const auto path = std::filesystem::temp_directory_path() / ("yukari-render-" + std::to_string(SDL_GetPerformanceCounter()) + ".png");
        scene.capture(path);
        auto capture = yukari::read_file(path);
        std::filesystem::remove(path);
        require(capture.size() > 8 && png_sig_cmp(capture.data(), 0, 8) == 0, "capture is not PNG");
        scene.clear();
        auto atlas = png(4, 4, 255, 0, 0);
        {
            unsigned calls=0;auto deferred=[&]{++calls;return atlas;};
            scene.load_deferred(90,"deferred-resource",deferred);auto first=scene.render_stats();scene.erase(90);scene.load_deferred(90,"deferred-resource",deferred);
            require(calls==1&&scene.render_stats().image_decodes==first.image_decodes&&scene.render_stats().resource_cache_hits>first.resource_cache_hits,"released resource was decoded or read again despite retained cache");scene.erase(90);
            for(int i=0;i<65;++i){scene.load_cached(90,"eviction-"+std::to_string(i),atlas);scene.erase(90);}
            scene.load_deferred(90,"deferred-resource",deferred);require(calls==2,"decoded cache did not evict least recently used entries");require(scene.render_stats().resource_cache_bytes<=128ULL*1024*1024,"decoded cache exceeded its budget");scene.erase(90);
        }
        const auto before = scene.render_stats();
        scene.load_cached(10, "atlas", atlas);
        scene.load_cached(11, "atlas", atlas);
        scene.copy(12, 10, 2, 0);
        scene.set_visible(10, false);
        scene.set_clip(11, {0, 0, 2, 4}); scene.set_clip(12, {2, 0, 2, 4});
        scene.set_alpha(10, 0); scene.set_alpha(11, 64); scene.set_alpha(12, 192);
        result = pixels(scene);
        require(near(result[0],64) && near(result[8],192), "shared atlas alpha leaked between layers");
        auto metrics = scene.render_stats();
        require(metrics.uploads == before.uploads+1 && metrics.shared_loads == before.shared_loads+2, "atlas was uploaded more than once");
        auto frozen = scene.snapshot();
        require(scene.render_stats().readbacks == metrics.readbacks, "snapshot performed CPU readback");
        scene.clear(); scene.load(0, png(4,4,0,0,255)); scene.render(); scene.overlay(frozen,128);
        result = pixels(scene,false);
        require(near(result[0],32) && near(result[2],127), "GPU frame crossfade failed");
        scene.render_frozen(frozen,1,0); result = pixels(scene,false);
        require(result[0]==0 && near(result[4],64) && result[2]==0, "GPU frozen-frame offset failed");
        scene.render(); require(pixels(scene,false)[2]==255, "snapshot changed the live scene or render target");
        const auto read_before = scene.render_stats(); scene.thumbnail(2,1);
        require(scene.render_stats().readback_pixels == read_before.readback_pixels+2, "thumbnail read full-resolution pixels back");
        scene.clear();
        scene.load_cached(1, "filter-source", png(1, 1, 240, 100, 20));
        scene.load_cached(2, "filter-source", png(1, 1, 240, 100, 20), 1, 0);
        const auto filter_before = scene.render_stats();
        scene.set_add_color(1, 0x000a141e); // R+30, G+20, B+10.
        result = pixels(scene);
        require(result[0] == 255 && result[1] == 120 && result[2] == 30, "native saturated add or COLORREF channel order failed");
        require(result[4] == 240 && result[5] == 100 && result[6] == 20, "filter mutated a shared source layer");
        scene.set_add_color(1, 0x000a141e);
        require(pixels(scene)[1] == 120, "color setting accumulated instead of recomputing original pixels");
        scene.set_sub_color(1, 0x0032141e); // R-30, G-20, B-50 after saturation.
        result = pixels(scene);
        require(result[0] == 225 && result[1] == 100 && result[2] == 0, "native add/sub ordering or lower clamp failed");
        scene.copy(3, 1, 2, 0);
        scene.set_add_color(1, 0xffffffff); scene.set_sub_color(1, 0);
        result = pixels(scene);
        require(result[0] == 240 && result[1] == 100 && result[2] == 20, "filter disabling did not restore original image");
        require(result[8] == 225 && result[9] == 100 && result[10] == 0, "source filter change mutated a copied image");
        require(scene.render_stats().readbacks == filter_before.readbacks, "color filter read back GPU pixels");
        scene.clear(); scene.load(1, png(1, 1, 100, 200, 40));
        scene.set_sepia(1, true); result = pixels(scene);
        require(result[0] == 172 && result[1] == 150 && result[2] == 111, "native sepia luminance/offset formula failed");
        scene.set_monochrome(1, true); result = pixels(scene);
        require(result[0] == 152 && result[1] == 152 && result[2] == 152, "native sepia-before-monochrome order failed");
        scene.set_negaposi(1, true); result = pixels(scene);
        require(result[0] == 103 && result[1] == 103 && result[2] == 103, "native negaposi-last order failed");
        scene.set_sepia(1, false); scene.set_negaposi(1, false); result = pixels(scene);
        require(result[0] == 154 && result[1] == 154 && result[2] == 154, "native monochrome 3:6:1 weights failed");
        scene.set_monochrome(1, false); result = pixels(scene);
        require(result[0] == 100 && result[1] == 200 && result[2] == 40, "disabling image effects did not restore original RGB");
        scene.load(1, png(1, 1, 100, 200, 40, 128)); scene.set_negaposi(1, true); result = pixels(scene);
        require(near(result[0], 77) && near(result[1], 27) && near(result[2], 107), "RGB filters changed source alpha");
        scene.clear();scene.load(1,png(1,4,255,0,0));scene.set_raster(1,true,2,2,0.75);result=pixels(scene);
        require(result[0]==255&&result[(1*4+1)*4]==255&&result[(2*4)*4]==255&&result[(3*4)*4]==0,
                "native raster row phase/height/truncation differs");
        scene.set_raster(1,false,2,2,0);require(pixels(scene)[3*4*4]==255,"disabled raster retained row displacement");
        scene.set_raster(1,true,2,2,0);scene.set_scale(1,2000,1000);result=pixels(scene);
        require(result[(1*4)*4]==255&&result[(1*4+2)*4]==0,"scale did not suppress native raster");
        scene.clear(); scene.load(1, png(1,1,255,0,0), 1, 1);
        scene.set_scale(1, 2000, 2000); result = pixels(scene);
        require(result[(1*4+1)*4] == 255 && result[(2*4+2)*4] == 255 && result[(3*4+3)*4] == 0,
                "native scale-per-mille or default origin failed");
        scene.set_scale_center(1, 1, 1); result = pixels(scene);
        require(result[0] == 255 && result[(1*4+1)*4] == 255 && result[(2*4+2)*4] == 0,
                "independent scale center failed");
        scene.set_scale(1, 1000, 1000); scene.set_position(1, 2, 0); scene.set_rotate(1, 500);
        result = pixels(scene);
        require(result[4] == 255 && result[8] == 0, "native 500-unit clockwise quarter turn failed");
        scene.set_rotate(1, 0); scene.set_scale_center(1, 0, 0); scene.set_scale(1, -1000, 1000);
        require(pixels(scene)[4] == 255, "negative scale mirror failed");
        scene.set_position(1, 0, 0); scene.set_scale(1, 2000, 2000); scene.set_alpha(1, 128);
        require(near(pixels(scene)[0], 128), "transformed layer alpha was lost or applied twice");
        scene.set_alpha(1, 255); scene.set_position(1, 2, 2); scene.set_scale(1, 2000, 1000);
        scene.set_rotate_center(1, 1, 0); scene.set_rotate(1, 500); result = pixels(scene);
        require(result[(1*4+2)*4] == 255 && result[(2*4+2)*4] == 255 && result[(2*4+3)*4] == 0,
                "local scale/rotation center composition order failed");
        for (int mode = 1; mode <= 4; ++mode) {
            scene.clear(); scene.load(0, png(4,4,100,200,40)); scene.create_filter(1, mode);
            require(scene.filter_type(1) == 10000+mode, "native GraphicFilter type not retained");
            scene.load(2, png(1,1,255,0,0), 3, 3);
            const auto prefix_before = scene.render_stats(); result = pixels(scene);
            if (scene.renderer_info().accelerated || mode == 4)
                require(result[0] == 100 && result[1] == 200 && result[2] == 40, "native GPU/type4 GraphicFilter skip changed prefix");
            else if (mode == 1)
                require(result[0] == 172 && result[1] == 150 && result[2] == 111, "prefix sepia failed");
            else if (mode == 2)
                require(result[0] == 154 && result[1] == 154 && result[2] == 154, "prefix monochrome failed");
            else
                require(result[0] == 155 && result[1] == 55 && result[2] == 215, "prefix negaposi failed");
            require(result[(3*4+3)*4] == 255 && result[(3*4+3)*4+1] == 0, "prefix filter changed a layer above it");
            require(scene.renderer_info().accelerated || mode == 4 || scene.render_stats().readbacks == prefix_before.readbacks+1,
                    "software prefix filter did not account for its CPU readback");
            if (scene.renderer_info().accelerated || mode == 4)
                require(scene.render_stats().readbacks == prefix_before.readbacks, "native GPU/type4 filter skip read back pixels");
            scene.set_visible(1,false); result = pixels(scene);
            require(result[0] == 100 && result[1] == 200 && result[2] == 40, "hidden filter still affected prefix");
        }
        for (int type : {4,5}) {
            scene.clear(); scene.load(0,png(4,4,100,100,100)); scene.load(1,png(1,1,200,60,60,128));
            scene.set_image_type(1,type);
            require(scene.image_type(1)==type,"native image blend type was lost");
            const auto blend_before=scene.render_stats();result=pixels(scene);
            if(scene.renderer_info().accelerated) {
                require(near(result[0],150)&&near(result[1],80)&&near(result[2],80),"native GPU ADD/SUB branch must retain normal alpha");
                require(scene.render_stats().readbacks==blend_before.readbacks,"GPU ADD/SUB performed software readback");
            } else if(type==4) require(result[0]==255&&result[1]==160&&result[2]==160,"software ADD middle-mask saturation failed");
            else require(result[0]==45&&result[1]==0&&result[2]==0,"software SUB must use D+S-255, not D-S");
            scene.load(1,png(1,1,200,60,60));scene.set_image_type(1,type);result=pixels(scene);
            require(result[0]==200&&result[1]==60&&result[2]==60,"opaque mask endpoint must copy for ADD/SUB");
            scene.load(1,png(1,1,200,60,60,0));scene.set_image_type(1,type);result=pixels(scene);
            require(result[0]==100&&result[1]==100&&result[2]==100,"transparent mask endpoint must skip ADD/SUB");
            scene.load(1,png(1,1,200,60,60,128));scene.set_image_type(1,type);scene.set_alpha(1,0);result=pixels(scene);
            require(result[0]==100&&result[1]==100&&result[2]==100,"fully transparent graphic layer must skip ADD/SUB");
        }
        scene.clear();
        const auto external_source=png(1,1,255,0,0), external_white=png(1,1,255,255,255), external_black=png(1,1,0,0,0);
        const auto mask_before=scene.render_stats();
        scene.load_cached(1,"external-mask",external_source,0,0,external_white);
        scene.load_cached(2,"external-mask",external_source,1,0,external_black);
        scene.load_cached(3,"external-mask",external_source,2,0,external_white);
        result=pixels(scene);
        require(result[0]==0&&result[4]==255&&result[8]==0,"external mask cache contaminated a different mask variant");
        require(scene.render_stats().uploads==mask_before.uploads+2&&scene.render_stats().shared_loads==mask_before.shared_loads+1,
                "mask-aware image cache did not separate/share the correct variants");
        scene.load(4,png(1,1,255,0,0,128),3,0,png(1,1,128,128,128));
        require(near(pixels(scene)[12],64),"external mask did not multiply existing embedded alpha");
        bool mask_rejected=false;
        try{scene.load(4,external_source,0,0,png(2,1,0,0,0));}catch(const std::runtime_error&){mask_rejected=true;}
        require(mask_rejected,"external mask size mismatch was accepted");

        yukari::TextStyle measure_style;measure_style.font_size=24;measure_style.line_height=36;measure_style.border_width=0;
        const auto single=scene.measure_text("日",measure_style),pair=scene.measure_text("日日",measure_style);
        require(single.x>0&&single.y==36&&pair.x==single.x*2,"FreeType text advance measurement failed");
        const auto multiline=scene.measure_text("日日\n日",measure_style);
        require(multiline.x==pair.x&&multiline.y==72,"text measurement newline extent failed");
        measure_style.letter_spacing=3;
        require(scene.measure_text("日日",measure_style).x==pair.x+6,"text measurement ignored letter spacing");
        measure_style.border_width=2;measure_style.shadow_offset={3,3};
        require(scene.measure_text("日日",measure_style).x==pair.x+6,"logical text extent included effect padding");
        require(scene.measure_text("",measure_style).x==0&&scene.measure_text("",measure_style).y==0,"empty text extent must be zero");
        yukari::Scene::Frame expired;
        { yukari::Scene other(4,4,"frame ownership");
          bool bad_owner=false;try {other.overlay(frozen);}catch(const std::runtime_error&){bad_owner=true;}
          require(bad_owner,"cross-renderer frame accepted");expired=other.snapshot(); }
        require(!expired,"frame outlived its renderer without invalidation");
        std::cout << "renderer mask, sprite and capture tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

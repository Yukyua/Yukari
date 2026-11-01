#include "yukari/render.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
constexpr int width = 240, height = 144;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
yukari::Bytes pixels(yukari::Scene& scene) {
    scene.render();
    int actual_width{},actual_height{};require(SDL_GetRendererOutputSize(scene.renderer(),&actual_width,&actual_height)==0&&actual_width==width&&actual_height==height,"text test drawable dimensions differ from pixel buffer");
    yukari::Bytes result(width*height*4);
    require(SDL_RenderReadPixels(scene.renderer(), nullptr, SDL_PIXELFORMAT_RGBA32, result.data(), width*4) == 0,
            "cannot read text pixels");
    return result;
}
size_t lit(const yukari::Bytes& bytes, int y0, int y1, int x0 = 0, int x1 = width) {
    size_t count{};
    for (int y = y0; y < y1; ++y) for (int x = x0; x < x1; ++x) {
        const size_t at = size_t(y*width+x)*4;
        if (bytes[at] || bytes[at+1] || bytes[at+2]) ++count;
    }
    return count;
}
}
int main(int argc,char** argv) {
    try {
        const bool gpu=argc>1&&std::string(argv[1])=="--gpu";
        if(!gpu)require(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0, "cannot select dummy driver");
        if(gpu)SDL_SetHint(SDL_HINT_RENDER_DRIVER,"metal");
        SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED,"1");
        require(std::filesystem::is_regular_file(yukari::Scene::default_font_path()), "Japanese system font missing");
        yukari::Scene scene(width, height, "Japanese text tests");
        if(gpu)require(scene.renderer_info().name=="metal","text GPU test requires Metal");
        yukari::TextStyle style;
        style.font_size = 24; style.line_height = 36; style.border_width = 0;
        scene.text(1, "日本\n語", 0, 0, width, height, style);
        auto result = pixels(scene);
        require(lit(result, 0, 36) > 30 && lit(result, 36, 72) > 30, "Japanese text or explicit newline missing");
        require(lit(result, 72, height) == 0, "unexpected extra text lines");
        scene.text(1, "日本語", 0, 0, 27, height, style);
        result = pixels(scene);
        require(lit(result, 0, 36) > 10 && lit(result, 36, 72) > 10 && lit(result, 72, 108) > 10,
                "CJK character wrapping failed");
        require(lit(result, 0, height, 27, width) == 0, "text escaped its box");

        style.color = {255, 0, 0, 255}; style.border_color = {0, 255, 0, 255}; style.border_width = 2;
        style.shadow_color = {0, 0, 255, 255}; style.shadow_offset = {5, 5};
        scene.text(1, "雪月花", 4, 4, 180, 80, style);
        result = pixels(scene);
        size_t red{}, green{}, blue{};
        for (size_t i = 0; i < result.size(); i += 4) {
            if (result[i] > result[i+1] && result[i] > result[i+2]) ++red;
            if (result[i+1] > result[i] && result[i+1] > result[i+2]) ++green;
            if (result[i+2] > result[i] && result[i+2] > result[i+1]) ++blue;
        }
        require(red > 10 && green > 10 && blue > 10, "foreground, border or shadow color missing");
        require(scene.size(1).x == 180 && scene.size(1).y == 80, "text layer dimensions incorrect");
        const auto uploads = scene.render_stats().uploads;
        scene.text(1, "雪月", 4, 4, 180, 80, style);
        require(scene.render_stats().uploads == uploads && scene.render_stats().texture_updates > 0,
                "same-size text replaced its GPU texture");
        scene.copy(2, 1); scene.set_visible(2, false);
        scene.text(1, "新しい文字", 4, 4, 180, 80, style);
        require(scene.render_stats().uploads == uploads+1, "text update mutated a shared texture");
        scene.erase(2);
        bool rejected{};
        try { scene.text(1, std::string("\xc0\xaf", 2), 0, 0, 50, 50, style); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "invalid UTF-8 must be rejected");
        scene.text(1, "", 0, 0, width, height, style);
        require(lit(pixels(scene), 0, height) == 0, "empty text layer must be transparent");
        // Absolute native glyph positions must survive batching and per-run styles.
        style.border_width=0;style.shadow_offset={0,0};style.color={255,255,255,255};
        scene.text(1,"日本",16,12,200,100,style);const auto reference=pixels(scene);
        scene.positioned_text(1,{{"日本",16,12,style}},{0,0,width,height});
        require(pixels(scene)==reference,"absolute positioned text changed glyph cell origin");
        const int advance=scene.measure_text("日",style).x;
        scene.positioned_text(1,{{"日",16,12,style},{"本",16+advance,12,style}},{0,0,width,height});
        require(pixels(scene)==reference,"separate glyph runs were reshaped or displaced");
        const auto glyph_stats=scene.render_stats();
        scene.positioned_text(1,{{"日",16,12,style},{"本",16+advance,12,style}},{0,0,width,height});
        const auto cached_stats=scene.render_stats();
        require(cached_stats.glyph_rasterizations==glyph_stats.glyph_rasterizations&&cached_stats.text_layout_reuses==glyph_stats.text_layout_reuses+1&&cached_stats.texture_updates==glyph_stats.texture_updates,
                "positioned glyph runs were rerasterized despite cache hits");
        auto red_style=style,green_style=style;red_style.color={255,0,0,255};green_style.color={0,255,0,255};green_style.font_size=14;
        scene.positioned_text(1,{{"雪",20,10,red_style},{"花",140,60,green_style}},{20,10,160,100});
        result=pixels(scene);red=green=0;
        for(int y=0;y<height;++y)for(int x=0;x<width;++x){const auto at=size_t(y*width+x)*4;
            if(result[at]>0&&result[at+1]==0){++red;require(x>=20&&x<70&&y>=10&&y<50,"first run lost absolute placement");}
            if(result[at+1]>0&&result[at]==0){++green;require(x>=140&&x<170&&y>=60&&y<90,"second run lost its own placement/font size");}}
        require(red>10&&green>10,"per-run font colors were overwritten");
        scene.positioned_text(1,{{"日本語日本語日本語",160,10,style}},{160,10,20,90});result=pixels(scene);
        require(lit(result,10,45)>0&&lit(result,45,height)==0,"positioned run was automatically rewrapped");
        require(lit(result,0,height,0,160)==0&&lit(result,0,height,180,width)==0,"positioned run escaped absolute clipping");
        const auto positioned_stats=scene.render_stats();
        scene.positioned_text(1,{{"雪",160,10,style}},{160,10,20,90});
        require(scene.render_stats().uploads==positioned_stats.uploads&&scene.render_stats().texture_updates>positioned_stats.texture_updates,
                "positioned text failed to reuse its single GPU texture");
        scene.positioned_text(1,{}, {0,0,width,height});require(lit(pixels(scene),0,height)==0,"empty positioned runs must clear the layer");
        std::cout << "Japanese UTF-8, newline, wrap, border and shadow tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

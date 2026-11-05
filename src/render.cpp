#include "yukari/render.hpp"
#include "yukari/gpu_ops.hpp"
#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>
#include <tuple>

namespace yukari {
namespace {
using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;
using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
void apply_alpha_mask(SDL_Surface* color, SDL_Surface* mask) {
    if (mask->w != color->w || mask->h != color->h)
        throw std::runtime_error("color and alpha mask dimensions differ");
    // Current independent-mask rule is inferred from original samples/layout;
    // the native compositor's full mask arithmetic remains a separate question.
    for (int y = 0; y < color->h; ++y) {
        auto* c = static_cast<uint8_t*>(color->pixels)+y*color->pitch;
        const auto* m = static_cast<const uint8_t*>(mask->pixels)+y*mask->pitch;
        for (int x=0;x<color->w;++x,c+=4,m+=4) {
            const unsigned opacity=255-(unsigned(m[0])+m[1]+m[2])/3;
            c[3]=uint8_t((unsigned(c[3])*opacity+127)/255);
        }
    }
}
Surface decode_image(Bytes bytes) {
    if (bytes.size() >= 6 && std::memcmp(bytes.data(), "YKG000", 6) == 0) {
        auto graphic = parse_graphic(bytes);
        auto color = decode_image(std::move(graphic.color));
        if (!graphic.alpha.empty()) {
            auto mask = decode_image(std::move(graphic.alpha));
            apply_alpha_mask(color.get(),mask.get());
        }
        return color;
    }
    if (bytes.size() >= 4 && std::memcmp(bytes.data(), "\x89GNP", 4) == 0) {
        bytes[1] = 'P'; bytes[2] = 'N'; bytes[3] = 'G';
    }
    if (bytes.size() >= 8 && png_sig_cmp(bytes.data(), 0, 8) == 0) {
        png_image png{};
        png.version = PNG_IMAGE_VERSION;
        struct Cleanup { png_image* p; ~Cleanup() { png_image_free(p); } } cleanup{&png};
        if (!png_image_begin_read_from_memory(&png, bytes.data(), bytes.size()))
            throw std::runtime_error("invalid PNG header");
        if (!png.width || !png.height || png.width > 16384 || png.height > 16384 || uint64_t(png.width)*png.height > 64000000)
            throw std::runtime_error("PNG dimensions exceed limit");
        png.format = PNG_FORMAT_RGBA;
        Bytes rgba(PNG_IMAGE_SIZE(png));
        if (!png_image_finish_read(&png, nullptr, rgba.data(), 0, nullptr))
            throw std::runtime_error("invalid PNG payload");
        Surface surface(SDL_CreateRGBSurfaceWithFormat(0, int(png.width), int(png.height), 32, SDL_PIXELFORMAT_RGBA32), SDL_FreeSurface);
        if (!surface) throw std::runtime_error(SDL_GetError());
        for (unsigned y = 0; y < png.height; ++y)
            std::memcpy(static_cast<uint8_t*>(surface->pixels) + y*surface->pitch,
                        rgba.data() + size_t(y)*png.width*4, size_t(png.width)*4);
        return surface;
    }
    if (bytes.size() >= 26 && bytes[0] == 'B' && bytes[1] == 'M') {
        if (u32(bytes, 14) < 40) throw std::runtime_error("legacy BMP DIB header unsupported");
        const auto w = u32(bytes, 18), raw_h = u32(bytes, 22);
        int32_t signed_h;
        std::memcpy(&signed_h, &raw_h, 4);
        int64_t h = signed_h < 0 ? -int64_t(signed_h) : signed_h;
        if (!w || !h || w > 16384 || h > 16384 || uint64_t(w)*h > 64000000 || bytes.size() > INT32_MAX)
            throw std::runtime_error("BMP dimensions exceed limit");
        auto* rw = SDL_RWFromConstMem(bytes.data(), int(bytes.size()));
        if (!rw) throw std::runtime_error(SDL_GetError());
        Surface surface(SDL_LoadBMP_RW(rw, 1), SDL_FreeSurface);
        if (!surface) throw std::runtime_error(SDL_GetError());
        Surface rgba(SDL_ConvertSurfaceFormat(surface.get(), SDL_PIXELFORMAT_RGBA32, 0), SDL_FreeSurface);
        if (!rgba) throw std::runtime_error(SDL_GetError());
        return rgba;
    }
    throw std::runtime_error("unsupported image payload (expected YKG/BMP/PNG/GNP)");
}
Surface decode_image(Bytes bytes, const Bytes& extra_mask) {
    auto color=decode_image(std::move(bytes));
    if(!extra_mask.empty()) { auto mask=decode_image(extra_mask);apply_alpha_mask(color.get(),mask.get()); }
    return color;
}
Bytes encode_surface(const SDL_Surface* surface) {
    if(!surface)return {};
    png_image png{};png.version=PNG_IMAGE_VERSION;png.width=unsigned(surface->w);png.height=unsigned(surface->h);png.format=PNG_FORMAT_RGBA;
    png_alloc_size_t count{};
    if(!png_image_write_to_memory(&png,nullptr,&count,0,surface->pixels,surface->pitch,nullptr))throw std::runtime_error(std::string("cannot size bitmap PNG: ")+png.message);
    Bytes bytes(count);
    if(!png_image_write_to_memory(&png,bytes.data(),&count,0,surface->pixels,surface->pitch,nullptr))throw std::runtime_error(std::string("cannot encode bitmap PNG: ")+png.message);
    bytes.resize(count);return bytes;
}
void check(int result) { if (result < 0) throw std::runtime_error(SDL_GetError()); }
void color_effects(uint8_t* pixel, bool sepia, bool monochrome, bool negaposi) {
    if (sepia) {
        // Native 0x408d30, not the commonly used sepia matrix.
        const float luminance = (float(pixel[2])/255.0)*0.114 + (float(pixel[0])/255.0)*0.299 +
                                (float(pixel[1])/255.0)*0.587;
        const float red = luminance + 0.078512;
        const float green = (luminance - -0.031304) - 0.039984;
        const float blue = luminance - 0.16125199;
        const auto byte = [](float v) { return uint8_t(std::lrint(std::clamp(v, 0.0f, 1.0f)*255.0f)); };
        pixel[0] = byte(red); pixel[1] = byte(green); pixel[2] = byte(blue);
    }
    if (monochrome) {
        // Native 0x408f90 uses integer weights 3:6:1 and truncates.
        const auto gray = uint8_t((unsigned(pixel[0])*3+unsigned(pixel[1])*6+pixel[2])/10);
        pixel[0] = pixel[1] = pixel[2] = gray;
    }
    if (negaposi) for (unsigned c = 0; c < 3; ++c) pixel[c] = uint8_t(255-pixel[c]);
}

std::vector<uint32_t> codepoints(const std::string& text) {
    if (text.size() > 1024 * 1024) throw std::runtime_error("text input exceeds limit");
    std::vector<uint32_t> result;
    for (size_t i = 0; i < text.size();) {
        const auto first = uint8_t(text[i++]);
        uint32_t value = first;
        unsigned remaining{};
        uint32_t minimum{};
        if (first < 0x80) {}
        else if (first >= 0xc2 && first <= 0xdf) { value &= 0x1f; remaining = 1; minimum = 0x80; }
        else if (first >= 0xe0 && first <= 0xef) { value &= 0xf; remaining = 2; minimum = 0x800; }
        else if (first >= 0xf0 && first <= 0xf4) { value &= 7; remaining = 3; minimum = 0x10000; }
        else throw std::runtime_error("invalid UTF-8 text");
        while (remaining--) {
            if (i >= text.size() || (uint8_t(text[i]) & 0xc0) != 0x80) throw std::runtime_error("invalid UTF-8 text");
            value = (value << 6) | (uint8_t(text[i++]) & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            throw std::runtime_error("invalid UTF-8 code point");
        result.push_back(value);
    }
    return result;
}
struct TextRasterizer {
    NativeFontRasterizer native_fonts;
    std::filesystem::path default_font;
    FT_Library library{};
    uint64_t cache_hits{},rasterizations{};
    std::map<std::filesystem::path, FT_Face> faces;
    struct Glyph {int advance{},left{},top{},width{},height{};Bytes coverage;};
    std::map<std::tuple<uintptr_t,unsigned,FT_UInt>,std::shared_ptr<Glyph>> glyphs;
    std::shared_ptr<const Glyph> glyph(FT_Face face,FT_UInt index){
        const auto key=std::make_tuple(reinterpret_cast<uintptr_t>(face),unsigned(face->size->metrics.y_ppem),index);
        auto found=glyphs.find(key);if(found!=glyphs.end()){++cache_hits;return found->second;}
        ++rasterizations;
        if(FT_Load_Glyph(face,index,FT_LOAD_DEFAULT)||FT_Render_Glyph(face->glyph,FT_RENDER_MODE_NORMAL))throw std::runtime_error("cannot rasterize font glyph");
        auto* source=face->glyph;const auto& bitmap=source->bitmap;
        if(bitmap.pixel_mode!=FT_PIXEL_MODE_GRAY&&bitmap.pixel_mode!=FT_PIXEL_MODE_MONO&&bitmap.width)throw std::runtime_error("unsupported font bitmap format");
        Glyph value;value.advance=int((source->advance.x+32)>>6);value.left=source->bitmap_left;value.top=source->bitmap_top;
        value.width=int(bitmap.width);value.height=int(bitmap.rows);value.coverage.resize(size_t(value.width)*value.height);
        for(unsigned y=0;y<bitmap.rows;++y){const auto* row=bitmap.pitch>=0?bitmap.buffer+y*bitmap.pitch:bitmap.buffer+(bitmap.rows-y-1)*(-bitmap.pitch);
            for(unsigned x=0;x<bitmap.width;++x)value.coverage[size_t(y)*bitmap.width+x]=bitmap.pixel_mode==FT_PIXEL_MODE_MONO?((row[x/8]&(0x80>>(x%8)))?255:0):row[x];}
        if(glyphs.size()>=8192)glyphs.clear();
        return glyphs.emplace(key,std::make_shared<Glyph>(std::move(value))).first->second;
    }
    TextRasterizer() { if (FT_Init_FreeType(&library)) throw std::runtime_error("cannot initialize FreeType"); }
    ~TextRasterizer() {
        for (auto& face : faces) FT_Done_Face(face.second);
        FT_Done_FreeType(library);
    }
    std::shared_ptr<const NativeFontBitmap> native_cell(const std::string& text,const TextStyle& style){
        const uint32_t foreground=uint32_t(style.color.r)|(uint32_t(style.color.g)<<8)|(uint32_t(style.color.b)<<16);
        const auto& fallback=style.font_path.empty()?default_font:style.font_path;
        return native_fonts.rasterize(text,fallback.empty()?Scene::default_font_path():fallback,*style.native_font,foreground);
    }
    Surface native_bitmap(const NativeFontBitmap& cell){
        const auto& color=cell.planes.color;const auto& mask=cell.planes.transparency;
        Surface out(SDL_CreateRGBSurfaceWithFormat(0,std::max(1,color.width),std::max(1,color.height),32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);if(!out)throw std::runtime_error(SDL_GetError());check(SDL_FillRect(out.get(),nullptr,0));
        for(int y=0;y<color.height;++y){auto* row=static_cast<uint8_t*>(out->pixels)+y*out->pitch;for(int x=0;x<color.width;++x){auto c=color.pixels[size_t(y)*color.width+x],m=mask.pixels[size_t(y)*color.width+x];if((m&255)!=((m>>8)&255)||(m&255)!=((m>>16)&255))throw std::runtime_error("RGB subpixel GDI mask requires per-channel text composition");row[x*4]=uint8_t(c>>16);row[x*4+1]=uint8_t(c>>8);row[x*4+2]=uint8_t(c);row[x*4+3]=uint8_t(255-(m&255));}}
        return out;
    }
    static void over(SDL_Surface* destination,const SDL_Surface* source,int64_t x,int64_t y){
        const int64_t x0=std::max<int64_t>(0,-x),y0=std::max<int64_t>(0,-y),x1=std::min<int64_t>(source->w,int64_t(destination->w)-x),y1=std::min<int64_t>(source->h,int64_t(destination->h)-y);
        for(int64_t sy=y0;sy<y1;++sy){auto* dst=static_cast<uint8_t*>(destination->pixels)+(sy+y)*destination->pitch;const auto* src=static_cast<const uint8_t*>(source->pixels)+sy*source->pitch;for(int64_t sx=x0;sx<x1;++sx){const auto* pixel=src+sx*4;const unsigned a=pixel[3];if(!a)continue;auto* target=dst+(sx+x)*4;const unsigned tail=(unsigned(target[3])*(255-a)+127)/255,out=a+tail;for(int c=0;c<3;++c)target[c]=uint8_t((unsigned(pixel[c])*a+unsigned(target[c])*tail+out/2)/out);target[3]=uint8_t(out);}}
    }
    std::vector<PositionedText> native_runs(const std::string& text,int width,int height,const TextStyle& style){
        if(width<=0||height<=0||width>16384||height>16384||uint64_t(width)*height>16000000)throw std::runtime_error("native text bounds exceed limit");
        codepoints(text);std::vector<PositionedText> runs;int x=0,y=0;const int line=style.line_height?style.line_height:std::max(1,std::abs(style.native_font->height));
        for(size_t at=0;at<text.size();){const unsigned char first=text[at];size_t length=first<128?1:first<224?2:first<240?3:4;auto part=text.substr(at,length);at+=length;if(part=="\r")continue;if(part=="\n"){x=0;y+=line;continue;}if(part=="\t")part=" ";auto cell=native_cell(part,style);if(x&&int64_t(x)+cell->advance>width){x=0;y+=line;}if(y>=height)break;runs.push_back({part,x,y,style});x+=cell->advance+style.letter_spacing;}
        return runs;
    }
    Surface native_text(const std::string& text,int width,int height,const TextStyle& style){return positioned(native_runs(text,width,height,style),{0,0,width,height});}
    FT_Face select_face(const TextStyle& style) {
        if (style.font_size < 1 || style.font_size > 512 || style.border_width < 0 || style.border_width > 8 ||
            style.line_height < 0 || style.line_height > 4096 || style.letter_spacing < -512 || style.letter_spacing > 512 ||
            style.shadow_offset.x < -512 || style.shadow_offset.x > 512 || style.shadow_offset.y < -512 || style.shadow_offset.y > 512)
            throw std::runtime_error("text layout exceeds limits");
        const auto path = style.font_path.empty() ?
            (default_font.empty()?Scene::default_font_path():default_font) : style.font_path;
        auto found = faces.find(path);
        if (found == faces.end()) {
            FT_Face face{};
            if (FT_New_Face(library, path.string().c_str(), 0, &face))
                throw std::runtime_error("cannot load font: " + path.string());
            try { found = faces.emplace(path, face).first; }
            catch (...) { FT_Done_Face(face); throw; }
        }
        auto face = found->second;
        if (FT_Set_Pixel_Sizes(face, 0, unsigned(style.font_size))) throw std::runtime_error("cannot set font size");
        return face;
    }
    SDL_Point measure(const std::string& text, const TextStyle& style) {
        if(style.native_font){
            codepoints(text);if(text.empty())return {0,0};size_t begin=0;int maximum=0,lines=0;for(;;){auto end=text.find('\n',begin);auto line=text.substr(begin,end==std::string::npos?end:end-begin);line.erase(std::remove(line.begin(),line.end(),'\r'),line.end());if(!line.empty())maximum=std::max(maximum,native_cell(line,style)->advance);++lines;if(end==std::string::npos)break;begin=end+1;}
            return {maximum,lines*(style.line_height?style.line_height:std::max(1,std::abs(style.native_font->height)))};
        }
        auto points=codepoints(text);auto face=select_face(style);
        if(points.empty())return {0,0};
        const int line_height=style.line_height?style.line_height:std::max(1,int((face->size->metrics.height+63)>>6));
        int64_t pen=0,maximum=0,lines=1;FT_UInt previous{};
        for(auto cp:points) {
            if(cp=='\r')continue;
            if(cp=='\n'){maximum=std::max(maximum,pen);pen=0;previous=0;++lines;continue;}
            if(cp=='\t')cp=' ';
            auto index=FT_Get_Char_Index(face,cp);
            const auto& cached=glyph(face,index);
            if(previous&&index&&FT_HAS_KERNING(face)){FT_Vector delta{};if(!FT_Get_Kerning(face,previous,index,FT_KERNING_DEFAULT,&delta))pen+=delta.x>>6;}
            pen+=std::max(1,cached->advance+style.letter_spacing);previous=index;
        }
        maximum=std::max(maximum,pen);
        if(maximum>INT32_MAX||lines*line_height>INT32_MAX)throw std::runtime_error("measured text extent exceeds limit");
        return {int(maximum),int(lines*line_height)};
    }
    Surface rasterize(const std::string& text, int width, int height, const TextStyle& style) {
        if(width<=0||height<=0||width>16384||height>16384||uint64_t(width)*height>16000000)
            throw std::runtime_error("text layout exceeds limits");
        if(style.native_font)return native_text(text,width,height,style);
        const auto points=codepoints(text);auto face=select_face(style);
        const int line_height = style.line_height ? style.line_height : std::max(1, int((face->size->metrics.height + 63) >> 6));
        const int inset_x = style.border_width + std::max(0, -style.shadow_offset.x);
        const int inset_y = style.border_width + std::max(0, -style.shadow_offset.y);
        const int right = width - style.border_width - std::max(0, style.shadow_offset.x);
        int pen_x = inset_x, baseline = inset_y + int((face->size->metrics.ascender + 63) >> 6);
        Bytes mask(size_t(width)*height);
        FT_UInt previous{};
        for (auto cp : points) {
            if (cp == '\r') continue;
            if (cp == '\n') { pen_x = inset_x; baseline += line_height; previous = 0; continue; }
            if (baseline - style.font_size > height) break;
            if (cp == '\t') cp = ' ';
            const auto index = FT_Get_Char_Index(face, cp);
            const auto& cached=glyph(face,index);
            int kerning{};
            if (previous && index && FT_HAS_KERNING(face)) {
                FT_Vector delta{};
                if (!FT_Get_Kerning(face, previous, index, FT_KERNING_DEFAULT, &delta)) kerning = int(delta.x >> 6);
            }
            const int advance = std::max(1, cached->advance + style.letter_spacing);
            if (pen_x != inset_x && pen_x + kerning + advance > right) {
                pen_x = inset_x; baseline += line_height; kerning = 0;
            }
            pen_x += kerning;
            for(int row=0;row<cached->height;++row){
                const int py=baseline-cached->top+row;if(py<0||py>=height)continue;
                for(int col=0;col<cached->width;++col){
                    const int px=pen_x+cached->left+col;if(px<0||px>=width)continue;
                    const unsigned coverage=cached->coverage[size_t(row)*cached->width+col];auto& target=mask[size_t(py)*width+px];
                    target=uint8_t(coverage+(unsigned(target)*(255-coverage)+127)/255);
                }
            }
            pen_x += advance; previous = index;
        }
        return decorate(mask,width,height,style);
    }
    Surface positioned(const std::vector<PositionedText>& runs,SDL_Rect clip){
        if(clip.w<=0||clip.h<=0||clip.w>16384||clip.h>16384||uint64_t(clip.w)*clip.h>16000000||runs.size()>100000)
            throw std::runtime_error("positioned text layout exceeds limits");
        Surface result(SDL_CreateRGBSurfaceWithFormat(0,clip.w,clip.h,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
        if(!result)throw std::runtime_error(SDL_GetError());check(SDL_FillRect(result.get(),nullptr,0));
        struct Placement {int64_t x,y;std::shared_ptr<const Glyph> glyph;};
        for(const auto& run:runs){
            if(run.style.native_font){
                codepoints(run.text);size_t begin=0;int64_t y=run.y;for(;;){auto end=run.text.find('\n',begin);auto text=run.text.substr(begin,end==std::string::npos?end:end-begin);text.erase(std::remove(text.begin(),text.end(),'\r'),text.end());if(!text.empty()){auto cell=native_cell(text,run.style);auto bitmap=native_bitmap(*cell);over(result.get(),bitmap.get(),int64_t(run.x)-clip.x,y-clip.y);}if(end==std::string::npos)break;begin=end+1;y+=run.style.line_height?run.style.line_height:std::max(1,std::abs(run.style.native_font->height));}continue;
            }
            const auto points=codepoints(run.text);auto face=select_face(run.style);
            const int line_height=run.style.line_height?run.style.line_height:std::max(1,int((face->size->metrics.height+63)>>6));
            int64_t pen=run.x,baseline=int64_t(run.y)+int((face->size->metrics.ascender+63)>>6);
            int64_t minx=INT64_MAX,miny=INT64_MAX,maxx=INT64_MIN,maxy=INT64_MIN;FT_UInt previous{};
            std::vector<Placement> placements;
            for(auto cp:points){
                if(cp=='\r')continue;if(cp=='\n'){pen=run.x;baseline+=line_height;previous=0;continue;}if(cp=='\t')cp=' ';
                const auto index=FT_Get_Char_Index(face,cp);const auto cached=glyph(face,index);
                if(previous&&index&&FT_HAS_KERNING(face)){FT_Vector delta{};if(!FT_Get_Kerning(face,previous,index,FT_KERNING_DEFAULT,&delta))pen+=delta.x>>6;}
                if(cached->width&&cached->height){const int64_t x=pen+cached->left,y=baseline-cached->top;
                    placements.push_back({x,y,cached});minx=std::min(minx,x);miny=std::min(miny,y);
                    maxx=std::max(maxx,x+cached->width);maxy=std::max(maxy,y+cached->height);}
                pen+=std::max(1,cached->advance+run.style.letter_spacing);previous=index;
            }
            if(placements.empty())continue;
            const int lp=run.style.border_width+std::max(0,-run.style.shadow_offset.x),rp=run.style.border_width+std::max(0,run.style.shadow_offset.x);
            const int tp=run.style.border_width+std::max(0,-run.style.shadow_offset.y),bp=run.style.border_width+std::max(0,run.style.shadow_offset.y);
            minx=std::max(minx-lp,int64_t(clip.x)-lp);miny=std::max(miny-tp,int64_t(clip.y)-tp);
            maxx=std::min(maxx+rp,int64_t(clip.x)+clip.w+rp);maxy=std::min(maxy+bp,int64_t(clip.y)+clip.h+bp);
            if(maxx<=minx||maxy<=miny)continue;
            const int w=int(maxx-minx),h=int(maxy-miny);
            if(uint64_t(w)*h>64000000)throw std::runtime_error("positioned text effect bounds exceed limits");
            Bytes mask(size_t(w)*h);
            for(const auto& placement:placements){const auto& cached=*placement.glyph;
                const int64_t gx=placement.x-minx,gy=placement.y-miny;
                for(int sy=0;sy<cached.height;++sy){const int64_t y=gy+sy;if(y<0||y>=h)continue;
                    for(int sx=0;sx<cached.width;++sx){const int64_t x=gx+sx;if(x<0||x>=w)continue;
                        const unsigned a=cached.coverage[size_t(sy)*cached.width+sx];auto& target=mask[size_t(y)*w+size_t(x)];
                        target=uint8_t(a+(unsigned(target)*(255-a)+127)/255);}}
            }
            auto painted=decorate(mask,w,h,run.style);
            const int x0=int(std::max(int64_t(clip.x),minx)-minx),y0=int(std::max(int64_t(clip.y),miny)-miny);
            const int x1=int(std::min(int64_t(clip.x)+clip.w,maxx)-minx),y1=int(std::min(int64_t(clip.y)+clip.h,maxy)-miny);
            for(int y=y0;y<y1;++y){const auto* src=static_cast<const uint8_t*>(painted->pixels)+y*painted->pitch;
                auto* dst=static_cast<uint8_t*>(result->pixels)+size_t(miny+y-clip.y)*result->pitch;
                for(int x=x0;x<x1;++x){const auto* pixel=src+x*4;const unsigned a=pixel[3];if(!a)continue;
                    auto* target=dst+size_t(minx+x-clip.x)*4;const unsigned tail=(unsigned(target[3])*(255-a)+127)/255,out=a+tail;
                    for(int c=0;c<3;++c)target[c]=uint8_t((unsigned(pixel[c])*a+unsigned(target[c])*tail+out/2)/out);
                    target[3]=uint8_t(out);
                }
            }
        }
        return result;
    }
    Surface decorate(const Bytes& mask,int width,int height,const TextStyle& style){
        Surface surface(SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32), SDL_FreeSurface);
        if (!surface) throw std::runtime_error(SDL_GetError());
        check(SDL_FillRect(surface.get(), nullptr, 0));
        const auto paint = [&](const Bytes& alpha, SDL_Color color, SDL_Point offset) {
            if (!color.a) return;
            for (int py = 0; py < height; ++py) {
                const int source_y = py-offset.y;
                if (source_y < 0 || source_y >= height) continue;
                auto* row = static_cast<uint8_t*>(surface->pixels) + py*surface->pitch;
                for (int px = 0; px < width; ++px) {
                    const int source_x = px-offset.x;
                    if (source_x < 0 || source_x >= width) continue;
                    const unsigned a = (unsigned(alpha[size_t(source_y)*width+source_x])*color.a+127)/255;
                    if (!a) continue;
                    auto* dst = row+px*4;
                    const unsigned tail = (unsigned(dst[3])*(255-a)+127)/255;
                    const unsigned out_a = a+tail;
                    dst[0] = uint8_t((unsigned(color.r)*a+unsigned(dst[0])*tail+out_a/2)/out_a);
                    dst[1] = uint8_t((unsigned(color.g)*a+unsigned(dst[1])*tail+out_a/2)/out_a);
                    dst[2] = uint8_t((unsigned(color.b)*a+unsigned(dst[2])*tail+out_a/2)/out_a);
                    dst[3] = uint8_t(out_a);
                }
            }
        };
        if (style.shadow_offset.x || style.shadow_offset.y) paint(mask, style.shadow_color, style.shadow_offset);
        if (style.border_width) {
            Bytes horizontal(mask.size()), border(mask.size());
            const int radius = style.border_width;
            for (int py = 0; py < height; ++py) for (int px = 0; px < width; ++px) {
                uint8_t alpha{};
                for (int dx = -radius; dx <= radius; ++dx) if (px+dx >= 0 && px+dx < width)
                    alpha = std::max(alpha, mask[size_t(py)*width+px+dx]);
                horizontal[size_t(py)*width+px] = alpha;
            }
            for (int py = 0; py < height; ++py) for (int px = 0; px < width; ++px) {
                uint8_t alpha{};
                for (int dy = -radius; dy <= radius; ++dy) if (py+dy >= 0 && py+dy < height)
                    alpha = std::max(alpha, horizontal[size_t(py+dy)*width+px]);
                border[size_t(py)*width+px] = alpha;
            }
            paint(border, style.border_color, {0, 0});
        }
        paint(mask, style.color, {0, 0});
        return surface;
    }
};
}

namespace {
struct NativeTransitionPlan {
    int steps{},step_x{1},step_y{1},fade_bits{8},interval{};bool total_duration{};
};
NativeTransitionPlan transition_plan(int mode,const TransitionSampling& sampling,bool user,SDL_Rect area){
    const int quality=sampling.quality,timing=sampling.timing_mode,w=area.w,h=area.h;
    int delay=sampling.duration<0?((mode==1||(mode>=3&&mode<=6)||(mode>=11&&mode<=19))?50:mode==20?10:100):sampling.duration;
    NativeTransitionPlan plan;plan.interval=delay;plan.total_duration=timing>=2;
    if(user){plan.steps=256;plan.total_duration=true;return plan;}
    if(mode==1){plan.steps=16;return plan;}
    if(mode==2){
        plan.steps=256;plan.total_duration=timing!=0;
        if(timing!=0)return plan;
        plan.fade_bits=3;const int base=quality==0?264:quality==1?128:32,low=quality==0?66:quality==1?32:0;
        if(delay>base*16){plan.fade_bits=8;delay>>=5;}
        else if(delay>base*8){plan.fade_bits=7;delay>>=4;}
        else if(delay>base*4){plan.fade_bits=6;delay>>=3;}
        else if(delay>base*2){plan.fade_bits=5;delay>>=2;}
        else if(delay>base){plan.fade_bits=4;delay>>=1;}
        else if(low&&delay<low){plan.fade_bits=2;delay*=2;}
        plan.steps=1<<plan.fade_bits;plan.interval=delay;return plan;
    }
    if(mode>=3&&mode<=6){
        int step=8;
        if(timing<2&&quality==2){if((delay>>3)>=16){step=1;delay>>=3;}else if((delay>>2)>=16){step=2;delay>>=2;}else if((delay>>1)>=16){step=4;delay>>=1;}}
        plan.step_x=plan.step_y=step;plan.steps=((mode<=4?w:h)+step-1)/step;
    }else if(mode>=7&&mode<=10)plan.steps=16;
    else if(mode>=11&&mode<=14){plan.step_x=plan.step_y=16;plan.steps=((mode<=12?w:h)+256+15)/16;}
    else if(mode>=15&&mode<=18){
        int step=8;
        if(timing<2&&quality==0){while(delay<32&&step<128){delay*=2;step*=2;}}
        if(timing<2&&quality==2){if(delay>128){step=1;delay>>=3;}else if(delay>64){step=2;delay>>=2;}else if(delay>32){step=4;delay>>=1;}else if(delay<16){step=16;delay*=2;}}
        const int extent=w+area.y+h+((mode%2)?255:256);plan.step_x=plan.step_y=step;
        plan.steps=(mode%2)?(extent+step-1)/step:extent/step+1;
    }else if(mode==19||mode==20){const int tile=mode==19?64:32;plan.steps=((w+tile-1)/tile)*((h+tile-1)/tile);}
    else if(mode==21||mode==22){
        const int halfw=w/2,halfh=h/2;int stepx=32,stepy=32;
        if(timing>=2){stepx=int(halfw*10/(int64_t(delay)+1))+1;stepy=int(halfh*10/(int64_t(delay)+1))+1;}
        else{
            if(mode==21){const int nx=std::max(1,halfw/32),ny=std::max(1,halfh/32);
                if(ny<nx){while((halfw*100/stepx)/ny>174)stepx*=2;}else while((halfh*100/stepy)/nx>174)stepy*=2;}
            const int threshold=quality==0?66:quality==1?33:16;
            while(delay>threshold&&stepx>1&&stepy>1){delay>>=1;stepx/=2;stepy/=2;}
        }
        plan.step_x=stepx;plan.step_y=stepy;plan.steps=std::max(halfw/stepx,halfh/stepy);
    }else if(mode>=23&&mode<=28){
        int step=64;
        if(timing>=2)step=int(w*10/(int64_t(delay)+1))+1;
        else{const int threshold=quality==0?66:quality==1?33:16;while(delay>threshold&&step>1){delay>>=1;step/=2;}}
        plan.step_x=plan.step_y=step;plan.steps=((mode==25||mode==26||mode==28)?h:w)/step;
    }else {plan.steps=mode<=30?w:h;plan.total_duration=true;}
    plan.interval=delay;return plan;
}

// Stateless reconstruction of native CPU helpers. Progress chooses a loop
// iteration; all source reads are from immutable old/new snapshots.
Bytes software_transition(const Bytes& old,const Bytes& next,int width,int height,int mode,double p,
                          SDL_Rect area,const TransitionSampling& sample,const std::vector<int>& order) {
    Bytes result=old;
    const int left=area.x,top=area.y,right=left+area.w,bottom=top+area.h;
    const int w=area.w,h=area.h;
    if(w<=0||h<=0)return result;
    const auto blit=[&](int dx,int dy,const Bytes& source,int sx,int sy,int ex,int ey){
        // Original 40bb60 clips against the image bounds, not the supplied
        // transition RECT. Staggered stripes may therefore touch its margins.
        int countx=ex-sx,county=ey-sy;
        if(sx<0){dx-=sx;countx+=sx;sx=0;}if(sy<0){dy-=sy;county+=sy;sy=0;}
        if(dx<0){sx-=dx;countx+=dx;dx=0;}if(dy<0){sy-=dy;county+=dy;dy=0;}
        countx=std::min({countx,width-sx,width-dx});county=std::min({county,height-sy,height-dy});
        if(countx<=0||county<=0)return;
        for(int y=0;y<county;++y)std::memcpy(result.data()+(size_t(dy+y)*width+dx)*4,
            source.data()+(size_t(sy+y)*width+sx)*4,size_t(countx)*4);
    };
    const auto lerp=[&](int x,int y,int ratio,int denominator=256){
        ratio=std::clamp(ratio,0,denominator);const size_t i=(size_t(y)*width+x)*4;
        for(int c=0;c<3;++c){const int delta=(int(next[i+c])-old[i+c])*ratio;
            result[i+c]=uint8_t(int(old[i+c])+(delta>=0?delta/denominator:-((-delta+denominator-1)/denominator)));}
        result[i+3]=255;
    };
    const auto phase=[&](int count){return std::min(count-1,int(p*count));};
    const auto plan=transition_plan(mode,sample,false,area);
    if(mode==1){
        static const SDL_Point table[16]={{0,0},{2,2},{0,2},{2,0},{1,1},{3,3},{1,3},{3,1},
            {1,0},{1,2},{3,0},{3,2},{0,1},{2,3},{2,1},{0,3}};
        for(int k=0;k<=phase(16);++k)for(int y=top+table[k].y;y<bottom;y+=4)
            for(int x=left+table[k].x;x<right;x+=4)blit(x,y,next,x,y,x+1,y+1);
    }else if(mode==2){
        const int denominator=1<<plan.fade_bits,ratio=std::max(1,int(p*denominator));
        // Direct native blend walks base DIB rows with positive stride; its
        // RECT.top only limits the row count. Preserve that addressing quirk.
        for(int y=height-h;y<height;++y)for(int x=left;x<right;++x)lerp(x,y,ratio,denominator);
    }else if(mode>=3&&mode<=6){
        const int distance=(phase(plan.steps)+1)*plan.step_x;
        if(mode==3)blit(left,top,next,left,top,left+distance,bottom);
        if(mode==4)blit(right-distance,top,next,right-distance,top,right,bottom);
        if(mode==5)blit(left,top,next,left,top,right,top+distance);
        if(mode==6)blit(left,bottom-distance,next,left,bottom-distance,right,bottom);
    }else if(mode>=7&&mode<=10){
        for(int k=0;k<=phase(16);++k){
            if(mode<=8)for(int x=left+(mode==7?k:-k);x<right+(mode==8?16:0);x+=16)blit(x,top,next,x,top,x+1,bottom);
            else for(int y=top+(mode==9?k:-k);y<bottom+(mode==10?16:0);y+=16)blit(left,y,next,left,y,right,y+1);
        }
    }else if(mode>=11&&mode<=14){
        const bool vertical=mode>=13,reverse=(mode%2)==0;const int extent=vertical?h:w;
        const int count=(extent+256+15)/16;
        for(int i=0;i<=phase(count);++i){
            int position=(vertical?top:left)+(reverse?extent-1-i*16:i*16);
            for(int k=0;k<16;++k,position+=reverse?17:-17){
                if(vertical)blit(left,position,next,left,position,right,position+1);
                else blit(position,top,next,position,top,position+1,bottom);
            }
        }
    }else if(mode>=15&&mode<=18){
        // CPU/DIB branch explicitly addresses (height-y-1)*stride. Modes17/18
        // visit rows from bottom to top, reversing the diagonal slope.
        const int index=phase(plan.steps)*plan.step_x;
        for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){
            const int row=(mode>=17)?bottom-1-y:y-top;
            const int ratio=(mode%2)?left+index-row-x:x-(right-index+row)+1;
            lerp(x,y,ratio);
        }
    }else if(mode==19||mode==20){
        const int tile=mode==19?64:32;
        // Native row divisor uses absolute RECT.right, not its width.
        const int columns=(right+tile-1)/tile;
        for(int k=0;k<=phase(int(order.size()));++k){const int value=order[size_t(k)];
            const int x=left+(value%columns)*tile,y=top+(value/columns)*tile;blit(x,y,next,x,y,x+tile,y+tile);}
    }else if(mode==21||mode==22){
        const int halfw=w/2,halfh=h/2,stepx=plan.step_x,stepy=plan.step_y,count=plan.steps;
        if(count>0)for(int k=0;k<=phase(count);++k){const int dx=k*stepx,dy=k*stepy;
            if(mode==21){
                blit(left,top,old,left,top+dy,left+halfw,top+halfh);
                blit(left+halfw+dx,top,old,left+halfw,top,right-dx,top+halfh);
                blit(left,top+halfh,old,left+dx,top+halfh,left+halfw,bottom);
                blit(left+halfw,top+halfh+dy,old,left+halfw,top+halfh,right,bottom-dy);
                blit(left,top+halfh-dy-stepy,next,left,top+halfh-dy-stepy,left+halfw,top+halfh-dy);
                blit(left+halfw,top,next,left+halfw,top,left+halfw+dx+stepx,top+halfh);
                blit(left+halfw-dx,top+halfh,next,left+halfw-dx,top+halfh,left+halfw,bottom);
                blit(left+halfw,top+halfh,next,left+halfw,top+halfh,right,top+halfh+dy+stepy);
            }else{
                blit(left,top,old,left+dx,top,left+halfw,top+halfh);
                blit(left+halfw,top,old,left+halfw,top+dy,right,top+halfh);
                blit(left,top+halfh+dy,old,left,top+halfh,left+halfw,bottom-dy);
                blit(left+halfw+dx,top+halfh,old,left+halfw,top+halfh,right-dx,bottom);
                blit(left+halfw-dx-stepx,top,next,left+halfw-dx-stepx,top,left+halfw-dx,top+halfh);
                blit(left+halfw,top+halfh-dy-stepy,next,left+halfw,top+halfh-dy-stepy,right,top+halfh);
                blit(left,top+halfh,next,left,top+halfh,left+halfw,top+halfh+dy+stepy);
                blit(left+halfw,top+halfh,next,left+halfw,top+halfh,left+halfw+dx,bottom);
            }
        }
    }else if(mode>=23&&mode<=28){
        const int step=plan.step_x,count=plan.steps;
        if(count>0){const int distance=(phase(count)+1)*step;
            if(mode==23){blit(left,top,next,left,top,left+distance,top+h/2);blit(right-distance,top+h/2,next,right-distance,top+h/2,right,bottom);}
            if(mode==24){blit(right-distance,top,next,right-distance,top,right,top+h/2);blit(left,top+h/2,next,left,top+h/2,left+distance,bottom);}
            if(mode==25){blit(left,top,next,left,top,left+w/2,top+distance);blit(left+w/2,bottom-distance,next,left+w/2,bottom-distance,right,bottom);}
            if(mode==26){blit(left,bottom-distance,next,left,bottom-distance,left+w/2,bottom);blit(left+w/2,top,next,left+w/2,top,right,top+distance);}
            if(mode==27)blit(left+w/2-distance,top,next,left+w/2-distance,top,left+w/2+distance,bottom);
            if(mode==28)blit(left,top+h/2-distance,next,left,top+h/2-distance,right,top+h/2+distance);
        }
    }else{
        const int distance=std::max(1,int(p*(mode<=30?w:h)));
        blit(left,top,next,left,top,right,bottom);
        blit(left+(mode==29?-distance:mode==30?distance:0),top+(mode==31?-distance:mode==32?distance:0),old,left,top,right,bottom);
    }
    return result;
}

}
struct Scene::Impl {
    struct Image {
        std::shared_ptr<SDL_Texture> texture;
        std::shared_ptr<SDL_Surface> pixels, native_mask;
        std::shared_ptr<GpuPixels> gpu,gpu_mask; bool pixels_valid{true},texture_valid{true}; uint64_t revision{};
    };
    struct Layer {
        std::shared_ptr<Image> image, original;
        uint32_t add_color{}, sub_color{};
        bool sepia{}, monochrome{}, negaposi{};
        bool filter_dirty{};
        int filter_mode{};
        int image_type{2};
        double scale_x{1}, scale_y{1}, rotation{};
        bool raster_enabled{};
        double raster_amplitude{}, raster_phase{};
        int raster_frequency{};
        SDL_Point scale_center{}, rotate_center{};
        int alpha{255}; bool text{};
        int x{}, y{}, z{}, width{}, height{}, destination_width{}, destination_height{};
        bool visible{true}, resized{};
        SDL_Rect clip{};
        using DrawKey=std::tuple<int,uint64_t,int,int,double,double,double,bool,double,double,int,
            int,int,int,int,int,int,int,int,int,int,int,int,bool,bool,int,int,int,int>;
        DrawKey draw_key(int id) const {
            return std::make_tuple(id,image?image->revision:0,filter_mode,image_type,
                scale_x,scale_y,rotation,raster_enabled,raster_amplitude,raster_phase,raster_frequency,
                scale_center.x,scale_center.y,rotate_center.x,rotate_center.y,alpha,
                x,y,z,width,height,destination_width,destination_height,visible,resized,
                clip.x,clip.y,clip.w,clip.h);
        }
    };
    struct DrawState { std::weak_ptr<Image> image; Layer::DrawKey key; };
    struct TextLayout {std::vector<PositionedText> runs;SDL_Rect clip;std::weak_ptr<Image> image;uint64_t revision;};
    std::map<int,TextLayout> text_layouts;
    std::vector<DrawState> composed_state;
    bool composite_valid{};
    struct SDLVideo {
        SDLVideo() { check(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)); }
        ~SDLVideo() { SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS); }
    } video;
    int width, height;
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{nullptr, SDL_DestroyWindow};
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer{nullptr, SDL_DestroyRenderer};
    // The token expires before SDL destroys renderer-owned textures. Frames may
    // outlive Scene, but cannot be used after their owner has gone away.
    std::shared_ptr<int> lifetime = std::make_shared<int>(0);
    Texture composite{nullptr, SDL_DestroyTexture},transition_composite{nullptr,SDL_DestroyTexture};
    Texture prefix_cache{nullptr, SDL_DestroyTexture};int prefix_cutoff{};
    std::map<int, Layer> layers;
    std::map<std::pair<std::string,Bytes>, std::weak_ptr<Image>> image_cache;
    struct CachedImage {std::shared_ptr<Image> image;uint64_t stamp{},bytes{};};
    std::map<Image*,CachedImage> retained_images;uint64_t cache_stamp{};
    static constexpr uint64_t resource_cache_limit=128ULL*1024*1024;
    std::map<std::pair<int,Bytes>,Bytes> transition_masks;
    std::function<uint32_t()> transition_random;
    std::weak_ptr<SDL_Texture> random_frame;
    int random_mode{}; SDL_Rect random_area{};std::vector<int> random_order;
    RenderStats stats;
    RendererInfo info;
    std::unique_ptr<TextRasterizer> text_rasterizer;
    std::unique_ptr<GpuOps> gpu_ops;
    std::map<std::shared_ptr<const NativeFontBitmap>,std::shared_ptr<Image>,std::owner_less<std::shared_ptr<const NativeFontBitmap>>> gpu_glyphs;
    std::map<std::pair<int,Bytes>,std::shared_ptr<Image>> gpu_masks;
    Impl(int w, int h, const std::string& title) : width(w), height(h) {
        if (w <= 0 || h <= 0 || w > 16384 || h > 16384) throw std::runtime_error("invalid scene dimensions");
        window.reset(SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));
        if (!window) throw std::runtime_error(SDL_GetError());
        int driver = -1;
#ifdef __APPLE__
        if (!SDL_GetHint(SDL_HINT_RENDER_DRIVER)) {
            for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
                SDL_RendererInfo candidate{};
                if (SDL_GetRenderDriverInfo(i, &candidate) == 0 && std::string(candidate.name) == "metal") { driver = i; break; }
            }
        }
#endif
        renderer.reset(SDL_CreateRenderer(window.get(), driver, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC));
        if (!renderer) renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE));
        if (!renderer) throw std::runtime_error(SDL_GetError());
        SDL_RendererInfo actual{}; check(SDL_GetRendererInfo(renderer.get(), &actual));
        info = {actual.name, bool(actual.flags & SDL_RENDERER_ACCELERATED), bool(SDL_RenderTargetSupported(renderer.get()))};
        if (!info.target_textures) throw std::runtime_error("renderer does not support composition targets");
        check(SDL_RenderSetLogicalSize(renderer.get(), w, h));
        composite = target(w, h);transition_composite=target(w,h);
        gpu_ops=GpuOps::create(renderer.get());
    }
    Texture target(int w, int h) {
        Texture result(SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, w, h), SDL_DestroyTexture);
        if (!result) throw std::runtime_error(SDL_GetError());
        check(SDL_SetTextureBlendMode(result.get(), SDL_BLENDMODE_NONE));
        return result;
    }
    std::shared_ptr<SDL_Texture> upload(SDL_Surface* surface) {
        auto* texture = SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, surface->w, surface->h);
        if (!texture) throw std::runtime_error(SDL_GetError());
        std::weak_ptr<int> owner = lifetime;
        std::shared_ptr<SDL_Texture> result(texture, [owner](SDL_Texture* value) { if (!owner.expired()) SDL_DestroyTexture(value); });
        check(SDL_UpdateTexture(texture, nullptr, surface->pixels, surface->pitch));
        ++stats.uploads;
        return result;
    }
    std::shared_ptr<SDL_Surface> native_mask(const Bytes& input,const Bytes& external){
        Bytes bytes=external;
        if(bytes.empty()&&input.size()>=3&&std::memcmp(input.data(),"YKG",3)==0)bytes=parse_graphic(input).alpha;
        if(bytes.empty())return {};
        auto surface=decode_image(std::move(bytes));return {surface.release(),SDL_FreeSurface};
    }
    std::shared_ptr<Image> upload_image(Surface surface) {
        auto value = std::make_shared<Image>();
        value->texture = upload(surface.get());
        value->pixels = std::shared_ptr<SDL_Surface>(surface.release(), SDL_FreeSurface);
        return value;
    }
    std::shared_ptr<Image> resource_image(const Bytes& bytes,const Bytes& extra_mask){
        ++stats.image_decodes;
        if(!gpu_ops){auto image=upload_image(decode_image(bytes,extra_mask));image->native_mask=native_mask(bytes,extra_mask);return image;}
        Bytes color=bytes,internal;
        if(bytes.size()>=6&&std::memcmp(bytes.data(),"YKG000",6)==0){auto graphic=parse_graphic(bytes);color=std::move(graphic.color);internal=std::move(graphic.alpha);}
        auto image=upload_image(decode_image(std::move(color)));std::shared_ptr<SDL_Surface> raw_mask;std::shared_ptr<GpuPixels> gpu_mask;
        for(const auto* bytes:std::array<const Bytes*,2>{&internal,&extra_mask})if(!bytes->empty()){
            auto mask=decode_image(*bytes);if(mask->w!=image->pixels->w||mask->h!=image->pixels->h)throw std::runtime_error("color and alpha mask dimensions differ");
            raw_mask={mask.release(),SDL_FreeSurface};gpu_mask=gpu_ops->upload(raw_mask.get());auto pixels=gpu_ops->run(GpuOps::AlphaMask,gpu_pixels(image),gpu_mask,{},{},image->pixels->w,image->pixels->h);image=gpu_image(std::move(pixels));
        }
        image->native_mask=std::move(raw_mask);image->gpu_mask=std::move(gpu_mask);return image;
    }
    std::shared_ptr<Image> cached_image(const std::string& key,const Bytes& mask,const std::function<std::shared_ptr<Image>()>& decode){
        auto found=image_cache.find({key,mask});auto image=found==image_cache.end()?std::shared_ptr<Image>{}:found->second.lock();
        if(!image){image=decode();image_cache[{key,mask}]=image;}else{++stats.shared_loads;++stats.resource_cache_hits;}
        const uint64_t bytes=uint64_t(image->pixels->w)*image->pixels->h*(gpu_ops?12:8)+
            (image->native_mask?uint64_t(image->native_mask->w)*image->native_mask->h*4:0)+
            (image->gpu_mask?uint64_t(image->gpu_mask->width)*image->gpu_mask->height*4:0);
        if(bytes<=resource_cache_limit){auto [it,inserted]=retained_images.try_emplace(image.get(),CachedImage{image,0,bytes});if(inserted)stats.resource_cache_bytes+=bytes;it->second.stamp=++cache_stamp;
            while(stats.resource_cache_bytes>resource_cache_limit||retained_images.size()>64){auto oldest=std::min_element(retained_images.begin(),retained_images.end(),[](const auto& a,const auto& b){return a.second.stamp<b.second.stamp;});stats.resource_cache_bytes-=oldest->second.bytes;retained_images.erase(oldest);}}
        if(image_cache.size()>128){for(auto it=image_cache.begin();it!=image_cache.end();){if(it->second.expired())it=image_cache.erase(it);else ++it;}}
        return image;
    }
    void image(int id, std::shared_ptr<Image> image, int x, int y) {
        Layer value;
        value.image = std::move(image); value.original = value.image; value.x = x; value.y = y; value.z = id;
        value.width = value.image->pixels->w; value.height = value.image->pixels->h;
        value.destination_width = value.width; value.destination_height = value.height;
        value.clip = {0, 0, value.width, value.height};
        layers.insert_or_assign(id, std::move(value));
    }
    struct TargetScope {
        Impl& owner; SDL_Texture* previous;
        TargetScope(Impl& owner, SDL_Texture* target, int w, int h) : owner(owner), previous(SDL_GetRenderTarget(owner.renderer.get())) {
            check(SDL_SetRenderTarget(owner.renderer.get(), target));
            if (SDL_RenderSetLogicalSize(owner.renderer.get(), w, h) < 0) {
                SDL_SetRenderTarget(owner.renderer.get(), previous); throw std::runtime_error(SDL_GetError());
            }
        }
        ~TargetScope() { SDL_SetRenderTarget(owner.renderer.get(), previous); SDL_RenderSetLogicalSize(owner.renderer.get(), owner.width, owner.height); }
    };
    std::shared_ptr<GpuPixels> gpu_pixels(const std::shared_ptr<Image>& image) {
        if(!image->gpu)image->gpu=gpu_ops->upload(image->pixels.get());
        return image->gpu;
    }
    std::shared_ptr<GpuPixels> gpu_native_mask(const std::shared_ptr<Image>& image){
        if(image->native_mask&&!image->gpu_mask)image->gpu_mask=gpu_ops->upload(image->native_mask.get());
        return image->gpu_mask;
    }
    void host_pixels(const std::shared_ptr<Image>& image) {
        if(!image->pixels_valid){gpu_ops->read(image->gpu,image->pixels.get());image->pixels_valid=true;++stats.readbacks;stats.readback_pixels+=uint64_t(image->pixels->w)*image->pixels->h;}
    }
    std::shared_ptr<Image> gpu_image(std::shared_ptr<GpuPixels> pixels,std::shared_ptr<SDL_Texture> reuse={}) {
        auto image=std::make_shared<Image>();image->gpu=std::move(pixels);image->pixels_valid=false;image->texture_valid=false;
        image->pixels={SDL_CreateRGBSurfaceWithFormat(0,image->gpu->width,image->gpu->height,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface};
        if(!image->pixels)throw std::runtime_error(SDL_GetError());
        if(reuse)image->texture=std::move(reuse);
        else{auto output=target(image->gpu->width,image->gpu->height);std::weak_ptr<int> owner=lifetime;image->texture={output.release(),[owner](SDL_Texture* t){if(!owner.expired())SDL_DestroyTexture(t);}};}
        ++stats.gpu_operations;return image;
    }
    SDL_Texture* texture(const std::shared_ptr<Image>& image) {
        if(!image)throw std::runtime_error("missing render image");
        if(!image->texture_valid){
            if(!gpu_ops||!image->gpu)throw std::runtime_error("render image has no GPU pixels");
            TargetScope scope(*this,image->texture.get(),image->gpu->width,image->gpu->height);
            gpu_ops->write_target(image->texture.get(),image->gpu);image->texture_valid=true;
        }
        return image->texture.get();
    }
    Layer& layer(int id) {
        auto it = layers.find(id);
        if (it == layers.end()) throw std::runtime_error("unknown layer " + std::to_string(id));
        return it->second;
    }
    void filter(Layer& value) {
        if (!value.original) throw std::runtime_error("image color operation requires an image layer");
        value.filter_dirty=false;
        const auto enabled = [](uint32_t color) { return color != 0 && color != 0xffffffff; };
        if (!enabled(value.add_color) && !enabled(value.sub_color) && !value.sepia && !value.monochrome && !value.negaposi) {
            value.image = value.original; return;
        }
        if(gpu_ops){
            std::array<int,16> p{};p[0]=enabled(value.add_color);p[1]=int(value.add_color);p[2]=enabled(value.sub_color);p[3]=int(value.sub_color);p[4]=value.sepia;p[5]=value.monochrome;p[6]=value.negaposi;
            value.image=gpu_image(gpu_ops->run(GpuOps::Filter,gpu_pixels(value.original),{},{},p,value.width,value.height));value.image->native_mask=value.original->native_mask;value.image->gpu_mask=value.original->gpu_mask;return;
        }
        Surface result(SDL_ConvertSurfaceFormat(value.original->pixels.get(), SDL_PIXELFORMAT_RGBA32, 0), SDL_FreeSurface);
        if (!result) throw std::runtime_error(SDL_GetError());
        // Original 0x416a00 applies saturated channel addition followed by
        // saturated subtraction to a fresh resource image. Its bitmap is BGR;
        // this surface is RGBA, so COLORREF's low R byte maps to channel zero.
        for (int y = 0; y < result->h; ++y) {
            auto* row = static_cast<uint8_t*>(result->pixels) + y*result->pitch;
            for (int x = 0; x < result->w; ++x) {
                auto* pixel = row+x*4;
                for (unsigned c = 0; c < 3; ++c) {
                    int channel = pixel[c];
                    if (enabled(value.add_color)) channel = std::min(255, channel + int((value.add_color >> (c*8)) & 255));
                    if (enabled(value.sub_color)) channel = std::max(0, channel - int((value.sub_color >> (c*8)) & 255));
                    pixel[c] = uint8_t(channel);
                }
                color_effects(pixel, value.sepia, value.monochrome, value.negaposi);
            }
        }
        value.image = upload_image(std::move(result));
        value.image->native_mask=value.original->native_mask;
    }
    void filter_prefix(int mode) {
        // Original 0x413f24 dispatches 10001..10003 to the pixel helpers and
        // 10004 straight to 0x414e98. Each helper immediately returns when
        // both native GPU globals are set. This is observed native behavior,
        // not an unsupported-operation fallback.
        if (info.accelerated || mode == 4) return;
        Surface pixels(SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32), SDL_FreeSurface);
        if (!pixels) throw std::runtime_error(SDL_GetError());
        check(SDL_RenderReadPixels(renderer.get(), nullptr, SDL_PIXELFORMAT_RGBA32, pixels->pixels, pixels->pitch));
        ++stats.readbacks; stats.readback_pixels += uint64_t(width)*height;
        for (int y = 0; y < height; ++y) {
            auto* row = static_cast<uint8_t*>(pixels->pixels)+y*pixels->pitch;
            for (int x = 0; x < width; ++x) color_effects(row+x*4, mode==1, mode==2, mode==3);
        }
        auto image = upload_image(std::move(pixels));
        check(SDL_SetTextureBlendMode(image->texture.get(), SDL_BLENDMODE_NONE));
        check(SDL_RenderCopy(renderer.get(), image->texture.get(), nullptr, nullptr));
    }
    void draw_image(const Layer* layer, SDL_BlendMode blend, int alpha) {
            SDL_Rect dst{layer->x, layer->y, layer->destination_width, layer->destination_height};
            if (dst.w > 0 && dst.h > 0) {
                auto* image=texture(layer->image);
                check(SDL_SetTextureBlendMode(image, blend));
                check(SDL_SetTextureAlphaMod(image, uint8_t(alpha)));
                if (layer->scale_x == 1 && layer->scale_y == 1 && layer->rotation == 0) {
                    if (!layer->raster_enabled) check(SDL_RenderCopy(renderer.get(), image, &layer->clip, &dst));
                    else {
                      std::vector<SDL_Vertex> vertices;std::vector<int> indices;
                      vertices.reserve(size_t(std::min(layer->clip.h,8192))*4);indices.reserve(size_t(std::min(layer->clip.h,8192))*6);
                      const auto flush=[&]{if(vertices.empty())return;check(SDL_RenderGeometry(renderer.get(),image,vertices.data(),int(vertices.size()),indices.data(),int(indices.size())));vertices.clear();indices.clear();};
                      for(int row=0;row<layer->clip.h;++row) {
                        // 0x4140f0 uses truncation before adding row, then the
                        // original 3.141592 constant and full source height.
                        const uint32_t wrapped=uint32_t(int32_t(layer->raster_phase))+uint32_t(row);
                        int32_t phase;std::memcpy(&phase,&wrapped,sizeof phase);
                        const double offset=std::sin(double(phase)/layer->height*layer->raster_frequency*3.141592)*layer->raster_amplitude;
                        const int shift=offset>=INT32_MIN&&offset<=INT32_MAX?int(offset):INT32_MIN;
                        const int top=int(int64_t(row)*dst.h/layer->clip.h),bottom=int(int64_t(row+1)*dst.h/layer->clip.h);
                        if(bottom<=top)continue;
                        const int base=int(vertices.size()),left=int(int64_t(dst.x)+shift),right=left+dst.w;
                        const float u0=float(layer->clip.x)/layer->width,u1=float(layer->clip.x+layer->clip.w)/layer->width;
                        const float v0=float(layer->clip.y+row)/layer->height,v1=float(layer->clip.y+row+1)/layer->height;
                        const SDL_Color color{255,255,255,uint8_t(alpha)};
                        vertices.push_back({{float(left),float(dst.y+top)},color,{u0,v0}});vertices.push_back({{float(right),float(dst.y+top)},color,{u1,v0}});
                        vertices.push_back({{float(right),float(dst.y+bottom)},color,{u1,v1}});vertices.push_back({{float(left),float(dst.y+bottom)},color,{u0,v1}});
                        for(int n:{0,1,2,0,2,3})indices.push_back(base+n);
                        if(vertices.size()>=32768)flush();
                      }
                      flush();
                    }
                }
                else {
                    // Native GPU path 0x413440: local scale around one center,
                    // followed by Z rotation around an independent center.
                    // 0x41368f divides native angle by 1000 then multiplies pi.
                    const float radians = float(float(layer->rotation)/1000.0*3.1415927410125732);
                    const double cosine = std::cos(radians), sine = std::sin(radians);
                    const double x[4] = {0, double(dst.w), double(dst.w), 0};
                    const double y[4] = {0, 0, double(dst.h), double(dst.h)};
                    const float u0 = float(layer->clip.x)/layer->width, v0 = float(layer->clip.y)/layer->height;
                    const float u1 = float(layer->clip.x+layer->clip.w)/layer->width;
                    const float v1 = float(layer->clip.y+layer->clip.h)/layer->height;
                    const SDL_FPoint uv[4] = {{u0,v0},{u1,v0},{u1,v1},{u0,v1}};
                    SDL_Vertex vertices[4];
                    for (int i = 0; i < 4; ++i) {
                        const double sx = (x[i]-layer->scale_center.x)*layer->scale_x + layer->scale_center.x;
                        const double sy = (y[i]-layer->scale_center.y)*layer->scale_y + layer->scale_center.y;
                        const double rx = sx-layer->rotate_center.x, ry = sy-layer->rotate_center.y;
                        vertices[i] = {{float(dst.x+layer->rotate_center.x+rx*cosine-ry*sine),
                                        float(dst.y+layer->rotate_center.y+rx*sine+ry*cosine)},
                                       {255,255,255,uint8_t(alpha)}, uv[i]};
                    }
                    static const int indices[6] = {0,1,2,0,2,3};
                    check(SDL_RenderGeometry(renderer.get(), image, vertices, 4, indices, 6));
                }
            }
    }
    void software_blend(const Layer* layer) {
        // 0x40bb60 jump table: type4 -> 0x40c293; type5 -> 0x40c418.
        // White transparency skips, black transparency copies source. Only
        // middle mask values select saturated ADD or max(D+S-255,0) SUB.
        // These branches do not read the global transmittance argument.
        Surface source(SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
        Surface destination(SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
        if (!source || !destination) throw std::runtime_error(SDL_GetError());
        auto scratch = target(width,height);
        {
            TargetScope target_scope(*this,scratch.get(),width,height);
            check(SDL_SetRenderDrawColor(renderer.get(),0,0,0,0)); check(SDL_RenderClear(renderer.get()));
            draw_image(layer,SDL_BLENDMODE_NONE,255);
            check(SDL_RenderReadPixels(renderer.get(),nullptr,SDL_PIXELFORMAT_RGBA32,source->pixels,source->pitch));
        }
        check(SDL_RenderReadPixels(renderer.get(),nullptr,SDL_PIXELFORMAT_RGBA32,destination->pixels,destination->pitch));
        stats.readbacks += 2; stats.readback_pixels += uint64_t(width)*height*2;
        for (int y=0;y<height;++y) {
            const auto* s=static_cast<const uint8_t*>(source->pixels)+y*source->pitch;
            auto* d=static_cast<uint8_t*>(destination->pixels)+y*destination->pitch;
            for(int x=0;x<width;++x,s+=4,d+=4) {
                if (!s[3]) continue;
                for(unsigned c=0;c<3;++c) d[c]=s[3]==255?s[c]:uint8_t(layer->image_type==4?
                    std::min(255,int(d[c])+s[c]):std::max(0,int(d[c])+s[c]-255));
            }
        }
        auto image=upload_image(std::move(destination));
        check(SDL_SetTextureBlendMode(image->texture.get(),SDL_BLENDMODE_NONE));
        check(SDL_RenderCopy(renderer.get(),image->texture.get(),nullptr,nullptr));
    }
    void compose(std::optional<int> through={},bool use_cache=true) {
        for(auto& [id,layer]:layers)if(layer.filter_dirty)filter(layer);
        // Compare exact drawing state rather than a hash. Weak ownership keeps
        // the text texture reuse and resource LRU lifetimes unchanged.
        std::vector<DrawState> state;
        if(!through && use_cache){
            state.reserve(layers.size());
            for(const auto& [id,layer]:layers)if(layer.visible && !(prefix_cache&&id<=prefix_cutoff))
                state.push_back({layer.image,layer.draw_key(id)});
            if(composite_valid && state.size()==composed_state.size() &&
               std::equal(state.begin(),state.end(),composed_state.begin(),[](const auto& a,const auto& b){
                   return a.key==b.key&&!a.image.owner_before(b.image)&&!b.image.owner_before(a.image);
               })){++stats.composition_reuses;return;}
        }
        std::vector<const Layer*> sorted;
        for (const auto& item : layers) if (item.second.visible&&(!through||item.first<=*through)&&
            !(use_cache&&prefix_cache&&item.first<=prefix_cutoff)) sorted.push_back(&item.second);
        std::stable_sort(sorted.begin(), sorted.end(), [](const Layer* a, const Layer* b) { return a->z < b->z; });
        // Export all pending Metal images before selecting the composition
        // target. The first export submits the shared compute command buffer;
        // subsequent exports reuse that completed batch and avoid interrupting
        // SDL's layer draw encoder with target switches.
        for(const auto* layer:sorted)if(layer->image&&layer->alpha&&!layer->filter_mode)texture(layer->image);
        composite_valid=false;
        TargetScope target(*this, composite.get(), width, height);
        ++stats.compositions;
        check(SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255));
        check(SDL_RenderClear(renderer.get()));
        if(use_cache&&prefix_cache){check(SDL_SetTextureBlendMode(prefix_cache.get(),SDL_BLENDMODE_NONE));check(SDL_RenderCopy(renderer.get(),prefix_cache.get(),nullptr,nullptr));}
        for (const auto* layer : sorted) {
            if (layer->filter_mode) filter_prefix(layer->filter_mode);
            else if (!layer->alpha) continue;
            else if (!info.accelerated && layer->image_type>=4) software_blend(layer);
            else draw_image(layer,SDL_BLENDMODE_BLEND,layer->alpha);
        }
        if(!through && use_cache){composed_state=std::move(state);composite_valid=true;}
    }
    void gpu_positioned(int id,const std::vector<PositionedText>& runs,SDL_Rect clip,size_t first=0){
        if(clip.w<=0||clip.h<=0||clip.w>16384||clip.h>16384||uint64_t(clip.w)*clip.h>16000000||runs.size()>100000)throw std::runtime_error("positioned text layout exceeds limits");
        // Copy the old canvas before appending. SDL's separate queue may still
        // sample the previous GPU image; mutating it in place would race export.
        auto canvas=first?gpu_ops->run(GpuOps::Filter,layer(id).image->gpu,{},{},{},clip.w,clip.h):
            gpu_ops->run(GpuOps::Clear,{},{},{},{},clip.w,clip.h);
        if(first)++stats.text_appends;
        for(size_t at=first;at<runs.size();++at){const auto& run=runs[at];
            if(run.style.native_font){
                codepoints(run.text);size_t begin=0;int64_t y=run.y;
                for(;;){auto end=run.text.find('\n',begin);auto text=run.text.substr(begin,end==std::string::npos?end:end-begin);text.erase(std::remove(text.begin(),text.end(),'\r'),text.end());
                    if(!text.empty()){
                        auto cell=text_rasterizer->native_cell(text,run.style);auto found=gpu_glyphs.find(cell);
                        if(found==gpu_glyphs.end()){if(gpu_glyphs.size()>=2048)gpu_glyphs.clear();auto bitmap=text_rasterizer->native_bitmap(*cell);auto source=std::make_shared<Image>();source->gpu=gpu_ops->upload(bitmap.get());found=gpu_glyphs.emplace(cell,source).first;}
                        std::array<int,16> p{};auto source=found->second->gpu;const int64_t gx=int64_t(run.x)-clip.x,gy=y-clip.y;
                        if(gx<clip.w&&gy<clip.h&&gx+source->width>0&&gy+source->height>0){p[0]=int(gx);p[1]=int(gy);p[2]=source->width;p[3]=source->height;canvas=gpu_ops->run(GpuOps::TextOver,canvas,source,{},p,clip.w,clip.h);++stats.gpu_operations;}
                    }
                    if(end==std::string::npos)break;begin=end+1;y+=run.style.line_height?run.style.line_height:std::max(1,std::abs(run.style.native_font->height));
                }
            }else{
                auto prepared=text_rasterizer->positioned({run},clip);auto source=gpu_ops->upload(prepared.get());std::array<int,16> p{};p[2]=clip.w;p[3]=clip.h;canvas=gpu_ops->run(GpuOps::TextOver,canvas,source,{},p,clip.w,clip.h);++stats.gpu_operations;
            }
        }
        auto found=layers.find(id);bool update=found!=layers.end()&&found->second.text&&found->second.image==found->second.original&&found->second.image.use_count()==2&&found->second.width==clip.w&&found->second.height==clip.h;
        std::shared_ptr<SDL_Texture> reuse;if(update){int access{};check(SDL_QueryTexture(found->second.image->texture.get(),nullptr,&access,nullptr,nullptr));if(access==SDL_TEXTUREACCESS_TARGET)reuse=found->second.image->texture;}
        auto result=gpu_image(std::move(canvas),std::move(reuse));if(update){found->second.image=result;found->second.original=std::move(result);found->second.x=clip.x;found->second.y=clip.y;++stats.texture_updates;}else{image(id,std::move(result),clip.x,clip.y);layer(id).text=true;++stats.uploads;}
    }
    void render() {
        compose();
        check(SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255)); check(SDL_RenderClear(renderer.get()));
        check(SDL_RenderCopy(renderer.get(), composite.get(), nullptr, nullptr));
    }

};
Scene::Scene(int width, int height, std::string title) : impl_(std::make_unique<Impl>(width, height, title)) {}
Scene::~Scene() = default;
void Scene::set_title(const std::string& title) { SDL_SetWindowTitle(impl_->window.get(), title.c_str()); }
void Scene::configure_native_fonts(NativeFontOptions options){impl_->text_layouts.clear();if(!impl_->text_rasterizer)impl_->text_rasterizer=std::make_unique<TextRasterizer>();impl_->text_rasterizer->default_font=options.fallback_font;impl_->text_rasterizer->native_fonts.configure(std::move(options));}
void Scene::load(int id, Bytes bytes, int x, int y, const Bytes& extra_mask) {
    auto image=impl_->resource_image(bytes,extra_mask);
    impl_->image(id,std::move(image),x,y);
}
void Scene::load_cached(int id, const std::string& resource_key, const Bytes& bytes, int x, int y, const Bytes& extra_mask) {
    auto image=impl_->cached_image(resource_key,extra_mask,[&]{return impl_->resource_image(bytes,extra_mask);});impl_->image(id,std::move(image),x,y);
}
void Scene::load_deferred(int id,const std::string& key,std::function<Bytes()> loader,int x,int y,const Bytes& mask){
    if(!loader)throw std::runtime_error("resource loader is missing");auto image=impl_->cached_image(key,mask,[&]{auto bytes=loader();return impl_->resource_image(bytes,mask);});impl_->image(id,std::move(image),x,y);
}
void Scene::copy(int id, int source, int x, int y) {
    auto& source_layer=impl_->layer(source);if(source_layer.filter_dirty)impl_->filter(source_layer);
    const int type = source_layer.image_type;
    auto image = source_layer.image;
    if (!image) throw std::runtime_error("cannot copy a GraphicFilter layer as an image");
    impl_->image(id, std::move(image), x, y); ++impl_->stats.shared_loads;
    impl_->layer(id).image_type = type;
}
void Scene::create_canvas(int id,int width,int height,int x,int y){
    if(width<=0||height<=0||width>16384||height>16384||uint64_t(width)*height>16000000)
        throw std::runtime_error("invalid canvas dimensions");
    if(impl_->gpu_ops){impl_->image(id,impl_->gpu_image(impl_->gpu_ops->run(GpuOps::Clear,{},{},{},{},width,height)),x,y);return;}
    Surface pixels(SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
    if(!pixels)throw std::runtime_error(SDL_GetError());
    check(SDL_FillRect(pixels.get(),nullptr,SDL_MapRGBA(pixels->format,0,0,0,0)));
    impl_->image(id,impl_->upload_image(std::move(pixels)),x,y);
}
void Scene::paint(int destination,int source,SDL_Rect rect){
    auto& dest=impl_->layer(destination);auto& src=impl_->layer(source);
    if(src.filter_dirty)impl_->filter(src);
    if(!dest.original||!src.image)throw std::runtime_error("canvas paint requires bitmap layers");
    if(rect.w<=0||rect.h<=0||rect.w>16384||rect.h>16384||uint64_t(rect.w)*rect.h>16000000)
        throw std::runtime_error("invalid canvas paint dimensions");
    if(impl_->gpu_ops){std::array<int,16> p{};p[0]=rect.x;p[1]=rect.y;p[2]=rect.w;p[3]=rect.h;
        dest.original=impl_->gpu_image(impl_->gpu_ops->run(GpuOps::Paint,impl_->gpu_pixels(dest.original),impl_->gpu_pixels(src.image),{},p,dest.width,dest.height));dest.filter_dirty=true;return;}
    Surface result(SDL_ConvertSurfaceFormat(dest.original->pixels.get(),SDL_PIXELFORMAT_RGBA32,0),SDL_FreeSurface);
    Surface scaled(SDL_CreateRGBSurfaceWithFormat(0,rect.w,rect.h,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
    Surface input(SDL_ConvertSurfaceFormat(src.image->pixels.get(),SDL_PIXELFORMAT_RGBA32,0),SDL_FreeSurface);
    if(!result||!scaled||!input)throw std::runtime_error(SDL_GetError());
    check(SDL_SetSurfaceBlendMode(input.get(),SDL_BLENDMODE_NONE));
    check(SDL_BlitScaled(input.get(),nullptr,scaled.get(),nullptr));
    const int left=std::max(0,rect.x),top=std::max(0,rect.y);
    const int right=int(std::min<int64_t>(dest.width,int64_t(rect.x)+rect.w));
    const int bottom=int(std::min<int64_t>(dest.height,int64_t(rect.y)+rect.h));
    for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){
        auto* d=static_cast<uint8_t*>(result->pixels)+y*result->pitch+x*4;
        const auto* s=static_cast<const uint8_t*>(scaled->pixels)+(y-rect.y)*scaled->pitch+(x-rect.x)*4;
        // Straight-alpha Porter-Duff over. Keep the alpha plane: SDL's usual
        // surface blending otherwise leaves an unusable transparent canvas.
        const unsigned a=unsigned(s[3])*255+unsigned(d[3])*(255-s[3]);
        for(int c=0;c<3;++c)d[c]=a?uint8_t((unsigned(s[c])*s[3]*255+unsigned(d[c])*d[3]*(255-s[3])+a/2)/a):0;
        d[3]=uint8_t((a+127)/255);
    }
    dest.original=impl_->upload_image(std::move(result));
    dest.filter_dirty=true;
}
void Scene::set_glyph_color(int id,uint32_t colorref,const SDL_Rect* region){
    auto& layer=impl_->layer(id);
    if(!layer.original)throw std::runtime_error("glyph tint requires bitmap layer");
    if(colorref>0xffffff)throw std::runtime_error("glyph tint requires resolved COLORREF");
    if(layer.image_type==3)return; // Original 40cf90 switch skips type3.
    const SDL_Rect area=region?*region:SDL_Rect{0,0,layer.width,layer.height};
    if(area.w<=0||area.h<=0)return;
    const int left=std::max(0,area.x),top=std::max(0,area.y);
    const int right=int(std::min<int64_t>(layer.width,int64_t(area.x)+area.w));
    const int bottom=int(std::min<int64_t>(layer.height,int64_t(area.y)+area.h));
    if(left>=right||top>=bottom)return;
    if(impl_->gpu_ops){std::array<int,16> p{};p[0]=left;p[1]=top;p[2]=right-left;p[3]=bottom-top;p[4]=int(colorref);
        auto changed=impl_->gpu_image(impl_->gpu_ops->run(GpuOps::Tint,impl_->gpu_pixels(layer.original),{},{},p,layer.width,layer.height));changed->native_mask=layer.original->native_mask;changed->gpu_mask=layer.original->gpu_mask;layer.original=std::move(changed);layer.filter_dirty=true;return;}
    Surface pixels(SDL_ConvertSurfaceFormat(layer.original->pixels.get(),SDL_PIXELFORMAT_RGBA32,0),SDL_FreeSurface);
    if(!pixels)throw std::runtime_error(SDL_GetError());
    for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){
        auto* p=static_cast<uint8_t*>(pixels->pixels)+y*pixels->pitch+x*4;
        for(int c=0;c<3;++c)p[c]=uint8_t(colorref>>(8*c));
    }
    auto changed=impl_->upload_image(std::move(pixels));changed->native_mask=layer.original->native_mask;
    layer.original=std::move(changed);layer.filter_dirty=true;
}
Bytes Scene::layer_pixels(int id) const {
    auto& layer=impl_->layer(id);if(layer.filter_dirty)impl_->filter(layer);
    if(!layer.image)throw std::runtime_error("GraphicFilter has no bitmap snapshot");
    Bytes out={'Y','U','K','A','P','I','X','L'};
    auto put=[&](uint32_t value){for(unsigned i=0;i<4;++i)out.push_back(uint8_t(value>>(8*i)));};
    auto blob=[&](const SDL_Surface* surface){auto bytes=encode_surface(surface);put(uint32_t(bytes.size()));out.insert(out.end(),bytes.begin(),bytes.end());};
    const bool separate=layer.original&&layer.original!=layer.image;
    put(1);put(unsigned(layer.image_type));put(separate);
    impl_->host_pixels(layer.image);blob(layer.image->pixels.get());blob(layer.image->native_mask.get());
    if(separate){impl_->host_pixels(layer.original);blob(layer.original->pixels.get());blob(layer.original->native_mask.get());}
    return out;
}
Bytes Scene::image_thumbnail(const Bytes& image,int width,int height){
    if(width<=0||height<=0||width>16384||height>16384||uint64_t(width)*height>16000000)throw std::runtime_error("invalid image thumbnail dimensions");
    auto original=decode_image(image);
    Surface scaled(SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
    if(!scaled)throw std::runtime_error(SDL_GetError());
    check(SDL_SetSurfaceBlendMode(original.get(),SDL_BLENDMODE_NONE));
    check(SDL_BlitScaled(original.get(),nullptr,scaled.get(),nullptr));
    return encode_surface(scaled.get());
}
Bytes Scene::resource_thumbnail(const Bytes& bytes,int width,int height){
    if(width<=0||height<=0||width>16384||height>16384||uint64_t(width)*height>16000000)throw std::runtime_error("invalid image thumbnail dimensions");
    if(!impl_->gpu_ops)return image_thumbnail(bytes,width,height);
    auto image=impl_->resource_image(bytes,{});auto output=impl_->target(width,height);
    Surface pixels(SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);if(!pixels)throw std::runtime_error(SDL_GetError());
    auto* texture=impl_->texture(image);
    {Impl::TargetScope scope(*impl_,output.get(),width,height);check(SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_NONE));check(SDL_RenderCopy(renderer(),texture,nullptr,nullptr));check(SDL_RenderReadPixels(renderer(),nullptr,SDL_PIXELFORMAT_RGBA32,pixels->pixels,pixels->pitch));}
    ++impl_->stats.gpu_operations;++impl_->stats.readbacks;impl_->stats.readback_pixels+=uint64_t(width)*height;return encode_surface(pixels.get());
}
void Scene::restore_layer_pixels(int id,const Bytes& bytes) {
    auto& layer=impl_->layer(id);
    if(bytes.size()<20||std::memcmp(bytes.data(),"YUKAPIXL",8))throw std::runtime_error("invalid bitmap snapshot");
    size_t at=8;
    auto get=[&](){if(bytes.size()-at<4)throw std::runtime_error("truncated bitmap snapshot");const auto value=u32(bytes,at);at+=4;return value;};
    auto blob=[&](){const auto count=get();if(count>bytes.size()-at)throw std::runtime_error("truncated bitmap snapshot payload");Bytes value(bytes.begin()+at,bytes.begin()+at+count);at+=count;return value;};
    if(get()!=1)throw std::runtime_error("unsupported bitmap snapshot version");
    const auto type=get(),separate=get();if(type<1||type>5||separate>1)throw std::runtime_error("invalid bitmap snapshot flags");
    auto image=[&](){
        auto color=blob(),mask=blob();
        if(color.size()<8||png_sig_cmp(color.data(),0,8))throw std::runtime_error("bitmap snapshot color is not PNG");
        auto pixels=decode_image(std::move(color));
        if(pixels->w!=layer.width||pixels->h!=layer.height)throw std::runtime_error("bitmap snapshot dimensions differ from loaded resource");
        Surface alpha(nullptr,SDL_FreeSurface);
        if(!mask.empty()){
            if(mask.size()<8||png_sig_cmp(mask.data(),0,8))throw std::runtime_error("bitmap snapshot mask is not PNG");
            alpha=decode_image(std::move(mask));if(alpha->w!=pixels->w||alpha->h!=pixels->h)throw std::runtime_error("bitmap snapshot mask dimensions differ");
        }
        auto result=impl_->upload_image(std::move(pixels));result->native_mask=std::move(alpha);return result;
    };
    auto current=image(),original=separate?image():current;
    if(at!=bytes.size())throw std::runtime_error("trailing bitmap snapshot bytes");
    layer.image=std::move(current);layer.original=std::move(original);layer.image_type=int(type);layer.filter_dirty=false;
}
void Scene::blit(int destination,int x,int y,int source,SDL_Rect rect,NativePixelKernel kernel){
    auto dest_it=impl_->layers.find(destination),source_it=impl_->layers.find(source);
    if(dest_it==impl_->layers.end()||source_it==impl_->layers.end())return;
    auto& dest=dest_it->second;auto& src=source_it->second;
    if(dest.filter_dirty)impl_->filter(dest);if(&src!=&dest&&src.filter_dirty)impl_->filter(src);
    if(!dest.image||!src.image)throw std::runtime_error("native GraphicCopy requires bitmap layers");
    int64_t dx=x,dy=y,sx=rect.x,sy=rect.y,ex=sx+rect.w,ey=sy+rect.h;
    if(dx<0){sx-=dx;dx=0;}if(dy<0){sy-=dy;dy=0;}
    if(sx<0){dx-=sx;sx=0;}if(sy<0){dy-=sy;sy=0;}
    ex=std::min<int64_t>(ex,src.width);ey=std::min<int64_t>(ey,src.height);
    ex=std::min(ex,int64_t(dest.width)-dx+sx);ey=std::min(ey,int64_t(dest.height)-dy+sy);
    if(ex<=sx||ey<=sy)return;
    if(impl_->gpu_ops){
        std::array<int,16> p{};p[0]=int(dx);p[1]=int(dy);p[2]=int(sx);p[3]=int(sy);p[4]=int(ex-sx);p[5]=int(ey-sy);p[6]=src.image_type;p[7]=kernel==NativePixelKernel::Mmx;p[8]=bool(src.image->native_mask);
        auto mask=impl_->gpu_native_mask(src.image);
        auto previous=dest.image;auto changed=impl_->gpu_image(impl_->gpu_ops->run(destination==source?GpuOps::SelfBlit:GpuOps::Blit,impl_->gpu_pixels(previous),impl_->gpu_pixels(src.image),mask,p,dest.width,dest.height));changed->native_mask=previous->native_mask;changed->gpu_mask=previous->gpu_mask;dest.image=changed;if(dest.original==previous)dest.original=changed;return;
    }
    impl_->host_pixels(dest.image);impl_->host_pixels(src.image);
    Surface result(SDL_ConvertSurfaceFormat(dest.image->pixels.get(),SDL_PIXELFORMAT_RGBA32,0),SDL_FreeSurface);
    if(!result)throw std::runtime_error(SDL_GetError());
    auto* source_pixels=destination==source?result.get():src.image->pixels.get();
    const auto* source_mask=src.image->native_mask.get();
    const auto* key=static_cast<const uint8_t*>(source_pixels->pixels)+(source_pixels->h-1)*source_pixels->pitch;
    const uint8_t key_color[3]={key[0],key[1],key[2]};
    const auto signed16=[](uint16_t value){int16_t out;std::memcpy(&out,&value,2);return int(out);};
    const auto shift8=[](int value){return value>=0?value/256:-((-value+255)/256);};
    for(int64_t row=ey-sy-1;row>=0;--row)for(int64_t col=0;col<ex-sx;++col){
        const auto* pixel=static_cast<const uint8_t*>(source_pixels->pixels)+(sy+row)*source_pixels->pitch+(sx+col)*4;
        auto* target=static_cast<uint8_t*>(result->pixels)+(dy+row)*result->pitch+(dx+col)*4;
        uint8_t mask[3];
        if(source_mask){const auto* value=static_cast<const uint8_t*>(source_mask->pixels)+(sy+row)*source_mask->pitch+(sx+col)*4;
            std::copy(value,value+3,mask);}
        else std::fill(mask,mask+3,uint8_t(255-pixel[3]));
        const bool transparent=mask[0]==255&&mask[1]==255&&mask[2]==255,opaque=mask[0]==0&&mask[1]==0&&mask[2]==0;
        if(src.image_type==3){if(pixel[0]!=key_color[0]||pixel[1]!=key_color[1]||pixel[2]!=key_color[2])std::copy(pixel,pixel+3,target);continue;}
        if(src.image_type==1){std::copy(pixel,pixel+3,target);continue;}
        if(src.image_type==2){
            if(kernel==NativePixelKernel::Scalar){
                if(transparent)continue;if(opaque){std::copy(pixel,pixel+3,target);continue;}
                for(int c=0;c<3;++c)target[c]=uint8_t(int(pixel[c])+shift8((int(target[c])-pixel[c])*mask[2-c]));
            }else{
                // Native MMX: PMULLW(delta,mask), PADDSW(product,delta),
                // PSRLW8, PADDSW(source,...) then low-byte masks.
                for(int c=0;c<3;++c){const int delta=int(target[c])-pixel[c];
                    const int product=signed16(uint16_t(delta*mask[c]));const int sum=std::clamp(product+delta,-32768,32767);
                    target[c]=uint8_t(int(pixel[c])+(uint16_t(sum)>>8));}
            }
        }else if(src.image_type==4||src.image_type==5){
            if(transparent)continue;if(opaque){std::copy(pixel,pixel+3,target);continue;}
            for(int c=0;c<3;++c)target[c]=uint8_t(src.image_type==4?std::min(255,int(target[c])+pixel[c]):std::max(0,int(target[c])+pixel[c]-255));
        }
    }
    auto previous=dest.image;auto changed=impl_->upload_image(std::move(result));changed->native_mask=previous->native_mask;
    dest.image=changed;if(dest.original==previous)dest.original=changed;
}
void Scene::swap_layers(int first,int second){
    if(first==second)return;auto a=impl_->layers.find(first),b=impl_->layers.find(second);
    if(a==impl_->layers.end()&&b==impl_->layers.end())return;
    if(a!=impl_->layers.end()&&b!=impl_->layers.end()){
        const int az=a->second.z,bz=b->second.z;std::swap(a->second,b->second);a->second.z=az;b->second.z=bz;
    }else if(a==impl_->layers.end()){
        auto value=std::move(b->second);impl_->layers.erase(b);value.z=first;impl_->layers.emplace(first,std::move(value));
    }else{
        auto value=std::move(a->second);impl_->layers.erase(a);value.z=second;impl_->layers.emplace(second,std::move(value));
    }
}
void Scene::cache_prefix(int through_id){
    // 415170 immediately returns on the original accelerated backend.
    if(impl_->info.accelerated)return;
    clear_prefix_cache();impl_->compose(through_id,false);auto target=impl_->target(width(),height());
    {Impl::TargetScope scope(*impl_,target.get(),width(),height());check(SDL_RenderCopy(renderer(),impl_->composite.get(),nullptr,nullptr));}
    impl_->prefix_cache=std::move(target);impl_->prefix_cutoff=through_id;
}
void Scene::clear_prefix_cache(){impl_->composite_valid=false;impl_->prefix_cache.reset();impl_->prefix_cutoff=0;}
bool Scene::prefix_cached()const{return bool(impl_->prefix_cache);}
void Scene::text(int id, const std::string& utf8, int x, int y, int width, int height, const TextStyle& style) {
    if (!impl_->text_rasterizer) impl_->text_rasterizer = std::make_unique<TextRasterizer>();
    if(impl_->gpu_ops&&style.native_font){auto runs=impl_->text_rasterizer->native_runs(utf8,width,height,style);impl_->gpu_positioned(id,runs,{0,0,width,height});impl_->layer(id).x=x;impl_->layer(id).y=y;return;}
    auto surface = impl_->text_rasterizer->rasterize(utf8, width, height, style);
    auto found = impl_->layers.find(id);
    if (found != impl_->layers.end() && found->second.text && found->second.image == found->second.original &&
        found->second.image.use_count() == 2 &&
        found->second.width == width && found->second.height == height) {
        check(SDL_UpdateTexture(found->second.image->texture.get(), nullptr, surface->pixels, surface->pitch));
        found->second.image->pixels = std::shared_ptr<SDL_Surface>(surface.release(), SDL_FreeSurface);found->second.image->gpu.reset();found->second.image->pixels_valid=true;found->second.image->texture_valid=true;++found->second.image->revision;
        found->second.x = x; found->second.y = y; ++impl_->stats.texture_updates;
    } else {
        impl_->image(id, impl_->upload_image(std::move(surface)), x, y);
        impl_->layer(id).text = true;
    }
}
void Scene::positioned_text(int id,const std::vector<PositionedText>& runs,SDL_Rect clip){
    if(!impl_->text_rasterizer)impl_->text_rasterizer=std::make_unique<TextRasterizer>();
    const auto cached=impl_->text_layouts.find(id);const auto existing=impl_->layers.find(id);
    auto same_run=[](const PositionedText& a,const PositionedText& b){
        const auto& x=a.style;const auto& y=b.style;
        auto color=[](SDL_Color a,SDL_Color b){return a.r==b.r&&a.g==b.g&&a.b==b.b&&a.a==b.a;};
        return a.text==b.text&&a.x==b.x&&a.y==b.y&&x.font_path==y.font_path&&x.font_size==y.font_size&&
            x.line_height==y.line_height&&x.letter_spacing==y.letter_spacing&&color(x.color,y.color)&&
            color(x.border_color,y.border_color)&&color(x.shadow_color,y.shadow_color)&&
            x.border_width==y.border_width&&x.shadow_offset.x==y.shadow_offset.x&&x.shadow_offset.y==y.shadow_offset.y&&x.native_font==y.native_font;
    };
    size_t append_from=0;
    if(cached!=impl_->text_layouts.end()&&existing!=impl_->layers.end()){
        const auto& c=cached->second;auto& l=existing->second;
        if(l.image&&l.image==l.original&&c.image.lock()==l.image&&c.revision==l.image->revision&&
           c.clip.x==clip.x&&c.clip.y==clip.y&&c.clip.w==clip.w&&c.clip.h==clip.h&&
           c.runs.size()<=runs.size()&&std::equal(c.runs.begin(),c.runs.end(),runs.begin(),same_run)){
            if(c.runs.size()==runs.size()){l.x=clip.x;l.y=clip.y;++impl_->stats.text_layout_reuses;return;}
            if(impl_->gpu_ops&&l.image->gpu&&l.image.use_count()==2)append_from=c.runs.size();
        }
    }
    auto remember=[&]{const auto& l=impl_->layer(id);impl_->text_layouts.insert_or_assign(id,Impl::TextLayout{runs,clip,l.image,l.image->revision});};
    if(impl_->gpu_ops){impl_->gpu_positioned(id,runs,clip,append_from);remember();return;}
    auto surface=impl_->text_rasterizer->positioned(runs,clip);
    auto found=impl_->layers.find(id);
    if(found!=impl_->layers.end()&&found->second.text&&found->second.image==found->second.original&&found->second.image.use_count()==2&&
       found->second.width==clip.w&&found->second.height==clip.h){
        check(SDL_UpdateTexture(found->second.image->texture.get(),nullptr,surface->pixels,surface->pitch));
        found->second.image->pixels=std::shared_ptr<SDL_Surface>(surface.release(),SDL_FreeSurface);found->second.image->gpu.reset();found->second.image->pixels_valid=true;found->second.image->texture_valid=true;++found->second.image->revision;
        found->second.x=clip.x;found->second.y=clip.y;++impl_->stats.texture_updates;
    }else{impl_->image(id,impl_->upload_image(std::move(surface)),clip.x,clip.y);impl_->layer(id).text=true;}
    remember();
}
SDL_Point Scene::measure_text(const std::string& utf8,const TextStyle& style) {
    if(!impl_->text_rasterizer)impl_->text_rasterizer=std::make_unique<TextRasterizer>();
    return impl_->text_rasterizer->measure(utf8,style);
}
SDL_Point Scene::measure_glyph(const std::string& text,const TextStyle& style){
    if(!impl_->text_rasterizer)impl_->text_rasterizer=std::make_unique<TextRasterizer>();if(style.native_font){auto cell=impl_->text_rasterizer->native_cell(text,style);return {cell->planes.color.width,cell->planes.color.height};}return impl_->text_rasterizer->measure(text,style);
}
std::filesystem::path Scene::default_font_path() {
    // Use a system Japanese font, never a font from a translation patch.
    static const char* candidates[] = {
        "/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"
    };
    for (const auto* path : candidates) if (std::filesystem::is_regular_file(path)) return path;
    throw std::runtime_error("no system Japanese font found; set TextStyle::font_path explicitly");
}
void Scene::erase(int id) { impl_->layers.erase(id);impl_->text_layouts.erase(id); }
void Scene::clear() { impl_->layers.clear();impl_->text_layouts.clear(); }
void Scene::set_position(int id, int x, int y) { auto& layer = impl_->layer(id); layer.x = x; layer.y = y; }
void Scene::set_size(int id, int width, int height) {
    if (width < 0 || height < 0 || width > 16384 || height > 16384)
        throw std::runtime_error("invalid layer destination size");
    auto& layer = impl_->layer(id);
    layer.destination_width = width; layer.destination_height = height; layer.resized = true;
}
void Scene::set_visible(int id, bool visible) { impl_->layer(id).visible = visible; }
void Scene::set_alpha(int id, int alpha) {
    impl_->layer(id).alpha = std::clamp(alpha, 0, 255);
}
void Scene::set_add_color(int id, uint32_t colorref) {
    auto& layer = impl_->layer(id); layer.add_color = colorref; layer.filter_dirty=true;
}
void Scene::set_sub_color(int id, uint32_t colorref) {
    auto& layer = impl_->layer(id); layer.sub_color = colorref; layer.filter_dirty=true;
}
void Scene::set_sepia(int id, bool enabled) { auto& layer = impl_->layer(id); layer.sepia = enabled; layer.filter_dirty=true; }
void Scene::set_monochrome(int id, bool enabled) { auto& layer = impl_->layer(id); layer.monochrome = enabled; layer.filter_dirty=true; }
void Scene::set_negaposi(int id, bool enabled) { auto& layer = impl_->layer(id); layer.negaposi = enabled; layer.filter_dirty=true; }
void Scene::create_filter(int id, int mode) {
    if (mode < 1 || mode > 4) throw std::runtime_error("invalid native GraphicFilter mode");
    Impl::Layer layer; layer.z = id; layer.filter_mode = mode;
    impl_->layers.insert_or_assign(id, std::move(layer));
}
int Scene::filter_type(int id) const { const auto mode = impl_->layer(id).filter_mode; return mode ? 10000+mode : 0; }
void Scene::set_image_type(int id, int type) {
    if (type<1 || type>5) throw std::runtime_error("invalid native image type");
    auto& layer=impl_->layer(id);
    if (!layer.image) throw std::runtime_error("native image type requires an image layer");
    layer.image_type=type;
}
int Scene::image_type(int id) const {
    const auto& layer=impl_->layer(id);return layer.filter_mode?10000+layer.filter_mode:layer.image_type;
}
void Scene::set_scale(int id, double x_per_mille, double y_per_mille) {
    if (!std::isfinite(x_per_mille) || !std::isfinite(y_per_mille) || std::abs(x_per_mille) > 1000000 || std::abs(y_per_mille) > 1000000)
        throw std::runtime_error("invalid layer scale");
    auto& layer = impl_->layer(id); layer.scale_x = x_per_mille/1000.0; layer.scale_y = y_per_mille/1000.0;
}
void Scene::set_scale_center(int id, int x, int y) { impl_->layer(id).scale_center = {x,y}; }
void Scene::set_rotate(int id, double angle_units) {
    if (!std::isfinite(angle_units) || !std::isfinite(float(angle_units))) throw std::runtime_error("invalid layer rotation");
    impl_->layer(id).rotation = angle_units;
}
void Scene::set_rotate_center(int id, int x, int y) { impl_->layer(id).rotate_center = {x,y}; }
void Scene::set_raster(int id,bool enabled,double amplitude,int frequency,double phase) {
    if(!std::isfinite(amplitude)||!std::isfinite(phase)||phase<INT32_MIN||phase>INT32_MAX)
        throw std::runtime_error("invalid raster parameters");
    auto& layer=impl_->layer(id);layer.raster_enabled=enabled;layer.raster_amplitude=amplitude;
    layer.raster_frequency=frequency;layer.raster_phase=phase;
}
void Scene::set_clip(int id, SDL_Rect clip) {
    auto& layer = impl_->layer(id);
    if (clip.x < 0 || clip.y < 0 || clip.w < 0 || clip.h < 0 ||
        int64_t(clip.x) + clip.w > layer.width || int64_t(clip.y) + clip.h > layer.height)
        throw std::runtime_error("layer source rectangle exceeds image bounds");
    layer.clip = clip;
    if (!layer.resized) { layer.destination_width = clip.w; layer.destination_height = clip.h; }
}
void Scene::set_z(int id, int z) { impl_->layer(id).z = z; }
void Scene::render() { impl_->render(); }
void Scene::draw() { render(); SDL_RenderPresent(renderer()); }
Scene::Frame Scene::snapshot() {
    impl_->compose();
    auto target = impl_->target(width(), height());
    { Impl::TargetScope scope(*impl_, target.get(), width(), height());
      check(SDL_RenderCopy(renderer(), impl_->composite.get(), nullptr, nullptr)); }
    Frame result; result.owner_ = impl_->lifetime;
    auto owner = result.owner_;
    result.texture_ = std::shared_ptr<SDL_Texture>(target.release(), [owner](SDL_Texture* value) { if (!owner.expired()) SDL_DestroyTexture(value); });
    ++impl_->stats.snapshots;
    return result;
}
void Scene::overlay(const Frame& frame, int alpha, int x, int y) {
    if (!frame || frame.owner_.lock() != impl_->lifetime) throw std::runtime_error("frame belongs to an expired or different renderer");
    check(SDL_SetTextureBlendMode(frame.texture_.get(), SDL_BLENDMODE_BLEND));
    check(SDL_SetTextureAlphaMod(frame.texture_.get(), uint8_t(std::clamp(alpha, 0, 255))));
    SDL_Rect destination{x, y, width(), height()};
    check(SDL_RenderCopy(renderer(), frame.texture_.get(), nullptr, &destination));
}
void Scene::render_frozen(const Frame& frame, int x, int y) {
    check(SDL_SetRenderDrawColor(renderer(), 0, 0, 0, 255)); check(SDL_RenderClear(renderer()));
    overlay(frame, 255, x, y);
}
void Scene::render_transition(const Frame& old,int mode,double progress,const Bytes& user_mask,const SDL_Rect* rectangle,const TransitionSampling& sampling) {
    if (!old||old.owner_.lock()!=impl_->lifetime)throw std::runtime_error("transition frame belongs to another renderer");
    if (!std::isfinite(progress))throw std::runtime_error("invalid transition progress");
    const bool user=!user_mask.empty(),gpu=impl_->info.accelerated;
    if(mode<1||mode>(user?2:32))throw std::runtime_error("invalid native transition mode");
    SDL_Rect area=rectangle?*rectangle:SDL_Rect{0,0,width(),height()};
    SDL_Rect full{0,0,width(),height()},clipped{};
    if(!SDL_IntersectRect(&area,&full,&clipped))clipped={0,0,0,0};
    const double p=std::clamp(progress,0.0,1.0);
    // Native 15..28 write the CPU backbuffer without allocating an old GPU
    // layer. Their accelerated frame therefore consists of the new scene.
    if(!user&&mode>=15&&mode<=28&&gpu){render();return;}
    render();if(p>=1)return;
    if(!gpu&&!user){
        Bytes previous(size_t(width())*height()*4),next(previous.size());
        {Impl::TargetScope scope(*impl_,old.texture_.get(),width(),height());check(SDL_RenderReadPixels(renderer(),nullptr,SDL_PIXELFORMAT_RGBA32,previous.data(),width()*4));}
        {Impl::TargetScope scope(*impl_,impl_->composite.get(),width(),height());check(SDL_RenderReadPixels(renderer(),nullptr,SDL_PIXELFORMAT_RGBA32,next.data(),width()*4));}
        impl_->stats.readbacks+=2;impl_->stats.readback_pixels+=uint64_t(width())*height()*2;
        if(mode==19||mode==20){
            auto owner=impl_->random_frame.lock();const auto& saved=impl_->random_area;
            if(owner!=old.texture_||impl_->random_mode!=mode||saved.x!=clipped.x||saved.y!=clipped.y||saved.w!=clipped.w||saved.h!=clipped.h){
                if(!impl_->transition_random)throw std::runtime_error("random transition requires original CRT random-stream provider");
                const int tile=mode==19?64:32,count=((clipped.w+tile-1)/tile)*((clipped.h+tile-1)/tile);
                std::vector<int> order(size_t(count),0);for(int i=0;i<count;++i)order[size_t(i)]=i;
                for(int i=0;i<count;++i){const size_t j=impl_->transition_random()%unsigned(count);std::swap(order[size_t(i)],order[j]);}
                impl_->random_order=std::move(order);impl_->random_frame=old.texture_;impl_->random_area=clipped;impl_->random_mode=mode;
            }
        }
        auto data=software_transition(previous,next,width(),height(),mode,p,clipped,sampling,impl_->random_order);
        Surface image(SDL_CreateRGBSurfaceWithFormatFrom(data.data(),width(),height(),32,width()*4,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
        if(!image)throw std::runtime_error(SDL_GetError());auto texture=impl_->upload(image.get());check(SDL_SetTextureBlendMode(texture.get(),SDL_BLENDMODE_NONE));
        check(SDL_RenderCopy(renderer(),texture.get(),nullptr,nullptr));return;
    }
    if(gpu&&!user&&mode>=29){
        // Original GPU helpers move the full snapshot; RECT controls the loop
        // extent, not the snapshot geometry. Source clipping is unchanged.
        const int extent=(mode<=30)?clipped.w:clipped.h;
        const int distance=int(p*extent);
        overlay(old,255,mode==29?-distance:mode==30?distance:0,mode==31?-distance:mode==32?distance:0);return;
    }
    const auto native_plan=transition_plan(mode,sampling,user,clipped);
    double opacity_phase=p;
    if(gpu){
        const int count=std::max(1,native_plan.steps),phase=std::min(count-1,int(p*count));
        if(user||mode==2)opacity_phase=double(phase+1)/(count+1);
        else opacity_phase=double(phase)/count;
    }
    const auto alpha=uint8_t(std::clamp(int(255.0-double(float(opacity_phase*1000.0))*0.255),0,255));
    if(!user&&mode==2){overlay(old,alpha);return;}
    if(impl_->gpu_ops){
        const auto key=std::make_pair(user?0:mode,user_mask);auto found=impl_->gpu_masks.find(key);
        if(found==impl_->gpu_masks.end()){
            std::shared_ptr<GpuPixels> input;
            if(user){Bytes bytes=user_mask;if(bytes.size()>=3&&std::memcmp(bytes.data(),"YKG",3)==0)bytes=parse_graphic(bytes).color;auto image=decode_image(std::move(bytes));if(image->w!=width()||image->h!=height())throw std::runtime_error("transition mask must match logical canvas dimensions");input=impl_->gpu_ops->upload(image.get());}
            std::array<int,16> parameters{};parameters[0]=mode;parameters[1]=user;
            if(impl_->gpu_masks.size()>=32)impl_->gpu_masks.clear();
            found=impl_->gpu_masks.emplace(key,impl_->gpu_image(impl_->gpu_ops->run(GpuOps::Mask,{},input,{},parameters,width(),height()))).first;
        }
        auto* mask=impl_->texture(found->second);auto* masked=impl_->transition_composite.get();
        {Impl::TargetScope scope(*impl_,masked,width(),height());
            check(SDL_SetTextureAlphaMod(old.texture_.get(),255));check(SDL_SetTextureBlendMode(old.texture_.get(),SDL_BLENDMODE_NONE));check(SDL_RenderCopy(renderer(),old.texture_.get(),nullptr,nullptr));
            const auto multiply_alpha=SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_ADD,SDL_BLENDFACTOR_ZERO,SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDOPERATION_ADD);
            check(SDL_SetTextureBlendMode(mask,multiply_alpha));check(SDL_SetTextureAlphaMod(mask,alpha));check(SDL_RenderCopy(renderer(),mask,nullptr,nullptr));
        }
        check(SDL_SetTextureBlendMode(masked,SDL_BLENDMODE_BLEND));check(SDL_RenderCopy(renderer(),masked,nullptr,nullptr));++impl_->stats.gpu_operations;return;
    }
    const auto key=std::make_pair(user?0:mode,user_mask);
    auto found=impl_->transition_masks.find(key);
    if(found==impl_->transition_masks.end()){
        Bytes values(size_t(width())*height());
        if(user){
            Bytes input=user_mask;
            if(input.size()>=3&&std::memcmp(input.data(),"YKG",3)==0)input=parse_graphic(input).color;
            auto image=decode_image(std::move(input));
            if(image->w!=width()||image->h!=height())throw std::runtime_error("transition mask must match logical canvas dimensions");
            for(int y=0;y<height();++y)for(int x=0;x<width();++x){const auto* pixel=static_cast<uint8_t*>(image->pixels)+y*image->pitch+x*4;
                // 40ae00 copies the DIB's low (blue) byte into texture alpha.
                values[size_t(y)*width()+x]=gpu?pixel[2]:uint8_t((unsigned(pixel[0])+pixel[1]+pixel[2])/3);}
        }else{
            static const SDL_Point order[16]={{0,0},{2,2},{0,2},{2,0},{1,1},{3,3},{1,3},{3,1},
                {1,0},{1,2},{3,0},{3,2},{0,1},{2,3},{2,1},{0,3}};
            for(int y=0;y<height();++y)for(int x=0;x<width();++x){int value=0;
                if(mode==1){for(int k=0;k<16;++k)if(x%4==order[k].x&&y%4==order[k].y)value=k*15;}
                else if(mode==3)value=(width()-x)*255/width();
                else if(mode==4)value=x*255/width();
                // Native 5/6 divide vertical coordinates by WIDTH, including
                // on nonsquare displays; retain that original behavior.
                else if(mode==5)value=(width()-y)*255/width();
                else if(mode==6)value=y*255/width();
                else if(mode>=7&&mode<=10){const int q=(mode<=8?x:y)%16;value=int((mode%2?16-q:q)*15.9375);}
                else if(mode>=11&&mode<=14){const int q=mode<=12?x:y;const int j=(16-q%16)%16;
                    const int k=q+17*j;value=int(double(k%(width()+256))*255.0/(width()+256));if(mode%2)value=255-value;}
                values[size_t(y)*width()+x]=uint8_t(value);
            }
        }
        found=impl_->transition_masks.emplace(key,std::move(values)).first;
    }
    const auto& mask=found->second;
    if(!gpu){
        Bytes previous(size_t(width())*height()*4),next(previous.size());
        {Impl::TargetScope scope(*impl_,old.texture_.get(),width(),height());
         check(SDL_RenderReadPixels(renderer(),nullptr,SDL_PIXELFORMAT_RGBA32,previous.data(),width()*4));}
        {Impl::TargetScope scope(*impl_,impl_->composite.get(),width(),height());
         check(SDL_RenderReadPixels(renderer(),nullptr,SDL_PIXELFORMAT_RGBA32,next.data(),width()*4));}
        impl_->stats.readbacks+=2;impl_->stats.readback_pixels+=uint64_t(width())*height()*2;
        const int step=std::clamp(int(p*256.0)+1,1,256);
        for(int y=height()-clipped.h;y<height();++y)for(int x=clipped.x;x<clipped.x+clipped.w;++x){
            const size_t i=size_t(y)*width()+x;
            const int ramp=mode==1?(int(mask[i])+step>=255?256:0):std::clamp(int(mask[i])+step*2-255,0,256);
            for(int c=0;c<3;++c){const int delta=(int(next[i*4+c])-previous[i*4+c])*ramp;
                // x86 SAR rounds a negative product toward minus infinity.
                previous[i*4+c]=uint8_t(int(previous[i*4+c])+(delta>=0?delta/256:-((-delta+255)/256)));}
            previous[i*4+3]=255;
        }
        Surface result(SDL_CreateRGBSurfaceWithFormatFrom(previous.data(),width(),height(),32,width()*4,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
        if(!result)throw std::runtime_error(SDL_GetError());auto texture=impl_->upload(result.get());
        check(SDL_SetTextureBlendMode(texture.get(),SDL_BLENDMODE_NONE));check(SDL_RenderCopy(renderer(),texture.get(),nullptr,nullptr));return;
    }
    // Build constant-alpha pixel runs. SDL then samples the old GPU texture;
    // no screen pixels are read or composited on the CPU.
    check(SDL_SetTextureAlphaMod(old.texture_.get(),255));check(SDL_SetTextureBlendMode(old.texture_.get(),SDL_BLENDMODE_BLEND));
    std::vector<SDL_Vertex> vertices;std::vector<int> indices;
    const auto opacity=[&](int x,int y){const int m=mask[size_t(y)*width()+x];
        return int((unsigned(m)*alpha+127)/255);
    };
    const auto quad=[&](int x,int y,int right,int a){if(!a)return;
        const int base=int(vertices.size());const SDL_Color color{255,255,255,uint8_t(a)};
        const float u=float(x)/width(),v=float(y)/height(),r=float(right)/width(),b=float(y+1)/height();
        vertices.push_back({{float(x),float(y)},color,{u,v}});vertices.push_back({{float(right),float(y)},color,{r,v}});
        vertices.push_back({{float(right),float(y+1)},color,{r,b}});vertices.push_back({{float(x),float(y+1)},color,{u,b}});
        for(int n:{0,1,2,0,2,3})indices.push_back(base+n);
        if(vertices.size()>=32768){check(SDL_RenderGeometry(renderer(),old.texture_.get(),vertices.data(),int(vertices.size()),indices.data(),int(indices.size())));vertices.clear();indices.clear();}
    };
    for(int y=0;y<height();++y)for(int x=0;x<width();){
        const bool outside=!gpu&&(x<clipped.x||x>=clipped.x+clipped.w||y<clipped.y||y>=clipped.y+clipped.h);
        const int a=outside?255:opacity(x,y);int right=x+1;
        while(right<width()){
            const bool next_outside=!gpu&&(right<clipped.x||right>=clipped.x+clipped.w||y<clipped.y||y>=clipped.y+clipped.h);
            if((next_outside?255:opacity(right,y))!=a)break;++right;
        }
        quad(x,y,right,a);x=right;
    }
    if(!vertices.empty())check(SDL_RenderGeometry(renderer(),old.texture_.get(),vertices.data(),int(vertices.size()),indices.data(),int(indices.size())));
}
TransitionTiming Scene::transition_timing(int mode,const TransitionSampling& sampling,bool user,const SDL_Rect* rectangle) const {
    if(mode<1||mode>(user?2:32))throw std::runtime_error("invalid native transition mode");
    const SDL_Rect full{0,0,width(),height()};SDL_Rect area=full;
    if(rectangle&&!SDL_IntersectRect(rectangle,&full,&area))area={0,0,0,0};
    const auto plan=transition_plan(mode,sampling,user,area);
    const auto steps=uint32_t(std::max(0,plan.steps)),interval=uint32_t(std::max(0,plan.interval));
    // Deadline-based native loops may skip presentations when late. This is
    // their nominal total duration; OS scheduling/rendering overhead is external.
    const uint64_t total=plan.total_duration?(user?uint64_t(uint32_t(sampling.duration)):uint64_t(interval)):uint64_t(steps)*interval;
    return {steps,plan.total_duration?(steps?uint32_t(total/steps):0U):interval,total,plan.total_duration};
}
void Scene::set_transition_random(std::function<uint32_t()> random) { impl_->transition_random=std::move(random);impl_->random_frame.reset(); }
int Scene::effect_steps(int mode) {
    if (mode<1||mode>6) throw std::runtime_error("invalid native Effect mode");
    return mode<=4?4:12;
}
SDL_Point Scene::effect_offset(int mode, int amplitude, unsigned step) {
    const auto steps=effect_steps(mode);
    if (step>=unsigned(steps)) return {0,0};
    // Native x86 negation is modulo 2^32, including INT_MIN.
    const auto negative=[](int value) { const uint32_t bits=0U-uint32_t(value);int32_t result;std::memcpy(&result,&bits,4);return int(result); };
    if (mode<=4) {
        const int a=(step&1)?negative(amplitude):amplitude;
        if(mode==1)return {0,a};
        if(mode==2)return {a,0};
        if(mode==3)return {a,negative(a)};
        return {negative(a),negative(a)};
    }
    const int a=mode==5?amplitude/2:amplitude, n=negative(a), half=a/2, nhalf=negative(half);
    const SDL_Point offsets[12]={{n,n},{a,n},{n,a},{a,a},{nhalf,nhalf},{a,a},
        {a,n},{n,n},{n,half},{0,n},{nhalf,a},{a,n}};
    return offsets[step];
}
uint32_t Scene::effect_step_ms(int mode, int duration, bool whole_duration, bool accelerated) {
    const auto count=effect_steps(mode);
    uint32_t delay=duration<0?uint32_t(mode<=4?100:50):uint32_t(duration);
    if(whole_duration)delay=mode<=4?uint32_t(std::max(duration,0))>>2:uint32_t(std::max(duration,1))/uint32_t(count);
    return accelerated?std::max(15U,delay):delay;
}
void Scene::render_effect(const Frame& frame,int mode,int amplitude,unsigned step) {
    const auto offset=effect_offset(mode,amplitude,step);render_frozen(frame,offset.x,offset.y);
}
RendererInfo Scene::renderer_info() const { return impl_->info; }
RenderStats Scene::render_stats() const { auto stats=impl_->stats;if(impl_->gpu_ops){stats.gpu_submissions=impl_->gpu_ops->submissions();stats.gpu_uploads=impl_->gpu_ops->uploads();}if(impl_->text_rasterizer){stats.glyph_cache_hits=impl_->text_rasterizer->cache_hits+impl_->text_rasterizer->native_fonts.cache_hits();stats.glyph_rasterizations=impl_->text_rasterizer->rasterizations+impl_->text_rasterizer->native_fonts.rasterizations();}return stats; }
void Scene::capture(const std::filesystem::path& path) {
    impl_->render();
    int w{}, h{};
    check(SDL_GetRendererOutputSize(renderer(), &w, &h));
    if (w <= 0 || h <= 0 || uint64_t(w)*h > 64000000) throw std::runtime_error("capture dimensions exceed limit");
    Bytes pixels(size_t(w)*h*4);
    check(SDL_RenderReadPixels(renderer(), nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(), w*4));
    ++impl_->stats.readbacks; impl_->stats.readback_pixels += uint64_t(w)*h;
    png_image png{};
    png.version = PNG_IMAGE_VERSION; png.width = unsigned(w); png.height = unsigned(h); png.format = PNG_FORMAT_RGBA;
    if (!png_image_write_to_file(&png, path.string().c_str(), 0, pixels.data(), 0, nullptr))
        throw std::runtime_error(std::string("cannot write scene capture: ") + png.message);
    SDL_RenderPresent(renderer());
}
Bytes Scene::thumbnail(int width, int height) {
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384 || uint64_t(width)*height > 16000000)
        throw std::runtime_error("invalid thumbnail dimensions");
    impl_->compose();
    auto target = impl_->target(width, height);
    Bytes pixels(size_t(width)*height*4);
    { Impl::TargetScope scope(*impl_, target.get(), width, height);
      check(SDL_RenderCopy(renderer(), impl_->composite.get(), nullptr, nullptr));
      check(SDL_RenderReadPixels(renderer(), nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(), width*4)); }
    ++impl_->stats.readbacks; impl_->stats.readback_pixels += uint64_t(width)*height;
    png_image png{};
    png.version = PNG_IMAGE_VERSION; png.width = unsigned(width); png.height = unsigned(height); png.format = PNG_FORMAT_RGBA;
    png_alloc_size_t count{};
    if (!png_image_write_to_memory(&png, nullptr, &count, 0, pixels.data(), width*4, nullptr))
        throw std::runtime_error(std::string("cannot size thumbnail PNG: ") + png.message);
    Bytes result(count);
    if (!png_image_write_to_memory(&png, result.data(), &count, 0, pixels.data(), width*4, nullptr))
        throw std::runtime_error(std::string("cannot encode thumbnail PNG: ") + png.message);
    result.resize(count);
    return result;
}
int Scene::width() const { return impl_->width; }
int Scene::height() const { return impl_->height; }
SDL_Point Scene::size(int id) const {
    const auto& layer = impl_->layer(id);
    return {layer.width, layer.height};
}
SDL_Renderer* Scene::renderer() const { return impl_->renderer.get(); }
SDL_Window* Scene::window() const { return impl_->window.get(); }
}

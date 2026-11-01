#include "yukari/formats.hpp"
#include <SDL.h>
#include <png.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace yukari {
namespace {
struct QuitSDL { ~QuitSDL() { SDL_Quit(); } };
using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;
Surface image(Bytes bytes) {
    if (bytes.size() >= 4 && std::memcmp(bytes.data(), "\x89GNP", 4) == 0) {
        bytes[1] = 'P'; bytes[2] = 'N'; bytes[3] = 'G';
    }
    if (bytes.size() >= 8 && png_sig_cmp(bytes.data(), 0, 8) == 0) {
        png_image p{};
        p.version = PNG_IMAGE_VERSION;
        if (!png_image_begin_read_from_memory(&p, bytes.data(), bytes.size()))
            throw std::runtime_error("invalid PNG header");
        struct Cleanup { png_image* p; ~Cleanup() { png_image_free(p); } } cleanup{&p};
        // Limit decoded allocation as well as compressed input size.
        if (!p.width || !p.height || p.width > 16384 || p.height > 16384 || uint64_t(p.width)*p.height > 64000000)
            throw std::runtime_error("PNG dimensions exceed limit");
        p.format = PNG_FORMAT_RGBA;
        Bytes rgba(PNG_IMAGE_SIZE(p));
        if (!png_image_finish_read(&p, nullptr, rgba.data(), 0, nullptr))
            throw std::runtime_error("invalid PNG payload");
        Surface s(SDL_CreateRGBSurfaceWithFormat(0, static_cast<int>(p.width), static_cast<int>(p.height), 32, SDL_PIXELFORMAT_RGBA32), SDL_FreeSurface);
        if (!s) throw std::runtime_error(SDL_GetError());
        for (unsigned y = 0; y < p.height; ++y)
            std::memcpy(static_cast<uint8_t*>(s->pixels) + y*s->pitch, rgba.data() + size_t(y)*p.width*4, size_t(p.width)*4);
        return s;
    }
    if (bytes.size() >= 26 && bytes[0] == 'B' && bytes[1] == 'M') {
        // SDL supports additional BMP variants; bound dimensions before decoding.
        if (u32(bytes,14) < 40) throw std::runtime_error("legacy BMP DIB header unsupported");
        const auto w = u32(bytes,18), raw_h = u32(bytes,22);
        int32_t signed_h;
        std::memcpy(&signed_h, &raw_h, 4);
        int64_t h = signed_h < 0 ? -int64_t(signed_h) : signed_h;
        if (!w || !h || w > 16384 || h > 16384 || uint64_t(w)*h > 64000000)
            throw std::runtime_error("BMP dimensions exceed limit");
        SDL_RWops* rw = SDL_RWFromConstMem(bytes.data(), static_cast<int>(bytes.size()));
        if (!rw) throw std::runtime_error(SDL_GetError());
        Surface s(SDL_LoadBMP_RW(rw, 1), SDL_FreeSurface);
        if (!s) throw std::runtime_error(SDL_GetError());
        return s;
    }
    throw std::runtime_error("unsupported image payload (expected BMP/PNG/GNP)");
}
}
void preview(const Bytes& bytes, const std::string& title, unsigned frame_limit) {
    Bytes color = bytes;
    if (bytes.size() >= 6 && std::memcmp(bytes.data(), "YKG000", 6) == 0) {
        auto g = parse_graphic(bytes);
        color = std::move(g.color);
        if (!g.alpha.empty() || !g.frames.empty())
            std::cerr << "Color-plane preview: separate alpha mask and animation are not applied.\n";
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) throw std::runtime_error(SDL_GetError());
    QuitSDL quit;
    auto s = image(std::move(color));
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         std::min(s->w, 1280), std::min(s->h, 800), SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI), SDL_DestroyWindow);
    if (!window) throw std::runtime_error(SDL_GetError());
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
        SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC), SDL_DestroyRenderer);
    if (!renderer) renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_SOFTWARE));
    if (!renderer) throw std::runtime_error(SDL_GetError());
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(
        SDL_CreateTextureFromSurface(renderer.get(), s.get()), SDL_DestroyTexture);
    if (!texture) throw std::runtime_error(SDL_GetError());
    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
    if (SDL_RenderSetLogicalSize(renderer.get(), s->w, s->h) != 0) throw std::runtime_error(SDL_GetError());
    std::cout << "Preview " << s->w << 'x' << s->h << '\n';
    bool running = true;
    unsigned frames = 0;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) running = false;
        SDL_SetRenderDrawColor(renderer.get(), 32, 32, 32, 255);
        SDL_RenderClear(renderer.get());
        SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
        SDL_RenderPresent(renderer.get());
        if (frame_limit && ++frames >= frame_limit) break;
        SDL_Delay(10);
    }
}
}

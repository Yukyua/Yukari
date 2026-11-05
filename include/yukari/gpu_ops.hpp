#pragma once
#include <SDL.h>
#include <memory>
#include <array>
namespace yukari {
// Metal-owned immutable RGBA image. No host pixel mirror is required.
struct GpuPixels { struct Impl; std::shared_ptr<Impl> impl; int width{},height{}; };
class GpuOps {
public:
    enum Operation { Filter=1, Paint=2, Tint=3, Blit=4, Mask=5, TextOver=6, SelfBlit=7, Clear=8, AlphaMask=9 };
    static std::unique_ptr<GpuOps> create(SDL_Renderer* renderer);
    ~GpuOps();
    std::shared_ptr<GpuPixels> upload(const SDL_Surface* pixels);
    std::shared_ptr<GpuPixels> run(Operation op,const std::shared_ptr<GpuPixels>& destination,
        const std::shared_ptr<GpuPixels>& source,const std::shared_ptr<GpuPixels>& mask,
        const std::array<int,16>& parameters,int width,int height);
    // Writes a complete SDL target through its public Metal encoder. Caller must
    // switch targets after this call before issuing any SDL drawing commands.
    void write_target(SDL_Texture* target,const std::shared_ptr<GpuPixels>& image);
    void read(const std::shared_ptr<GpuPixels>& image,SDL_Surface* output);
    uint64_t submissions() const;
    uint64_t uploads() const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit GpuOps(SDL_Renderer* renderer);
};
}

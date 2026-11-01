#pragma once
#include "yukari/sprite_motion.hpp"
#include <functional>
#include <map>
#include <memory>

namespace yukari {
// Original SnowEffect/SakuraEffect state from 0x417780. No SDL ownership.
class ParticleModel {
public:
    enum class Mode { None=0, Snow=1, Sakura=2 };
    struct Rect { int32_t left{},top{},right{},bottom{}; };
    struct Particle {
        uint64_t serial{};
        std::string resource;
        std::shared_ptr<const SpriteGroups> groups;
        SpriteMotion motion;
        double precise_x{},precise_y{},velocity_x{},velocity_y{};
        int32_t x{},y{},width{},height{};
        uint64_t animation_clock{};
        const Frame& frame() const;
    };
    struct Layer {
        bool enabled{};
        std::vector<Particle> particles;
    };
    using Position = std::function<std::pair<double,double>(int)>;
    void set_random(std::function<uint32_t()> random);
    // A zero/negative period would make the original countdown loop hang.
    // Reject it, invalid rectangles, missing groups and more than ten slots.
    // Configuration preserves particles already attached to existing layers.
    void configure(Mode mode,std::string resource,int32_t period,Rect rect,
                   const std::vector<int>& layer_ids,SpriteGroups groups);
    void stop_snow();
    // Native SakuraEffect() also deletes its configured layers. Caller must
    // remove the returned IDs from its Scene/VM layer collection.
    std::vector<int> stop_sakura();
    void erase_layer(int id);
    void clear();
    // The original sprite delta uses raw DWORD delta + collection accumulator,
    // without graphics-speed multiplication. Deferred frames still spawn and
    // advance animation; only position integration is deferred.
    bool tick(uint32_t delta_ms,uint32_t accumulated_ms=0,bool defer_motion=false,
              const Position& layer_position={});
    const std::map<int,Layer>& layers() const {return layers_;}
    Mode mode() const {return mode_;}
    int32_t countdown() const {return countdown_;}
    int32_t wind() const {return wind_;}
    uint64_t rejected_spawns() const {return rejected_spawns_;}
    using ResourceLoader = std::function<SpriteGroups(const std::string&)>;
    Bytes snapshot() const;
    // Resource metadata is resolved once per name. Restores atomically and
    // retains the caller's existing CRT callback/state.
    void restore(const Bytes& bytes,const ResourceLoader& loader);
private:
    uint32_t random();
    int32_t range(int32_t first,int32_t last);
    Particle spawn(size_t slot);
    Mode mode_{Mode::None};
    std::string resource_;
    std::shared_ptr<const SpriteGroups> groups_;
    int32_t period_{},countdown_{},wind_{};
    Rect rect_;
    std::vector<int> slots_;
    std::map<int,Layer> layers_;
    std::function<uint32_t()> random_;
    uint64_t next_serial_{1},rejected_spawns_{};
};
}

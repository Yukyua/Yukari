#pragma once
#include "yukari/vm.hpp"
#include <array>
#include <map>
namespace yukari {
struct AreaLayer {bool visible{};double x{},y{};};
struct AreaBounds {int32_t left{},top{},right{},bottom{};};
struct AreaSprites {
    int32_t parent{},source{},x{},y{},normal_frame{},hover_frame{},pressed_frame{};
    std::array<int32_t,3> sounds{};
};
struct AreaDefinition {
    // Exact native 0x44-byte area record: enabled,parent,L,T,R,B,
    // sprite_source,sprite_x,sprite_y,normal,hover,alternate,extra,
    // sound1,sound2,sound3,flags. Preserve positional unknowns rather than
    // assuming an unverified semantic name for every extra field.
    std::array<int32_t,17> native{};
    int32_t id{};
    std::string script;
    bool mouse_area{},history_indication{};
    std::vector<int32_t> keys; // OR alternatives; zero never matches native VK
};
struct AreaState {
    std::map<std::string,AreaDefinition> builtins;
    std::vector<AreaDefinition> scripts;
    int32_t history_page_size{2}; // native constructor 0x42913f
};
struct AreaHooks {
    std::function<std::optional<AreaLayer>(int32_t)> layer;
    // Additional scheduler policy (save/skip/history availability) belongs to
    // the runtime; callers can gate a registered area without rewriting it.
    std::function<bool(const std::string&)> eligible;
    std::function<void(const std::string&,const AreaState&)> changed;
};
class AreaServices {
public:
    explicit AreaServices(AreaHooks hooks={});
    std::optional<Value> call(const std::string& name,const std::vector<Value>& args);
    const AreaDefinition* definition(const std::string& name) const;
    const AreaDefinition* script_area(int32_t id) const;
    AreaSprites sprites(const AreaDefinition& area) const;
    // Native 0x4339c0 requires enabled area + existing visible parent. Its
    // truncated parent translation is added to the registered local rectangle.
    std::optional<AreaBounds> bounds(const AreaDefinition& area) const;
    bool hit(const std::string& name,int32_t x,int32_t y) const;
    // Actionable, nonempty script names; indication-only regions are exposed
    // separately for hover rendering. Ordinary input excludes history bit31.
    std::vector<int32_t> hit_scripts(int32_t x,int32_t y,bool mouse_move=false) const;
    std::vector<int32_t> hovered_regions(int32_t x,int32_t y) const;
    std::vector<int32_t> key_scripts(int32_t virtual_key,bool repeated=false) const;
    const AreaState& state() const {return state_;}
    void restore(AreaState state);
private:
    AreaHooks hooks_;
    AreaState state_;
    void changed(const std::string& name);
};
}

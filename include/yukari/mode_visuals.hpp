#pragma once
#include "yukari/text_flow.hpp"
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace yukari {
enum class ModeIndicator : unsigned { Auto=0,AlreadySkip=1,Skip=2,Already=3 };
enum class ModeGraphic : unsigned { Auto=0,Skip=1 };
struct ModeIconRecord {
    int parent{},x{},y{},source{},group{};
    bool configured()const{return parent||x||y||source||group;}
};
struct ModeGraphicState {
    bool enabled{},applied{};
    int layer{};
    std::string replacement,backup;
};
struct ModeVisualState {
    std::array<ModeIconRecord,4> icons;
    std::array<bool,4> icon_instances{};
    std::array<ModeGraphicState,2> graphics;
};
struct ModeVisualHooks {
    // nullopt means absent layer; an existing layer may have an empty name.
    std::function<std::optional<std::string>(int)> layer_resource;
    // Original416860 replaces image/mask without resetting layer position,
    // alpha, text or transform attributes; external mask is explicitly empty.
    // Resource-specific centering offsets follow the host's native loader.
    std::function<void(int,const std::string&,const std::string&)> reload_image;
    // Return false when source/parent is absent. Native instance uses mode0,
    // visible1, kind1, sort-order1 and the requested group/local coordinates.
    std::function<bool(ModeIndicator,const ModeIconRecord&)> create_icon;
    std::function<void(ModeIndicator,const ModeIconRecord&)> remove_icon;
};
class ModeVisuals {
public:
    explicit ModeVisuals(ModeVisualHooks hooks={});
    void set_icon(ModeIndicator mode,ModeIconRecord record);
    void set_graphic(ModeGraphic mode,int layer,std::string replacement);
    // Native no-argument registration disables the consumer, retaining its
    // applied latch and backup; it does not restore an already-swapped image.
    void disable_graphic(ModeGraphic mode);
    void update(uint8_t auto_mode,uint8_t already_skip,uint8_t skip);
    void update_already(bool current_word_was_read);
    void update_graphic(ModeGraphic mode,uint8_t active);
    bool icon_present(ModeIndicator mode)const;
    const ModeVisualState& state()const{return state_;}
    void restore(ModeVisualState state);
    void clear_icon_instances(); // Scene teardown invalidates native pointers.
private:
    ModeVisualState state_;
    ModeVisualHooks hooks_;
    void update_icon(ModeIndicator mode,uint8_t requested);
};
// KeyWaitIconPositionSet stores mode/offsets in layer138/130/134. The returned
// coordinates are layer-local; native mode2 sprite uses system.ykg and the VM's
// KeyWaitIconNumber group. Fallback is retained for out-of-range native modes.
std::pair<int32_t,int32_t> key_wait_icon_position(int32_t mode,int32_t offset_x,int32_t offset_y,
                                                const TextFlow& flow,int32_t fallback_x=0,int32_t fallback_y=0);
}

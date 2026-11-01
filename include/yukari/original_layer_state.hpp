#pragma once
#include "yukari/motion_model.hpp"

namespace yukari {
// The confirmed 0x140-byte native layer header fields from 415c00/416fe0.
// Unknown bytes remain in the caller's header template. Raw flag bytes are
// retained, rather than normalizing them merely by decoding/encoding a record.
struct OriginalLayerState {
    int32_t image_type{1},x{},y{},transmittance{};
    uint8_t visible{1};
    uint32_t add_color{},sub_color{};
    uint8_t sepia{},monochrome{},negative{};
    int32_t scroll_mode{};
    double scroll_x{},scroll_y{};
    uint8_t scale_enabled{},scale_scrolling{};
    double scale_x{1},scale_y{1},scale_velocity_x{},scale_velocity_y{},scale_target_x{1},scale_target_y{1};
    int32_t scale_center_x{},scale_center_y{};
    double angle{},angular_velocity{};
    int32_t rotate_center_x{},rotate_center_y{},rotate_mode{},rotate_target{};
    uint8_t raster_enabled{};
    int32_t raster_target{},raster_frequency{},raster_period{};
    // Reader leaves scroll bounds untouched. Native raster phase, remaining
    // tween duration and velocity are reset to zero rather than serialized.
    void apply_motion(MotionModel& motion) const;
    MotionModel motion() const;
    void capture_motion(const MotionModel& motion);
};
OriginalLayerState decode_original_layer_state(const Bytes& header);
Bytes encode_original_layer_state(const OriginalLayerState& state,const Bytes& preserved_header={});
// Writer 415dda saves the tween target when velocity is nonzero; the reader
// restores a fixed transmittance and clears the tween's velocity/target.
int32_t original_saved_transmittance(double current,double velocity,double target);
}

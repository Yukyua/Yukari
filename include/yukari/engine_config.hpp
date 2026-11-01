#pragma once
#include "yukari/formats.hpp"
#include <optional>

namespace yukari {
struct EngineConfigEntry {
    std::string key;
    std::vector<std::string> values; // decoded UTF-8 tokens, in original order
    uint32_t line{};
    uint64_t byte_offset{};
    bool recognized=false;
};
struct EngineConfigPoint {int32_t x{},y{};};
struct EngineConfig {
    // Absent optionals mean retain the caller's established engine defaults.
    // Scalar duplicates replace; list duplicates append, matching42c7e0.
    std::optional<int32_t> transition_wait_type,font_border_thick_size,font_shadow_thick_size;
    std::optional<int32_t> skip_scroll_speed;
    std::optional<bool> mode_exclusive,window_size_variable,sound_volume_type;
    std::optional<EngineConfigPoint> window_size,history_sound_icon_position;
    std::optional<int32_t> history_sound_icon_position_type,history_line_margin,history_rows_margin;
    std::optional<std::string> normal_cursor,save_data_directory;
    std::optional<bool> save_data_into_key_wait_icon_number,save_data_into_select_point_data;
    std::optional<bool> save_data_into_history_data,save_data_without_script_data;
    std::vector<int32_t> sprite_without_graphic_attribute;
    std::vector<int32_t> save_data_without_graphic_transmittance,save_data_without_graphic_layer;
    std::vector<EngineConfigEntry> entries; // Includes unknown keys; never silently claims their semantics.
};
// Original flat default.ini lexer: space/tab/comma split tokens, // comments
// anywhere, case-sensitive keys, no quotes/backslash escaping/INI sections.
// Bounds reject original stack-buffer overflow cases. Parsing is memory-only.
EngineConfig parse_engine_config(const Bytes&,const char* encoding="CP932");
// Windows32 CRT strtol(base0), including octal/hex, trailing junk/invalid=>0
// and signed overflow saturation. Host long width does not affect the result.
int32_t engine_config_integer(const std::string& token);
}

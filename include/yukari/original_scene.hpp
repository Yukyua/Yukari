#pragma once
#include "yukari/formats.hpp"

namespace yukari {
// Read-only native save records, derived from original 0x415c00/0x45eb30.
// Offsets in a nested save remain relative to that nested container's base.
struct OriginalSprite {
    int kind{}, x{}, y{}, width{}, height{}, left{}, top{}, right{}, bottom{};
    int wait{}, source_layer{-1}, mode{}, group{}, frame{}, slot{};
    uint16_t encoded_character{}; // Native byte pair; retain original encoding.
    int font{};
    uint32_t color{};
    int font_effect{}; // kind3 union at saved+0x48; <=0 inherits default
    uint32_t shadow_color=0xffffffffU,border_color=0xffffffffU;
    bool visible{}, named{};
    Bytes native_header;
    Bytes payload; // Animation overrides or text bytes, according to kind.
    std::vector<Frame> animation;
};
struct OriginalLayer {
    int id{}, image_type{1}, x{}, y{}, transmittance{};
    bool visible{};
    std::string resource, mask;
    Bytes native_header; // Retain unsupported text/filter/scroll settings.
    std::vector<OriginalSprite> sprites;
};
struct OriginalSound {
    int id{}, volume{}, parameter{};
    bool playing{}, loop{};
    uint8_t mode{};
    std::string resource;
    Bytes native_header;
};
struct OriginalScene {
    std::vector<OriginalLayer> layers;
    std::vector<OriginalSound> sounds;
};
OriginalScene decode_original_scene(const Bytes& bytes, size_t container_base = 0);
// Appends native records and updates the existing state header in a Yuka
// container. Unknown header fields survive round trips. UTF-8 resource names
// must be representable in CP932. Failure leaves bytes unchanged.
void encode_original_scene(Bytes& bytes, const OriginalScene& scene, size_t container_base = 0);
std::string encode_original_text(const std::string& utf8, const char* encoding = "CP932");
}

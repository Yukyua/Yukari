#pragma once
#include "yukari/settings_services.hpp"
#include <functional>
namespace yukari {
struct SavePreviewDate {
    int year{},month{},day{},hour{},minute{},second{};
    bool valid{}; // Native FileTimeToSystemTime, UTC rather than localtime.
};
struct SavePreviewMetadata {
    bool exists{};
    Bytes thumbnail;
    std::string comment1,comment2; // Decoded original metadata strings.
    SavePreviewDate date;
};
struct SavePreviewRect {int left{},top{},right{},bottom{};bool empty()const{return left>=right||top>=bottom;}};
struct SavePreviewText {
    enum Kind {Date,Comment1,Comment2} kind;
    std::string text;
    int font{},effect{};
    uint32_t color{}; // Raw native color value, including global-color tokens.
    int x{},y{}; // Parent-local coordinates.
    // Nonempty Comment2 rectangle requests the native glyph-flow/kinsoku/
    // ellipsis path. Renderer hook must use its existing TextModel/GDI bridge.
    std::optional<SavePreviewRect> rect;
};
struct SavePreviewBitmap {Bytes image;int x{},y{},width{},height{};};
struct SavePreviewLayout {
    bool success{};
    int parent_x{},parent_y{};
    std::optional<SavePreviewBitmap> thumbnail;
    std::vector<SavePreviewText> text;
};
struct SavePreviewOptions {
    size_t comment1_skip_bytes{}; // Native DAT56c864, applied before decode.
    std::string encoding="CP932";
};
// Consumes every SaveDataDisplaySetTo* field. Missing settings use verified
// native data defaults. Does not mutate Scene, settings or original metadata.
SavePreviewLayout save_preview_layout(const SettingsState& settings,const SavePreviewMetadata& metadata,
                                      int parent_x,int parent_y,const SavePreviewOptions& options={});
struct SavePreviewDrawHooks {
    std::function<void(const SavePreviewBitmap&,int parent_x,int parent_y)> bitmap;
    std::function<void(const SavePreviewText&,int parent_x,int parent_y)> text;
};
// Draw hooks receive resolved font/effect/color and layout, allowing the host
// to bind Scene/FreeType and its exact native paragraph layout. No hidden font.
void draw_save_preview(const SavePreviewLayout&,const SavePreviewDrawHooks&);
}

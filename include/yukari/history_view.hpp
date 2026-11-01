#pragma once
#include "yukari/area_services.hpp"
#include "yukari/settings_services.hpp"
namespace yukari {
struct HistoryGlyph {
 int32_t layer{},kind{},x{},y{};
 uint32_t character{};
 int32_t font_or_image{};
 uint32_t color{};
};
struct HistoryPage {std::vector<HistoryGlyph> glyphs;std::string voice;};
// Original ring-buffer serialization: count DWORD, count*28-byte records,
// voice byte count DWORD, zero-terminated text. Explicit source encoding.
std::vector<HistoryPage> decode_history_pages(const Bytes&,const char* encoding="CP932");
}

namespace yukari {
struct HistoryExtent {int32_t width{},height{};};
struct HistoryLayer {int32_t x{},y{};AreaBounds text_rect{};};
struct HistoryOptions {
 bool window_enabled=false,text_color_enabled=false,voice_continue=false,exit_to_next=false,all_or_nothing_clip=false;
 int32_t window_layer=0,default_layer=0,rows_margin=30,line_margin=0;
 uint32_t text_color=0xffffffffU,voiced_text_color=0xffffffffU;
 std::optional<AreaBounds> window_rect,screen_rect;
 std::optional<int32_t> sound_number;
 std::optional<std::array<int32_t,3>> sound_icon; // source, normal group, hover group
 int32_t icon_position_type=9,icon_x=0,icon_y=0;
 std::string encoding="CP932";
 static HistoryOptions from_settings(const SettingsState&,int32_t default_layer);
};
struct HistoryDrawGlyph {
 size_t page{};HistoryGlyph glyph;int32_t layer{},x{},y{},width{},height{};
 std::string utf8;uint32_t color{};bool visible=true;
};
struct HistoryIconFrame {int32_t group{},x{},y{},width{},height{};};
struct HistoryDrawIcon {size_t page{};int32_t source{},normal{},hover{},x{},y{},width{},height{};std::optional<HistoryIconFrame> normal_frame,hover_frame;};
struct HistoryRegion {int32_t action{};AreaBounds bounds{};bool disabled=false;std::optional<size_t> page;std::optional<AreaSprites> sprites;};
struct HistoryLayout {
 std::vector<HistoryDrawGlyph> glyphs;std::vector<HistoryDrawIcon> icons;std::vector<HistoryRegion> regions;
 std::vector<size_t> pages;
 std::optional<int32_t> right_exit_sound;
 size_t first{},last_start{};
 bool open=false;
};
struct HistoryHooks {
 std::function<std::optional<HistoryLayer>(int32_t)> layer;
 // Must use original per-record font ID, or original/graphic-font image index.
 std::function<HistoryExtent(const HistoryGlyph&)> glyph_extent;
 std::function<HistoryExtent(int32_t source,int32_t group)> icon_extent;
 std::function<bool(const std::string&,std::optional<int32_t>)> play_voice;
 std::function<void(uint32_t fade_ms)> stop_voice;
 std::function<void(uint32_t fade_ms)> stop_other_voices;
};
class HistoryView {
public:
 explicit HistoryView(HistoryHooks hooks);
 void open(std::vector<HistoryPage>,HistoryOptions,const AreaServices* areas=nullptr);
 const HistoryLayout& layout()const{return layout_;}
 // Native labels: 2/4 prev, 3/5 next, 8/9 page prev/next, 10 head,
 // 11 last, 0/6/7 exit, >=10000 voice page. 12 redraw-only (native H bar).
 bool action(int32_t label);
 bool click(int32_t x,int32_t y);
 bool drag_vertical(int32_t y);
 void close();
 bool take_advance_request();
private:
 struct PageLayout {std::vector<HistoryDrawGlyph> glyphs;AreaBounds bounds{};int32_t height{};std::optional<HistoryDrawIcon> icon;int32_t voice_layer{};};
 HistoryHooks hooks_;HistoryOptions options_;const AreaServices* areas_=nullptr;
 std::vector<HistoryPage> pages_;std::vector<PageLayout> prepared_;HistoryLayout layout_;
 bool advance_=false,voice_active_=false;
 PageLayout prepare(size_t page);
 void rebuild();void move(int64_t amount);
};
}

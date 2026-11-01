#pragma once
#include "yukari/vm.hpp"
#include "yukari/text_flow.hpp"
#include <map>

namespace yukari {
struct TextFont {
    std::string family;
    int32_t height{}, width{}, weight{}, cell_width{}, line_height{};
};
struct TextRecord {
    // Original sprite types: glyph3, original-font4, graphic-font5, LF7.
    int32_t kind=3,x{},y{},width{},height{},font{},image_id{};
    std::string encoded; // source-codepage bytes, never UTF-8-width heuristics
    uint64_t style_token{}; // renderer-owned color/border/shadow identity
    bool history_recorded=false;
};
struct TextLayoutRules {
    // Source-encoded pairs/lists from native configuration; no guessed defaults.
    std::string kinsoku_pairs,word_run_pairs;
    std::string suppress_after_rollback=std::string("\x81\x40",2),indent_space=std::string("\x81\x40",2);
    std::vector<std::string> hanging_prefixes;
    bool protect_word_runs=false;
};
struct TextLayerState {
    TextFlow flow;
    int32_t font{}, hanging_indent{};
    std::vector<TextRecord> records;
    uint64_t style_token{};
    bool output_prepared=false,rollback_active=false;
    std::string pending_output;
};
// Original419f90 geometry: +28=x,+2c=y,+24=width,+20=height.
// Fourth argument is also LOGFONT height; +238/style is COLORREF (404340).
struct TextBoxSpec { int32_t value_28{},value_2c{},value_24{},font_height{},style{}; };
struct TextModelHooks {
    std::function<void(const TextBoxSpec&)> create_box;
    std::function<void()> delete_box;
    std::function<std::string()> get_box; // UTF-8 UI adapter
    std::function<void(const std::string&)> set_box;
    std::function<void(const std::string&)> register_original_font,register_graphic_font;
    std::function<std::pair<int32_t,int32_t>(int32_t,const std::string&)> measure;
    // Optional separate raster glyph bounding-box provider; measure is the
    // text extent provider for proportional strings and fallback glyphs.
    std::function<std::pair<int32_t,int32_t>(int32_t,const std::string&)> glyph_size;
    std::function<bool(uint8_t)> dbcs_lead;
    std::function<std::pair<int32_t,int32_t>(int32_t)> graphic_font_size,original_font_size;
};
// Evidence-backed layout/record model. Rasterization and native edit-control
// lifecycle are supplied by explicit hooks, never claimed by string storage.
class TextModel {
public:
    explicit TextModel(TextModelHooks hooks={},std::string encoding="CP932");
    void set_encoding(std::string encoding){encoding_=std::move(encoding);}
    TextLayerState& layer(int32_t id);
    const std::map<int32_t,TextLayerState>& layers()const{return layers_;}
    void erase_layer(int32_t id){layers_.erase(id);}
    const std::map<int32_t,TextFont>& fonts()const{return fonts_;}
    int32_t name_window()const{return name_window_;}
    int32_t ruby_font()const{return ruby_font_;}
    void glyph(int32_t id,const std::string& encoded_character,bool dbcs,int32_t bitmap_width,int32_t bitmap_height);
    void line_feed(int32_t id);
    void graphic_font(int32_t id,int32_t image_id,int32_t width,int32_t height);
    // Consume one source-encoded glyph/control. Page overflow must be serviced
    // by the caller: return0 with flow.overflow means wait, clear(id), then
    // retry exactly the same input. Prelude state survives this continuation.
    size_t output_step(int32_t id,const std::string& encoded);
    TextLayoutRules& rules(){return rules_;}
    const TextLayoutRules& rules()const{return rules_;}
    std::pair<int32_t,int32_t> measure(int32_t font,const std::string& encoded)const;
    void name_out(const std::string& utf8,bool initial_instruction=true);
    void reset_cursor(int32_t id); // 412d80 does not clear overflow or sprites
    void clear(int32_t id); // 415890 clears text/image/LF records and overflow
    std::string str_in(int32_t id)const;
    Bytes snapshot()const;
    void restore(const Bytes&); // validates fully before replacing model state
    const std::string& original_font_resource()const{return original_font_resource_;}
    const std::string& graphic_font_resource()const{return graphic_font_resource_;}
    std::optional<Value> call(const std::string&,const std::vector<Value>&);
private:
    TextModelHooks hooks_;std::string encoding_;
    std::map<int32_t,TextLayerState> layers_;std::map<int32_t,TextFont> fonts_;
    int32_t name_window_=0,ruby_font_=0;bool box_exists_=false;
    TextBoxSpec box_spec_{};std::string box_value_;
    std::string original_font_resource_,graphic_font_resource_;
    TextLayoutRules rules_;
    void metrics(TextLayerState&);
};
}

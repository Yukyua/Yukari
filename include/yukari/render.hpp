#pragma once
#include "yukari/formats.hpp"
#include "yukari/native_font.hpp"
#include <optional>
#include <SDL.h>
#include <memory>
#include <functional>

namespace yukari {
struct TextStyle {
    std::filesystem::path font_path; // Empty selects a system Japanese font.
    int font_size{27};
    int line_height{}; // Zero selects the font's normal line height.
    int letter_spacing{};
    SDL_Color color{255, 255, 255, 255};
    SDL_Color border_color{0, 0, 0, 255};
    int border_width{1};
    SDL_Color shadow_color{0, 0, 0, 160};
    SDL_Point shadow_offset{0, 0}; // Zero disables the shadow.
    std::optional<NativeFontSpec> native_font; // Original GDI font/effect contract.
};
enum class NativePixelKernel { Scalar, Mmx };
struct PositionedText {
    std::string text;
    int x{},y{}; // Absolute cell top-left. No automatic wrapping or border inset.
    TextStyle style;
};
struct RenderStats {
    uint64_t uploads{}, texture_updates{}, shared_loads{}, snapshots{}, readbacks{}, readback_pixels{};
    uint64_t glyph_cache_hits{}, glyph_rasterizations{};
    uint64_t gpu_operations{},gpu_submissions{},gpu_uploads{};
    uint64_t compositions{},composition_reuses{},text_layout_reuses{},text_appends{};
    uint64_t image_decodes{},resource_cache_hits{},resource_cache_bytes{};
};
struct TransitionTiming { uint32_t steps{}, step_ms{}; uint64_t total_ms{}; bool total_duration{}; };
struct TransitionSampling {
    int duration{100}; // Effective native helper argument, after wrapper quality scaling.
    int quality{2}; // Native DAT_56c32c.
    int timing_mode{}; // Native DAT_56c860.
};
struct RendererInfo { std::string name; bool accelerated{}, target_textures{}; };
// SDL backend only; script command interpretation belongs to the runtime.
class Scene {
public:
    // Opaque renderer-owned GPU image. Does not read pixels back to the CPU.
    class Frame {
    public:
        explicit operator bool() const { return texture_ && !owner_.expired(); }
    private:
        friend class Scene;
        std::shared_ptr<SDL_Texture> texture_;
        std::weak_ptr<int> owner_;
    };
    Scene(int width, int height, std::string title);
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    void set_title(const std::string& title);
    void configure_native_fonts(NativeFontOptions options);
    // PNG/GNP retain embedded alpha. Separate YKG masks use inverted average
    // RGB opacity multiplied by color alpha. This mask polarity is inferred
    // from original samples/layout, not complete native compositor evidence.
    void load(int id, Bytes bytes, int x = 0, int y = 0, const Bytes& extra_mask = {});
    // Immutable resource key, scoped to this Scene. Live layers share one upload.
    void load_cached(int id, const std::string& resource_key, const Bytes& bytes, int x = 0, int y = 0,
                     const Bytes& extra_mask = {});
    // Loader is invoked only on decoded-resource cache miss.
    void load_deferred(int id,const std::string& resource_key,std::function<Bytes()> loader,int x=0,int y=0,const Bytes& extra_mask={});
    // Share the full source image, with independent alpha, visibility and crop.
    void copy(int id, int source, int x = 0, int y = 0);
    // Host-generated RGBA canvas, initially transparent. These are composition
    // helpers, not the native GraphicCopy color-plane operation below.
    void create_canvas(int id,int width,int height,int x=0,int y=0);
    // Scale the source bitmap into destinationRect and alpha-over its pixels.
    // Ignores source placement/global alpha/crop and preserves destination
    // attributes. Modified pixels persist through layer_pixels().
    void paint(int destination,int source,SDL_Rect destination_rect);
    // Native kind4 OriginalFont 0x40cf90: PATCOPY replaces RGB and retains
    // the independent coverage mask/alpha. Type3 color-key images skip it.
    // Region uses PatBlt x,y,width,height; null fills the complete source.
    void set_glyph_color(int id,uint32_t colorref,const SDL_Rect* region=nullptr);
    // Own-save bitmap payload: current/original color planes and raw masks.
    // Restore after resource loading and color-effect setters; preserves all
    // placement/clip/transform state. Dimensions must match the loaded layer.
    Bytes layer_pixels(int id) const;
    // Decode/scale a resource into a PNG without creating a window/renderer.
    static Bytes image_thumbnail(const Bytes& image,int width=126,int height=70);
    // Runtime resource thumbnail: GPU scaling, followed by explicit PNG export.
    Bytes resource_thumbnail(const Bytes& image,int width=126,int height=70);
    void restore_layer_pixels(int id,const Bytes& bytes);
    // Native GraphicCopy color-plane operation (Metal compute on GPU). Source rect is unscaled;
    // neither layer's display transform/crop/global alpha participates.
    void blit(int destination, int x, int y, int source, SDL_Rect rect,
              NativePixelKernel kernel = NativePixelKernel::Scalar);
    // Exchange content/state between native ID/list slots, including a missing
    // side. Each slot retains its own drawing-order position.
    void swap_layers(int first, int second);
    // Native software prefix snapshot, not resource prefetch. Accelerated
    // original GraphicCache returns without creating a cache.
    void cache_prefix(int through_id);
    void clear_prefix_cache();
    bool prefix_cached() const;
    // Replaces a layer with UTF-8 text in a transparent fixed-size box.
    // Newlines and character wrapping are supported; excess lines are clipped.
    // FreeType layout is a portability substitute, not Windows GDI equivalence.
    void text(int id, const std::string& utf8, int x, int y, int width, int height,
              const TextStyle& style = {});
    // Logical advance width / line-height extent using the same FreeType face,
    // kerning and spacing as text(). No wrapping or effect padding is included.
    SDL_Point measure_text(const std::string& utf8, const TextStyle& style = {});
    SDL_Point measure_glyph(const std::string& utf8, const TextStyle& style = {});
    // Each run retains its own absolute cell origin and style. Only the absolute
    // clip rectangle becomes a layer, backed by one transparent GPU texture.
    void positioned_text(int id, const std::vector<PositionedText>& runs, SDL_Rect clip);
    static std::filesystem::path default_font_path();
    void erase(int id);
    void clear();
    void set_position(int id, int x, int y);
    // Resize only the destination; source clip and original size are retained.
    void set_size(int id, int width, int height);
    void set_visible(int id, bool visible);
    void set_alpha(int id, int alpha);
    // Native COLORREF (R in low byte). Each setting replaces its previous
    // value; filters recompute from original pixels, add before subtract.
    // 0xffffffff disables the respective operation, as in native 0x416a00.
    void set_add_color(int id, uint32_t colorref);
    void set_sub_color(int id, uint32_t colorref);
    void set_sepia(int id, bool enabled);
    void set_monochrome(int id, bool enabled);
    void set_negaposi(int id, bool enabled);
    // Replace this layer with native GraphicFilter mode 1..4 (type 10001..4).
    // These affect the already-composited prefix, not a source image. Native
    // accelerated rendering skips modes 1..3; software applies their pixel
    // helpers. Mode 4 is retained and has no pixel operation in the dispatcher.
    void create_filter(int id, int mode);
    int filter_type(int id) const;
    // Native image types 1..3 use normal composition; 4/5 are LoadADD/SUB.
    // Original accelerated drawing uses normal alpha for all five types.
    // Software 4/5 preserve the native mask endpoint and middle-value rules.
    void set_image_type(int id, int type);
    int image_type(int id) const;
    // Native transform units: scale 1000 is identity; rotation 2000 is a
    // complete clockwise turn. Centers are independent layer-local pixels,
    // defaulting to (0,0). Scale is applied before rotation.
    void set_scale(int id, double x_per_mille, double y_per_mille);
    void set_scale_center(int id, int x, int y);
    void set_rotate(int id, double angle_units);
    void set_rotate_center(int id, int x, int y);
    // Native quality2 row displacement; nonidentity scale/rotation suppress it.
    void set_raster(int id, bool enabled, double amplitude, int frequency, double phase);
    // Source rectangle; its size determines destination size until set_size.
    void set_clip(int id, SDL_Rect clip);
    void set_z(int id, int z);
    // Render the scene into the back buffer, leaving presentation to callers
    // that need to overlay transition effects.
    void render();
    void draw();
    Frame snapshot();
    void overlay(const Frame& frame, int alpha = 255, int x = 0, int y = 0);
    void render_frozen(const Frame& frame, int x = 0, int y = 0);
    // Render new layers plus the original backend's transition from old.
    // Nonempty mask selects UserTransition1/2. Progress is normalized 0..1.
    // Does not Present. Software uses original integer CPU helpers; GPU
    // follows their distinct original accelerated branches. Random software
    // modes19/20 require set_transition_random(), shared with the VM CRT stream.
    void render_transition(const Frame& old, int mode, double progress,
                           const Bytes& user_mask = {}, const SDL_Rect* rect = nullptr,
                           const TransitionSampling& sampling = {});
    void set_transition_random(std::function<uint32_t()> random);
    TransitionTiming transition_timing(int mode, const TransitionSampling& sampling,
                                       bool user_mask = false, const SDL_Rect* rect = nullptr) const;
    // Native Effect1..6 tables. Steps beyond their sequence return identity.
    static int effect_steps(int mode);
    static SDL_Point effect_offset(int mode, int amplitude, unsigned step);
    // mode flag corresponds to native DAT_56c860==1 (duration means whole effect).
    // Accelerated native helpers always sleep at least 15ms per step.
    static uint32_t effect_step_ms(int mode, int duration, bool whole_duration, bool accelerated);
    void render_effect(const Frame& frame, int mode, int amplitude, unsigned step);
    RendererInfo renderer_info() const;
    RenderStats render_stats() const;
    // Redraws and reads the back buffer before presenting; writes RGBA PNG.
    void capture(const std::filesystem::path& path);
    // Redraw and read before presentation, scale, and encode PNG in memory.
    Bytes thumbnail(int width = 126, int height = 70);
    int width() const;
    int height() const;
    SDL_Point size(int id) const;
    SDL_Renderer* renderer() const;
    SDL_Window* window() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}

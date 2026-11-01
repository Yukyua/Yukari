#pragma once
#include "yukari/text_model.hpp"
#include <SDL.h>
#include <memory>

namespace yukari {
// A real editable Cocoa child view, backed by NSTextField/its field editor.
// Methods and destruction must run on the SDL/AppKit main thread. The SDL
// window must outlive this provider. It never runs a second modal event loop.
class NativeTextBox {
public:
    using FontFamily = std::function<std::string()>;
    using KeyForwarder = std::function<void(int32_t virtual_key,bool down,bool repeat)>;
    using BlockedKey = std::function<bool(int32_t virtual_key)>;
    NativeTextBox(SDL_Window*,int logical_width=1024,int logical_height=576,
                  FontFamily default_font={},KeyForwarder forward={},BlockedKey blocked={});
    ~NativeTextBox();
    NativeTextBox(const NativeTextBox&)=delete;
    NativeTextBox& operator=(const NativeTextBox&)=delete;
    void bind(TextModelHooks&); // captures this; provider outlives the model
    void create(const TextBoxSpec&);
    void destroy();
    void set(const std::string& utf8);
    std::string get()const;
    bool exists()const;
    SDL_Rect geometry()const; // native logical x/y/width/height
    void sync_geometry(); // also follows parent view resize automatically
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}

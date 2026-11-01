#pragma once
#include "yukari/vm.hpp"
#include <SDL.h>
#include <array>

namespace yukari {
struct PlatformImage {
    int width{},height{};
    Bytes rgba;
    SDL_Point hotspot{};
};
// Decode an original Windows CUR/ICO containing PNG or uncompressed 24/32-bit
// DIB data. Other encodings are explicit unsupported errors.
PlatformImage decode_windows_cursor(const Bytes& bytes);
struct PlatformHooks {
    std::function<Bytes(const std::string&)> read_resource;
    std::function<PlatformImage(const std::string&)> cursor_image;
    // Original normal/busy defaults come from engine configuration, not OS stock cursors.
    std::array<std::string,2> default_cursor_names;
    std::function<PlatformImage(uint16_t)> icon_image;
    std::function<std::optional<std::filesystem::path>(const std::string&,const std::filesystem::path&)> extract_dialog;
    // ShellExecute("open",target,"",working_directory,10). A provider must
    // honor the state-directory output policy. Tests inject a nonexecuting hook.
    std::function<bool(const std::string&,const std::filesystem::path&)> execute;
    std::function<void(bool)> auto_region;
    std::function<void(int32_t)> cursor_auto_hide;
    std::function<bool()> foreground;
    std::function<SDL_Point()> logical_size;
};
class PlatformServices {
public:
    PlatformServices(SDL_Window* window,SDL_Renderer* renderer,
                     std::filesystem::path game_dir,std::filesystem::path state_dir,
                     PlatformHooks hooks={});
    ~PlatformServices();
    PlatformServices(const PlatformServices&)=delete;
    PlatformServices& operator=(const PlatformServices&)=delete;
    // Unknown command => nullopt. An absent required native/platform provider
    // raises an explicit exception instead of claiming that the call succeeded.
    std::optional<Value> call(const std::string& name,const std::vector<Value>& arguments);
    void set_busy_cursor(bool busy);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}

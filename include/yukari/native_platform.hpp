#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <SDL.h>
#include "yukari/formats.hpp"
namespace yukari {
std::optional<std::filesystem::path> native_extract_dialog(const std::string& source,const std::filesystem::path& suggested);
std::optional<size_t> native_popup_menu(SDL_Window* window,SDL_Point point,const std::vector<std::string>& labels);
std::vector<std::string> native_font_families();
std::filesystem::path native_font_file(const std::string& family);
void native_window_region(SDL_Window* window,const Bytes& png); // empty restores rectangle
bool native_shell_open(const std::string& target,const std::filesystem::path& working_directory);
}

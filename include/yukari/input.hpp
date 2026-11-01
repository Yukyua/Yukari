#pragma once
#include <SDL.h>
#include <vector>
namespace yukari {
// Win32 VK values are part of YKS data. Return both generic and sided modifier
// keys so bindings written for VK_SHIFT and VK_LSHIFT behave independently.
inline std::vector<int32_t> virtual_keys(const SDL_Keysym& key){
 const auto k=key.sym;
 if(k>=SDLK_a&&k<=SDLK_z)return {int32_t(k-SDLK_a+'A')};
 if(k>=SDLK_0&&k<=SDLK_9)return {int32_t(k)};
 if(k>=SDLK_F1&&k<=SDLK_F12)return {int32_t(0x70+k-SDLK_F1)};
 if(k>=SDLK_F13&&k<=SDLK_F24)return {int32_t(0x7c+k-SDLK_F13)};
 switch(k){
 case SDLK_LSHIFT:return {0x10,0xa0};case SDLK_RSHIFT:return {0x10,0xa1};
 case SDLK_LCTRL:return {0x11,0xa2};case SDLK_RCTRL:return {0x11,0xa3};
 case SDLK_LALT:return {0x12,0xa4};case SDLK_RALT:return {0x12,0xa5};
 case SDLK_LGUI:return {0x5b};case SDLK_RGUI:return {0x5c};case SDLK_APPLICATION:return {0x5d};
 case SDLK_BACKSPACE:return {8};case SDLK_TAB:return {9};case SDLK_CLEAR:return {12};
 case SDLK_RETURN:case SDLK_KP_ENTER:return {13};case SDLK_PAUSE:return {19};case SDLK_CAPSLOCK:return {20};case SDLK_ESCAPE:return {27};case SDLK_SPACE:return {32};
 case SDLK_PAGEUP:return {33};case SDLK_PAGEDOWN:return {34};case SDLK_END:return {35};case SDLK_HOME:return {36};case SDLK_LEFT:return {37};case SDLK_UP:return {38};case SDLK_RIGHT:return {39};case SDLK_DOWN:return {40};
 case SDLK_SELECT:return {41};case SDLK_EXECUTE:return {43};case SDLK_PRINTSCREEN:return {44};case SDLK_INSERT:return {45};case SDLK_DELETE:return {46};case SDLK_HELP:return {47};
 case SDLK_KP_0:return {0x60};case SDLK_KP_1:return {0x61};case SDLK_KP_2:return {0x62};case SDLK_KP_3:return {0x63};case SDLK_KP_4:return {0x64};case SDLK_KP_5:return {0x65};case SDLK_KP_6:return {0x66};case SDLK_KP_7:return {0x67};case SDLK_KP_8:return {0x68};case SDLK_KP_9:return {0x69};
 case SDLK_KP_MULTIPLY:return {0x6a};case SDLK_KP_PLUS:return {0x6b};case SDLK_KP_COMMA:return {0x6c};case SDLK_KP_MINUS:return {0x6d};case SDLK_KP_PERIOD:return {0x6e};case SDLK_KP_DIVIDE:return {0x6f};case SDLK_NUMLOCKCLEAR:return {0x90};case SDLK_SCROLLLOCK:return {0x91};
 case SDLK_SEMICOLON:return {0xba};case SDLK_EQUALS:return {0xbb};case SDLK_COMMA:return {0xbc};case SDLK_MINUS:return {0xbd};case SDLK_PERIOD:return {0xbe};case SDLK_SLASH:return {0xbf};case SDLK_BACKQUOTE:return {0xc0};case SDLK_LEFTBRACKET:return {0xdb};case SDLK_BACKSLASH:return {0xdc};case SDLK_RIGHTBRACKET:return {0xdd};case SDLK_QUOTE:return {0xde};
 case SDLK_MUTE:return {0xad};case SDLK_VOLUMEDOWN:return {0xae};case SDLK_VOLUMEUP:return {0xaf};
 default:return {};
 }
}
}

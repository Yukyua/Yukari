#include "yukari/native_text_box.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
using namespace yukari;
void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
int main(){SDL_Window* w=nullptr;try{
 if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
 w=SDL_CreateWindow("Yukari native edit test",0,0,1024,576,SDL_WINDOW_HIDDEN|SDL_WINDOW_ALLOW_HIGHDPI);if(!w)throw std::runtime_error(SDL_GetError());
 {NativeTextBox box(w);
  if(std::strcmp(SDL_GetCurrentVideoDriver(),"cocoa")){bool rejected=false;try{box.create({12,34,210,28,0});}catch(const std::runtime_error&){rejected=true;}check(rejected&&!box.exists(),"non-Cocoa backend must explicitly reject");SDL_DestroyWindow(w);w=nullptr;SDL_Quit();std::cout<<"PASS explicit rejection of unavailable native Cocoa TextBox backend\n";return 0;}
  TextModelHooks hooks;box.bind(hooks);TextModel model(hooks);
  model.call("TextBoxSet",{int32_t(12),int32_t(34),int32_t(210),int32_t(28),int32_t(0x00cc2244)});check(box.exists(),"native control not created");auto r=box.geometry();check(r.x==12&&r.y==34&&r.w==210&&r.h==28,"native spec geometry mismatch");
  model.call("TextBoxStringSet",{std::string("Test \xe3\x81\x82")});check(std::get<std::string>(*model.call("TextBoxStringGet",{}))=="Test \xe3\x81\x82","real native control get/set failed");
  const auto saved=model.snapshot();model.call("TextBoxStringSet",{std::string("changed")});model.restore(saved);check(box.get()=="Test \xe3\x81\x82","native TextBox snapshot did not recreate real UI content");
  model.call("TextBoxSet",{int32_t(1),int32_t(2),int32_t(100),int32_t(20)});check(box.get().empty(),"replacement control retained old text");box.set("");check(box.get().empty(),"empty native text failed");box.sync_geometry();
  model.call("TextBoxDelete",{});check(!box.exists(),"native delete failed");model.call("TextBoxDelete",{});
 }
 SDL_DestroyWindow(w);SDL_Quit();std::cout<<"PASS native Cocoa TextBox lifecycle/hooks/geometry/get/set\n";return 0;
 }catch(const std::exception& e){if(w)SDL_DestroyWindow(w);SDL_Quit();std::cerr<<e.what()<<'\n';return 1;}}

// Catalog probe: call each real delegate API with no arguments, never a real
// game resource. Exceptions establish an explicit route, not semantic support.
#include "yukari/services.hpp"
#include "yukari/settings_services.hpp"
#include "yukari/area_services.hpp"
#include "yukari/platform_services.hpp"
#include "yukari/audio_commands.hpp"
#include "yukari/cd_commands.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace yukari;
int main(int argc,char** argv){
    SDL_Window* window=nullptr;SDL_Renderer* renderer=nullptr;
    try{
        if(argc!=3)throw std::runtime_error("usage: probe_delegates NAMES_FILE TEMPORARY_DIRECTORY");
        if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
        window=SDL_CreateWindow("Yukari audit probe",0,0,64,48,SDL_WINDOW_HIDDEN);
        if(!window)throw std::runtime_error(SDL_GetError());
        renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
        if(!renderer)throw std::runtime_error(SDL_GetError());
        {
            ScalarServices scalar;SettingsServices settings;SettingsFrame frame;AreaServices areas;
            PlatformServices platform(window,renderer,argv[2],argv[2]);
            CdCommands cd;Audio audio(false);AudioCommands::Records records;
            AudioCommands sound(audio,[](const std::string&)->Bytes{throw std::runtime_error("audit never reads media");},records);
            std::ifstream names(argv[1]);if(!names)throw std::runtime_error("cannot read dispatch names");
            std::string name;
            while(std::getline(names,name)){
                auto probe=[&](const char* provider,auto run){
                    try{if(run())std::cout<<"PROBE\t"<<name<<'\t'<<provider<<"\treturned\n";}
                    catch(const std::runtime_error& e){
                        std::string message=e.what();for(char& c:message)if(c=='\n'||c=='\r'||c=='\t')c=' ';
                        std::cout<<"PROBE\t"<<name<<'\t'<<provider<<"\texplicit_error\t"<<message<<'\n';
                    }
                };
                probe("ScalarServices",[&]{return bool(scalar.call(name,{}));});
                probe("SettingsServices",[&]{return bool(settings.call(name,{},&frame));});
                probe("AreaServices",[&]{return bool(areas.call(name,{}));});
                probe("PlatformServices",[&]{return bool(platform.call(name,{}));});
                probe("CdCommands",[&]{HostCall call;call.function=name;return bool(cd.handle(call));});
                probe("AudioCommands",[&]{HostCall call;call.function=name;return bool(sound.handle(call,{}));});
            }
        }
        SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
    }catch(const std::exception& e){if(renderer)SDL_DestroyRenderer(renderer);if(window)SDL_DestroyWindow(window);SDL_Quit();std::cerr<<e.what()<<'\n';return 1;}
}

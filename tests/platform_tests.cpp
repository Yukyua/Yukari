#include "yukari/platform_services.hpp"
#include <iostream>
#include <stdexcept>
#include <unistd.h>

using namespace yukari;
namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<class F> void rejects(F action){bool rejected=false;try{action();}catch(const std::runtime_error&){rejected=true;}check(rejected,"unsupported/unsafe call was accepted");}
void put16(Bytes& b,size_t at,uint16_t n){b[at]=uint8_t(n);b[at+1]=uint8_t(n>>8);}
void put32(Bytes& b,size_t at,uint32_t n){for(int i=0;i<4;++i)b[at+i]=uint8_t(n>>(8*i));}
Bytes cursor(){
    // One 1x1 original-format CUR: bottom-up BGRA XOR plus DWORD-aligned AND mask.
    Bytes b(22+40+4+4);put16(b,2,2);put16(b,4,1);b[6]=b[7]=1;
    put32(b,14,48);put32(b,18,22);put32(b,22,40);put32(b,26,1);put32(b,30,2);
    put16(b,34,1);put16(b,36,32);b[62]=30;b[63]=20;b[64]=10;b[65]=255;return b;
}
}
int main(){
    try{
        SDL_setenv("SDL_VIDEODRIVER","dummy",1);check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
        auto* window=SDL_CreateWindow("platform test",0,0,320,200,SDL_WINDOW_HIDDEN);check(window,SDL_GetError());
        struct Cleanup{SDL_Window* window;~Cleanup(){SDL_DestroyWindow(window);SDL_Quit();}} cleanup{window};
        const auto state=std::filesystem::weakly_canonical(std::filesystem::temp_directory_path())/("yukari-platform-test-"+std::to_string(getpid()));
        std::filesystem::remove_all(state);std::filesystem::create_directories(state);
        struct Files{std::filesystem::path path;~Files(){std::filesystem::remove_all(path);}} files{state};
        int auto_hide=-1;bool region=false,executed=false;bool foreground=false;bool dialog_cancel=true;
        PlatformHooks hooks;hooks.read_resource=[](const std::string& name){if(name=="absent")throw std::runtime_error("absent");return Bytes{1,2,3};};
        hooks.logical_size=[](){return SDL_Point{320,200};};hooks.foreground=[&](){return foreground;};
        hooks.cursor_auto_hide=[&](int32_t value){auto_hide=value;};hooks.auto_region=[&](bool value){region=value;};
        hooks.icon_image=[](uint16_t id){check(id==65535,"icon must narrow resource id to WORD");return PlatformImage{1,1,Bytes{1,2,3,255},{}};};
        hooks.extract_dialog=[&](const std::string&,const std::filesystem::path&)->std::optional<std::filesystem::path>{if(dialog_cancel)return std::nullopt;return state/"chosen.bin";};
        hooks.execute=[&](const std::string& target,const std::filesystem::path& cwd){check(target=="example.txt","Execute target changed");check(cwd==state/"working","Execute working directory changed");executed=true;return true;};
        PlatformServices service(window,nullptr,state,state,hooks);
        const auto integer=[&](const std::string& name,std::vector<Value> args={}){auto result=service.call(name,args);check(result.has_value(),"recognized command not dispatched");return std::get<int32_t>(*result);};
        check(!service.call("Unknown",{}),"unknown command must not be swallowed");
        service.call("CursorHide",{});check(SDL_ShowCursor(SDL_QUERY)==SDL_DISABLE,"cursor hide failed");
        service.call("CursorShow",{});check(SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE,"cursor show failed");
        check(integer("CursorAutoHide",{int32_t(123)})==123&&auto_hide==123,"autohide raw timer policy lost");
        check(integer("CursorAutoHide")==123,"autohide query failed");
        auto* original_cursor=SDL_GetCursor();service.call("CursorLoad",{int32_t(0)});check(SDL_GetCursor()==original_cursor,"absent original default cursor must retain current cursor");
        service.call("CursorLoad",{int32_t(8)}); // Native invalid slot is ignored.
        service.call("MousePointSet",{int32_t(4),int32_t(5)});check(integer("MouseKeyGet",{int32_t(9)})==0,"invalid mouse key must be false");
        service.call("WindowSizeSet",{int32_t(100),int32_t(80)});int w{},h{};SDL_GetWindowSize(window,&w,&h);check(w==100&&h==80,"window client resize failed");
        service.call("WindowSizeSet",{int32_t(321),int32_t(80)});SDL_GetWindowSize(window,&w,&h);check(w==320&&h==200,"native excess-axis resets both maxima");
        check(integer("WindowSizeXGet")==320&&integer("WindowSizeYGet")==200,"logical dimensions lost");
        check(integer("WindowSizeVariableSet",{int32_t(1)})==1&&integer("WindowSizeVariableSet")==1,"resizable query failed");
        service.call("WindowIconChange",{int32_t(-1)});service.call("WindowAutoRegionMode",{int32_t(1)});check(region,"region hook not invoked");
        check(integer("FileExtract")==0,"missing extract args must fail");
        check(integer("FileExtract",{std::string("asset"),std::string("nested/output.bin")})==1,"extract failed");
        check(read_file(state/"nested/output.bin")==Bytes({1,2,3}),"extracted bytes changed");
        check(integer("FileExtract",{std::string("asset"),std::string("nested/output.bin")})==1,"native CREATE_ALWAYS overwrite failed");
        check(integer("FileExtract",{std::string("absent")})==0,"missing resource must return failure");
        rejects([&](){integer("FileExtract",{std::string("asset"),std::string("../escape")});});
        rejects([&](){integer("FileExtract",{std::string("asset"),std::string("C:\\escape")});});
        std::filesystem::create_directory_symlink(state.parent_path(),state/"outside");
        rejects([&](){integer("FileExtract",{std::string("asset"),std::string("outside/escape")});});
        std::filesystem::create_symlink(state/"nested/output.bin",state/"blocked.yukari-extract.tmp");
        check(integer("FileExtract",{std::string("asset"),std::string("blocked")})==0,"temporary symlink must not be followed");
        check(integer("FileExtractDialog",{std::string("asset")})==0,"dialog cancel must not write");dialog_cancel=false;
        check(integer("FileExtractDialog",{std::string("asset")})==1&&read_file(state/"chosen.bin")==Bytes({1,2,3}),"dialog selection not extracted");
        check(integer("Execute",{std::string("example.txt"),std::string("working")})==1&&executed,"Execute provider result lost");
        rejects([&](){integer("Execute",{std::string("example.txt"),std::string("../escape")});});
        PlatformServices missing(window,nullptr,state,state);rejects([&](){missing.call("Execute",{std::string("example")});});
        rejects([&](){missing.call("WindowAutoRegionMode",{int32_t(1)});});rejects([&](){missing.call("CursorAutoHide",{int32_t(5)});});
        check(std::get<int32_t>(*missing.call("CursorAutoHide",{}))==0,"unsupported setter must not mutate state");
        auto data=cursor();auto image=decode_windows_cursor(data);check(image.width==1&&image.height==1&&image.rgba==Bytes({10,20,30,255}),"CUR BGRA decode failed");
        data[66]=0x80;check(decode_windows_cursor(data).rgba[3]==0,"CUR AND transparency lost");
        data[66]=0;data[65]=0;check(decode_windows_cursor(data).rgba[3]==255,"legacy zero alpha CUR must use AND mask");
        put16(data,10,1);rejects([&](){decode_windows_cursor(data);});rejects([&](){decode_windows_cursor(Bytes{});});
        std::cout<<"platform tests passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}

#include "yukari/area_services.hpp"
#include <iostream>
#include <stdexcept>
using namespace yukari;
namespace {
void check(bool ok,const std::string& m){if(!ok)throw std::runtime_error(m);}
Value call(AreaServices& s,const std::string& n,std::vector<Value> a={}){auto r=s.call(n,a);check(bool(r),"missing area service "+n);return *r;}
template<class F>void fails(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("expected area error");}
std::vector<Value> menu(){return {int32_t(13),int32_t(452),int32_t(554),int32_t(503),int32_t(575),int32_t(101),int32_t(452),int32_t(554),int32_t(-1),int32_t(10),int32_t(-1),int32_t(102),int32_t(-1),int32_t(-1)};}
void normalization(){
    AreaServices s;call(s,"AreaSetSkip",menu());auto d=s.definition("AreaSetSkip");check(d&&d->native[0]==1&&d->native[1]==13,"native enable and parent");
    auto sp=s.sprites(*d);check(sp.source==101&&sp.hover_frame==10&&sp.sounds[0]==102,"14-argument sprite and sound mapping");
    check(std::get<int32_t>(call(s,"AreaSetSkipCheck"))==1,"check is enable rather than hover state");
    call(s,"AreaSetSkip");check(s.definition("AreaSetSkip")->native[0]==0&&s.definition("AreaSetSkip")->native[6]==101,"zero args only disable, preserving fields");
    auto extended=menu();extended.push_back(int32_t(7));call(s,"AreaSetSkip",extended);d=s.definition("AreaSetSkip");
    check(d->native[11]==102&&d->native[13]==-1,"native 15-arg branch retains impossible argc14 conditional bug");
    fails([&]{call(s,"AreaSetSkip",{int32_t(0),int32_t(0)});});
    check(std::get<int32_t>(call(s,"AreaSetHistoryPageSize"))==2,"constructor default history page size");
    check(std::get<int32_t>(call(s,"AreaSetHistoryPageSize",{int32_t(5)}))==5,"page size setter query");
    check(!s.call("UnknownArea",{}),"unknown no empty success");
}
void geometry(){
    bool visible=true,eligible=true;AreaHooks hooks;
    hooks.layer=[&](int32_t id)->std::optional<AreaLayer>{if(id!=13)return std::nullopt;return AreaLayer{visible,10.9,-3.7};};hooks.eligible=[&](const std::string&){return eligible;};AreaServices s(hooks);
    call(s,"AreaSetSkip",menu());auto bounds=s.bounds(*s.definition("AreaSetSkip"));check(bounds&&bounds->left==462&&bounds->top==551&&bounds->right==513&&bounds->bottom==572,"truncated parent translation");
    check(s.hit("AreaSetSkip",480,560)&&!s.hit("AreaSetSkip",452,554),"hit uses translated rectangle");
    visible=false;check(!s.hit("AreaSetSkip",480,560),"hidden parent suppresses area");visible=true;eligible=false;check(!s.hit("AreaSetSkip",480,560),"runtime availability policy gates hit");
    AreaServices missing;call(missing,"AreaSetSkip",menu());fails([&]{missing.hit("AreaSetSkip",480,560);});
}
void dynamic(){
    AreaHooks hooks;hooks.layer=[](int32_t id)->std::optional<AreaLayer>{if(id==13)return AreaLayer{true,0,0};return std::nullopt;};AreaServices s(hooks);
    auto values=menu();values.insert(values.begin(),std::string("menu.yks"));values.insert(values.begin(),int32_t(7));
    call(s,"AreaSetMouseLeftScript",values);check(s.script_area(7)&&s.script_area(7)->script=="menu.yks","script copied by id");
    check(s.hit_scripts(480,560).size()==1&&s.hit_scripts(480,560,true).empty(),"left script vs move flags");
    call(s,"AreaSetMouseAreaScript",values);check(s.hit_scripts(480,560,true).size()==1,"mouse-move script mode");
    auto state=s.state();AreaServices restored(hooks);restored.restore(state);check(restored.hit_scripts(480,560).size()==1,"area state restore");
    call(s,"AreaSetMouseLeftScript",{int32_t(7)});check(s.hit_scripts(480,560).empty(),"short existing registration disables it");
    call(s,"AreaSetDelete",{int32_t(7)});check(!s.script_area(7),"dynamic deletion");
    std::vector<Value> indicate={int32_t(20),int32_t(13),int32_t(452),int32_t(554),int32_t(503),int32_t(575),int32_t(102),int32_t(729),int32_t(425),int32_t(-1),int32_t(0),int32_t(-1)};
    call(s,"AreaSetHistoryIndicate",indicate);auto a=s.script_area(20);check(a&&a->history_indication&&a->script.empty()&&a->native[10]==0,"indication wrapper normalization");
    indicate.back()=int32_t(77);call(s,"AreaSetIndicate",indicate);a=s.script_area(20);check(a->native[12]==-1&&a->native[13]==77&&a->native[11]==-1,"inserted placeholder precedes twelfth indication sound argument");
    check(s.hit_scripts(480,560).empty()&&s.hovered_regions(480,560)==std::vector<int32_t>{20},"indication-only regions hover without executing empty script");
    call(s,"KeySetScript",{int32_t(8),std::string("keys.yks"),int32_t(1),int32_t(0),int32_t(112),int32_t(113)});
    check(s.key_scripts(112)==std::vector<int32_t>{8}&&s.key_scripts(113)==std::vector<int32_t>{8},"key alternatives are OR, not chord");
    check(s.key_scripts(112,true).empty()&&s.key_scripts(0).empty(),"repeat and zero VK filtered");
    call(s,"AreaSetKeyInScript",{int32_t(8)});check(s.key_scripts(112)==std::vector<int32_t>{8},"native short key registration retains separate key table");
    call(s,"AreaSetMouseInScript",{int32_t(9),std::string("wheel.yks"),int32_t(4),int32_t(5)});
    check(s.mouse_scripts(4)==std::vector<int32_t>{9}&&s.mouse_scripts(5)==std::vector<int32_t>{9}&&s.mouse_scripts(2).empty(),"mouse input alternatives or wheel mapping");
    auto mouse_state=s.state();AreaServices mouse_restored(hooks);mouse_restored.restore(mouse_state);check(mouse_restored.mouse_scripts(4)==std::vector<int32_t>{9},"mouse input binding state restore");
    call(s,"AreaSetMouseInScript",{int32_t(9)});check(s.mouse_scripts(5)==std::vector<int32_t>{9},"native short mouse registration retains separate input table");
    call(s,"AreaSetDelete",{int32_t(8)});check(s.key_scripts(112).empty(),"key binding deleted by area id");
    call(s,"AreaSetDelete",{int32_t(9)});check(s.mouse_scripts(4).empty(),"mouse binding deleted by area id");
}
}
int main(){try{normalization();geometry();dynamic();std::cout<<"areas: 3 groups passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

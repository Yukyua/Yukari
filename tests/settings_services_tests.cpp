#include "yukari/settings_services.hpp"
#include <iostream>
#include <stdexcept>
using namespace yukari;
namespace {
void check(bool c,const std::string& m){if(!c)throw std::runtime_error(m);}
Value invoke(SettingsServices& s,const std::string& n,std::vector<Value> a={},SettingsFrame* f=nullptr){auto r=s.call(n,a,f);check(bool(r),"missing settings command: "+n);return *r;}
int32_t num(SettingsServices& s,const std::string& n,std::vector<Value> a={}){auto v=invoke(s,n,std::move(a));auto p=std::get_if<int32_t>(&v);check(p,"integer result required: "+n);return *p;}
template<class F>void fails(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("expected settings error");}
void scalar_rules(){
    SettingsServices s;check(num(s,"DefaultKeyinSoundStopGet")==1,"inverse zero native flag");check(num(s,"TransitionModeGet")==-1,"native internal zero transition default");
    invoke(s,"TextSpeedSet",{int32_t(-20)});check(num(s,"TextSpeedGet")==-20,"no invented clamping");
    invoke(s,"TextSpeedSet",{int32_t(1),int32_t(2)});check(num(s,"TextSpeedGet")==-20,"exact argument count");
    check(num(s,"AutoModeCancelLKey",{int32_t(-1)})==1,"boolean normalization");
    check(num(s,"AutoModeCancelLKey",{int32_t(0),int32_t(0)})==1,"cancel setter exact count otherwise query");
    check(num(s,"DefaultKeyinSoundStopSet",{int32_t(0)})==0,"public value despite inverse native storage");
    invoke(s,"SaveEnableMode",{int32_t(1)});invoke(s,"SaveEnableMode",{int32_t(2)});check(s.integer("SaveEnableMode")==1,"only zero and one accepted");
    check(num(s,"SkipScrollSpeed",{int32_t(8)})==8&&num(s,"SkipScrollSpeed",{int32_t(-1)})==8,"positive-only speed with query result");
    for(const auto& p:std::vector<std::pair<std::string,std::string>>{{"AutoSpeedSet","AutoSpeedGet"},{"AutoModeCharactorWaitSet","AutoModeCharactorWaitGet"},{"TransitionSkipSet","TransitionSkipGet"},{"ExitNoMessageSet","ExitNoMessageGet"},{"AlreadyNoWaitModeSet","AlreadyNoWaitModeGet"}}){
        invoke(s,p.first,{int32_t(1)});check(num(s,p.second)==1,"all scalar setter/getter pairs");
        invoke(s,p.first,{int32_t(0)});check(num(s,p.second)==0,"all scalar getter resets");
    }
    for(auto p:{"SkipModeCancelLKey","AlreadySkipModeCancelLKey","HistoryExitToNext","HistoryToVoiceContinue"})check(num(s,p,{int32_t(1)})==1&&num(s,p)==1,"all bool setter/query forms");
    check(num(s,"CursorAutoHide",{int32_t(1234)})==1234&&num(s,"CursorAutoHide")==1234,"timeout setter/query");
    invoke(s,"KeyWaitIconNumber",{int32_t(8)});check(s.integer("KeyWaitIconNumber")==8,"icon number state");
    for(auto p:std::vector<std::pair<int,int>>{{-100,-2},{-1,-1},{0,0},{99,1}}){invoke(s,"TransitionModeSet",{int32_t(p.first)});check(num(s,"TransitionModeGet")==p.second,"transition mapping");}
    check(s.integer("TransitionStep")==16&&s.integer("TransitionDrawMode")==0,"transition renderer state side effects");
    check(!s.call("UnreviewedSetting",{}),"unknown must not silently succeed");
    fails([&]{s.query("UnestablishedDefault");});
    fails([&]{invoke(s,"TextSpeedSet",{std::string("bad")});});
}
void modes_frames(){
    int changes=0;SettingsHooks h;h.changed=[&](const std::string&,const SettingsState&){++changes;};SettingsServices s(h);
    invoke(s,"AutoModeSet",{int32_t(0)});invoke(s,"SkipModeSet");invoke(s,"AlreadySkipModeSet");
    check(num(s,"AutoModeCheck")==1&&num(s,"SkipModeCheck")==1&&num(s,"AlreadySkipModeCheck")==1,"native setters force true, independent bits");
    invoke(s,"AutoModeSet");check(num(s,"AutoModeCheck")==1,"set is not toggle");
    invoke(s,"SkipModeReset");check(num(s,"SkipModeCheck")==0&&num(s,"AlreadySkipModeCheck")==1,"individual reset");
    SettingsFrame a{},b{};invoke(s,"ScriptSkipMode",{int32_t(2)},&a);invoke(s,"SubMenuEnableMode",{int32_t(1)},&a);
    check(a.script_skip_mode==2&&a.submenu_enabled&&b.script_skip_mode==0&&!b.submenu_enabled,"frame isolation");
    SettingsFrame child=a;invoke(s,"ScriptSkipMode",{int32_t(1)},&child);check(child.submenu_enabled&&a.script_skip_mode==2,"native frame-push copies parent settings then isolates child writes");
    invoke(s,"ScriptSkipMode",{int32_t(0)},&a);check(num(s,"AutoModeCheck")==1&&num(s,"AlreadySkipModeCheck")==0,"script mode zero keeps auto mode");
    invoke(s,"AllSkipModeReset");check(num(s,"AutoModeCheck")==0,"all reset alias");
    check(num(s,"ModeExclusive",{int32_t(2)})==0&&s.integer("ModeExclusive")==1,"ModeExclusive return differs from stored setting");
    fails([&]{invoke(s,"ScriptSkipMode",{int32_t(1)});});
    check(changes>8,"real change consumer hooks notified");
}
void strings_callbacks(){
    SettingsServices s;invoke(s,"Index",{std::string("chapter")});check(std::get<std::string>(s.query("Index"))=="chapter","index copy");
    invoke(s,"Index");check(std::get<std::string>(s.query("Index")).empty(),"index no args clears");
    invoke(s,"SubMenuScript",{std::string("menu.yks"),int32_t(258)});check(s.integer("SubMenuScriptParameter")==2,"submenu byte parameter");
    invoke(s,"SubMenuScript");check(std::get<std::string>(s.query("SubMenuScript")).empty(),"submenu reset");
    invoke(s,"SaveDataLoadToCallScript",{std::string("load.yks")});invoke(s,"SaveDataLoadToCallScript");check(std::get<std::string>(s.query("SaveDataLoadToCallScript"))=="load.yks","no args preserves load callback");
    check(num(s,"SelectFunctionGet")==127&&num(s,"MouseSelectFunctionGet")==0&&num(s,"KeyWaitFunctionGet")==2047,"original context constructor defaults");
    invoke(s,"SelectFunctionSet",{int32_t(8)});check(num(s,"SelectFunctionGet")==8,"function callback query");
    check(num(s,"SelectFunctionGet",{int32_t(9)})==9,"native getter can also set");
    for(const auto& p:std::vector<std::pair<std::string,std::string>>{{"MouseSelectFunctionSet","MouseSelectFunctionGet"},{"KeyWaitFunctionSet","KeyWaitFunctionGet"}}){invoke(s,p.first,{int32_t(123)});check(num(s,p.second)==123&&num(s,p.second,{int32_t(456)})==456,"all callback getter/setter contracts");}
    auto state=s.state();SettingsServices copy;copy.restore(state);check(num(copy,"SelectFunctionGet")==9,"restorable settings and index");
}
void records(){
    SettingsServices s;std::vector<Value> keys;for(int i=0;i<20;++i)keys.emplace_back(int32_t(i));
    invoke(s,"SystemKeySetDecide",keys);check(s.record("SystemKeySetDecide").size()==15,"native clamp16 and helper removes zero keys");
    check(s.key_matches("SystemKeySetDecide",5)&&!s.key_matches("SystemKeySetDecide",16)&&!s.key_matches("SystemKeySetDecide",5,true),"native OR key match with repeat filtering");
    invoke(s,"SystemKeySetDecide");check(s.record("SystemKeySetDecide").size()==15,"zero argc leaves key table intact");
    invoke(s,"DefaultTextWindowGroup",{int32_t(4),Value{},int32_t(5)});check(s.record("DefaultTextWindowGroup").size()==2,"group omits null pointers");
    invoke(s,"SelectTransitionSet",{int32_t(3),int32_t(4)});invoke(s,"SelectTransitionSet");check(s.integer("SelectTransition")==0&&s.integer("SelectTransitionParameter")==-1,"transition clear defaults");
    invoke(s,"HistoryTextColor",{int32_t(123),int32_t(456)});check(s.integer("HistoryTextColorEnabled")==1&&s.integer("HistoryVoicedTextColor")==456,"history colors");
    auto legacy=s.state();legacy.values.erase("HistoryVoicedTextColor");legacy.values["HistoryTextBorderColor"]=int32_t(999);s.restore(legacy);check(s.integer("HistoryVoicedTextColor")==999,"legacy second history color migrates as voiced-page color");
    invoke(s,"HistoryWindowSet",{int32_t(3),int32_t(1),int32_t(2),int32_t(10),int32_t(20)});check(s.record("HistoryWindowRect").size()==4,"history rect configured");
    invoke(s,"HistoryWindowSet");check(s.integer("HistoryWindowEnabled")==0,"history window disable");
    invoke(s,"HistorySoundGroupSet",{int32_t(258)});check(std::get<int32_t>(s.record("HistorySoundGroupSet")[0])==2,"native byte group");
    invoke(s,"SaveDataDisplaySetToGraphic",{int32_t(10),int32_t(20),int32_t(126),int32_t(70)});check(s.integer("SaveDataDisplaySetToGraphicW")==126,"thumbnail layout");
    invoke(s,"SaveDataDisplaySetToGraphic");check(s.integer("SaveDataDisplaySetToGraphicEnabled")==0,"thumbnail disable mask");
    invoke(s,"SaveDataDisplaySetToComment2",{int32_t(1),int32_t(2),int32_t(3),int32_t(4)});invoke(s,"SaveDataDisplaySetToComment2",{int32_t(5),int32_t(6)});
    check(std::get<int32_t>(s.record("SaveDataDisplaySetToComment2Rect")[0])==0,"short comment2 call empties rectangle");
    invoke(s,"SaveDataDisplaySetToDate",{int32_t(1),int32_t(2),std::string("%d"),int32_t(8)});check(s.record("SaveDataDisplaySetToDateFields").size()==16,"date fields native fixed array");
}
void preview_fonts(){
    SettingsServices s;
    check(s.record("SaveDataDisplaySetToFont")==std::vector<Value>({int32_t(9),int32_t(2),int32_t(0x303030)}),"native base preview font initial DWORDs");
    for(const auto* name:{"SaveDataDisplaySetToComment1Font","SaveDataDisplaySetToComment2Font"}){
        check(s.record(name)==std::vector<Value>({int32_t(-1),int32_t(-1),int32_t(-1)}),"comment font defaults inherit every field");
        invoke(s,name);check(s.record(name)==std::vector<Value>({int32_t(-1),int32_t(-1),int32_t(-1)}),"zero-argument font setter preserves defaults");
        invoke(s,name,{int32_t(11)});check(s.record(name)==std::vector<Value>({int32_t(11),int32_t(-1),int32_t(-1)}),"partial comment font setter keeps effect/color inheritance");
        invoke(s,name,{int32_t(12),int32_t(3)});check(s.record(name)==std::vector<Value>({int32_t(12),int32_t(3),int32_t(-1)}),"two-argument comment setter preserves color inheritance");
        invoke(s,name,{int32_t(-1),int32_t(-1),int32_t(0x112233),std::string("ignored")});check(s.record(name)==std::vector<Value>({int32_t(-1),int32_t(-1),int32_t(0x112233)}),"native font setter reads only first three arguments");
    }
    invoke(s,"SaveDataDisplaySetToFont",{int32_t(20)});check(s.record("SaveDataDisplaySetToFont")==std::vector<Value>({int32_t(20),int32_t(2),int32_t(0x303030)}),"base partial setter preserves original effect/color");
    invoke(s,"SaveDataDisplaySetToFont");check(std::get<int32_t>(s.record("SaveDataDisplaySetToFont")[0])==20,"base zero arguments retain prior value");
    auto old=s.state();old.records.erase("SaveDataDisplaySetToFont");old.records["SaveDataDisplaySetToComment1Font"]={int32_t(17)};old.records["SaveDataDisplaySetToComment2Font"]={int32_t(18),Value{},Value{}};s.restore(std::move(old));
    check(s.record("SaveDataDisplaySetToFont")==std::vector<Value>({int32_t(9),int32_t(2),int32_t(0x303030)}),"old snapshots without preview record establish native defaults");
    check(s.record("SaveDataDisplaySetToComment1Font")==std::vector<Value>({int32_t(17),int32_t(-1),int32_t(-1)}),"old short preview record gains inherited tail");
    check(s.record("SaveDataDisplaySetToComment2Font")==std::vector<Value>({int32_t(18),int32_t(-1),int32_t(-1)}),"legacy null placeholder tails migrate to inheritance");
}
void fullscreen(){
    bool fullscreen=false;int calls=0;SettingsHooks h;h.fullscreen_get=[&]{return fullscreen;};h.fullscreen_set=[&](bool v){fullscreen=v;++calls;};SettingsServices s(h);
    invoke(s,"FullScreenModeSet",{int32_t(1)});check(num(s,"FullScreenModeGet")==1&&calls==1,"platform applied fullscreen");
    invoke(s,"FullScreenMode",{int32_t(2)});check(calls==1,"idempotent nonzero fullscreen");
    invoke(s,"FullScreenModeSet",{int32_t(0)});check(!fullscreen&&calls==2,"platform windowed");
    SettingsServices missing;fails([&]{invoke(missing,"FullScreenModeGet");});
}
}
int main(){try{scalar_rules();modes_frames();strings_callbacks();records();preview_fonts();fullscreen();std::cout<<"settings: 6 groups passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

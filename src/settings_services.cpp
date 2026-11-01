#include "yukari/settings_services.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>
namespace yukari {
namespace {
int32_t number(const Value& v){auto p=std::get_if<int32_t>(&v);if(!p)throw std::runtime_error("setting requires integer");return *p;}
std::string string(const Value& v){auto p=std::get_if<std::string>(&v);if(!p)throw std::runtime_error("setting requires string");return p->substr(0,p->find('\0'));}
struct Spec {const char *set,*get,*key;int count;bool boolean,returns;};
const Spec specs[]={
    {"TextSpeedSet","TextSpeedGet","TextSpeed",1,false,false},
    {"AutoSpeedSet","AutoSpeedGet","AutoSpeed",1,false,false},
    {"AutoModeCharactorWaitSet","AutoModeCharactorWaitGet","AutoModeCharactorWait",1,false,false},
    {"TransitionSkipSet","TransitionSkipGet","TransitionSkip",1,true,false},
    {"ExitNoMessageSet","ExitNoMessageGet","ExitNoMessage",-1,true,false},
    {"DefaultKeyinSoundStopSet","DefaultKeyinSoundStopGet","DefaultKeyinSoundStop",-1,true,true},
    {"AlreadyNoWaitModeSet","AlreadyNoWaitModeGet","AlreadyNoWaitMode",-1,true,true},
    {"SkipModeCancelLKey","","SkipModeCancelLKey",1,true,true},
    {"AutoModeCancelLKey","","AutoModeCancelLKey",1,true,true},
    {"AlreadySkipModeCancelLKey","","AlreadySkipModeCancelLKey",1,true,true},
    {"HistoryExitToNext","","HistoryExitToNext",-1,true,true},
    {"HistoryToVoiceContinue","","HistoryToVoiceContinue",-1,true,true},
    {"CursorAutoHide","","CursorAutoHide",-1,false,true},
    {"KeyWaitIconNumber","","KeyWaitIconNumber",1,false,false},
    {"SelectFunctionSet","SelectFunctionGet","SelectFunction",-1,false,false},
    {"MouseSelectFunctionSet","MouseSelectFunctionGet","MouseSelectFunction",-1,false,false},
    {"KeyWaitFunctionSet","KeyWaitFunctionGet","KeyWaitFunction",-1,false,false},
};
std::vector<Value> preview_font_defaults(const std::string& name){
    // Original PE initial DWORDs: 4bc34c/350/354 and comment triplets
    // 4bc374/3c8/3cc, 4bc3d0/3d4/3d8. -1 means inherit base font.
    if(name=="SaveDataDisplaySetToFont")return {int32_t(9),int32_t(2),int32_t(0x303030)};
    return {int32_t(-1),int32_t(-1),int32_t(-1)};
}
void seed_preview_font_record(SettingsState& state,const std::string& name){
    auto defaults=preview_font_defaults(name);auto& record=state.records[name];
    const auto supplied=record.size();if(supplied<3)record.resize(3);
    for(size_t i=0;i<3;++i)if(i>=supplied||std::holds_alternative<std::monostate>(record[i]))record[i]=defaults[i];
}
bool system_key(const std::string& n){for(auto p:{"SystemKeySetDecide","SystemKeySetSelectPrev","SystemKeySetSelectNext","SystemKeySetWindowErase","SystemKeySetAutoMode","SystemKeySetHistoryMode","SystemKeySetKeyInSkip","SystemKeySetSkip"})if(n==p)return true;return false;}
}
SettingsServices::SettingsServices(SettingsHooks hooks):hooks_(std::move(hooks)) {
    // Original PE image global cells are zero before startup/config loaders.
    for(auto key:{"TextSpeed","AutoSpeed","AutoModeCharactorWait","TransitionSkip","ExitNoMessage",
        "SkipModeCancelLKey","AutoModeCancelLKey","AlreadySkipModeCancelLKey","HistoryExitToNext","HistoryToVoiceContinue",
        "CursorAutoHide","SkipScrollSpeed","AutoMode","SkipMode","AlreadySkipMode","SkipNoDraw","ModeExclusive","SaveEnableMode","LoadEnableMode"})state_.values[key]=int32_t(0);
    state_.values["DefaultKeyinSoundStop"]=int32_t(1); // public getter inverts zero native cell
    state_.values["TransitionMode"]=int32_t(-1); // zero internal cell maps to public -1
    // Context constructor 0x428fe0, stores at 0x429125/12f/135/14f.
    state_.values["SelectFunction"]=int32_t(0x7f);
    state_.values["MouseSelectFunction"]=int32_t(0);
    state_.values["KeyWaitFunction"]=int32_t(0x7ff);
    state_.values["KeyWaitIconNumber"]=int32_t(0);
    // Constructor's final 0x447780 initialization overrides initial zeros.
    state_.values["SelectTransition"]=int32_t(2);
    state_.values["SelectTransitionParameter"]=int32_t(-1);
    state_.values["KeyWaitIcon"]=int32_t(0);
    state_.values["ScriptSkipActive"]=int32_t(0);
    state_.values["HistoryTextColorEnabled"]=int32_t(0);
    state_.values["HistoryTextColor"]=int32_t(0);
    state_.values["HistoryVoicedTextColor"]=int32_t(0);
    state_.values["HistoryWindowEnabled"]=int32_t(0);
    state_.values["HistoryWindow"]=int32_t(0);
    state_.values["Index"]=std::string{};
    state_.values["SubMenuScript"]=std::string{};
    state_.values["SubMenuScriptParameter"]=int32_t(0);
    state_.records["HistorySoundGroupSet"]={};
    for(const auto* name:{"SaveDataDisplaySetToFont","SaveDataDisplaySetToComment1Font","SaveDataDisplaySetToComment2Font"})seed_preview_font_record(state_,name);
    // 0x428fe0 leaves context byte +0x80112 uninitialized. Give the port a
    // deterministic disabled value instead of reproducing allocator garbage;
    // this is documented as a portability choice, not an observed native value.
    state_.values["AlreadyNoWaitMode"]=int32_t(0);
}
const Value& SettingsServices::query(const std::string& key)const{auto it=state_.values.find(key);if(it==state_.values.end())throw std::runtime_error("setting has no established initial value: "+key);return it->second;}
int32_t SettingsServices::integer(const std::string& key)const{return number(query(key));}
const std::vector<Value>& SettingsServices::record(const std::string& key)const{auto it=state_.records.find(key);if(it==state_.records.end())throw std::runtime_error("setting record is not initialized: "+key);return it->second;}
bool SettingsServices::key_matches(const std::string& binding,int32_t vk,bool repeated)const{
    if(!system_key(binding))throw std::runtime_error("unknown system-key binding: "+binding);
    if(repeated||!vk)return false;auto p=state_.records.find(binding);if(p==state_.records.end())return false;
    return std::any_of(p->second.begin(),p->second.end(),[&](const auto& key){return number(key)==vk;});
}
void SettingsServices::seed(const std::string& key,Value value){state_.values[key]=std::move(value);}
void SettingsServices::restore(SettingsState state){if(!state.values.count("HistoryVoicedTextColor")&&state.values.count("HistoryTextBorderColor"))state.values["HistoryVoicedTextColor"]=state.values.at("HistoryTextBorderColor");for(const auto* name:{"SaveDataDisplaySetToFont","SaveDataDisplaySetToComment1Font","SaveDataDisplaySetToComment2Font"})seed_preview_font_record(state,name);state_=std::move(state);changed("Restore");}
void SettingsServices::changed(const std::string& name){if(hooks_.changed)hooks_.changed(name,state_);}
std::optional<Value> SettingsServices::call(const std::string& command,const std::vector<Value>& args,SettingsFrame* frame){
    std::string n=command;if(n=="TransitionSkip")n="TransitionSkipSet";
    for(const auto& spec:specs){
        bool get=n==spec.get&&*spec.get,set=n==spec.set;
        if(!get&&!set)continue;
        // Original three FunctionGet handlers also accept a first setter argument.
        bool get_sets=get&&(n=="SelectFunctionGet"||n=="MouseSelectFunctionGet"||n=="KeyWaitFunctionGet");
        if((set||get_sets)&&((spec.count==1&&args.size()==1)||(spec.count<0&&!args.empty()))){
            int32_t value=number(args[0]);state_.values[spec.key]=spec.boolean?int32_t(value!=0):value;changed(command);
        }
        if(get||spec.returns)return query(spec.key);
        return Value{};
    }
    if(n=="SaveEnableMode"||n=="LoadEnableMode"||n=="SubMenuEnableMode"||n=="KeyWaitIcon"){
        if((n=="KeyWaitIcon"?!args.empty():args.size()==1)){
            int32_t v=number(args[0]);if(v==0||v==1){
                if(n=="SubMenuEnableMode"){if(!frame)throw std::runtime_error("SubMenuEnableMode requires current script frame");frame->submenu_enabled=v!=0;}
                else state_.values[n]=v;
                changed(n);
            }
        }return Value{};
    }
    if(n=="AutoModeCheck"||n=="SkipModeCheck"||n=="AlreadySkipModeCheck"||n=="SkipNoDrawCheck")return Value{int32_t(integer(n.substr(0,n.size()-5))==1)};
    if(n=="AutoModeSet"||n=="SkipModeSet"||n=="AlreadySkipModeSet"||n=="SkipNoDrawSet"){
        state_.values[n.substr(0,n.size()-3)]=int32_t(1);changed(n);return Value{};
    }
    if(n=="AutoModeReset"||n=="SkipModeReset"||n=="AlreadySkipModeReset"||n=="SkipNoDrawReset"){
        state_.values[n.substr(0,n.size()-5)]=int32_t(0);changed(n);return Value{};
    }
    if(n=="AllSkipModeReset"||n=="ScriptSkipReset"){
        for(auto p:{"AutoMode","SkipMode","AlreadySkipMode"})state_.values[p]=int32_t(0);
        state_.values["ScriptSkipActive"]=int32_t(0);changed(n);return Value{};
    }
    if(n=="ScriptSkipMode"){
        if(args.size()==1){int32_t v=number(args[0]);if(v>=0&&v<=2){
            if(!frame)throw std::runtime_error("ScriptSkipMode requires current script frame");frame->script_skip_mode=v;
            if(!v){state_.values["SkipMode"]=int32_t(0);state_.values["AlreadySkipMode"]=int32_t(0);state_.values["ScriptSkipActive"]=int32_t(0);}
            changed(n);
        }}return Value{};
    }
    if(n=="ModeExclusive"){
        if(args.size()==1){state_.values[n]=int32_t(number(args[0])!=0);changed(n);}return Value{int32_t(0)};
    }
    if(n=="SkipScrollSpeed"){
        if(!args.empty()&&number(args[0])>0){state_.values[n]=number(args[0]);changed(n);}return query(n);
    }
    if(n=="TransitionMode"||n=="TransitionModeSet"){
        if(args.size()==1){int32_t v=number(args[0]);state_.values["TransitionMode"]=v<-1?-2:v==-1?-1:v==0?0:1;}
        state_.values["TransitionStep"]=int32_t(16);int32_t mode=integer("TransitionMode");
        state_.values["TransitionDrawMode"]=int32_t(mode==0?1:mode==1?0:2);changed(n);return Value{};
    }
    if(n=="TransitionModeGet")return query("TransitionMode");
    if(n=="FullScreenMode"||n=="FullScreenModeSet"||n=="FullScreenModeGet"){
        if(!hooks_.fullscreen_get)throw std::runtime_error("fullscreen requires actual platform query");
        bool current=hooks_.fullscreen_get();
        if(n!="FullScreenModeGet"&&!args.empty()&&(number(args[0])!=0)!=current){
            if(!hooks_.fullscreen_set)throw std::runtime_error("fullscreen requires actual platform setter");
            hooks_.fullscreen_set(number(args[0])!=0);changed(n);
        }
        return n=="FullScreenModeGet"?Value{int32_t(current)}:Value{};
    }
    if(n=="Index"||n=="SubMenuScript"||n=="SaveDataLoadToCallScript"){
        if(n=="Index")state_.values[n]=args.empty()?std::string{}:string(args[0]);
        else if(n=="SubMenuScript"){
            state_.values[n]=args.empty()?std::string{}:string(args[0]);
            if(!args.empty())state_.values["SubMenuScriptParameter"]=args.size()>1?int32_t(uint32_t(number(args[1]))&255):int32_t(0);
        }else if(!args.empty())state_.values[n]=string(args[0]);
        changed(n);return Value{};
    }
    if(n=="SelectTransitionSet"){
        if(args.size()<=2){state_.values["SelectTransition"]=args.empty()?int32_t(0):number(args[0]);state_.values["SelectTransitionParameter"]=args.size()==2?number(args[1]):int32_t(-1);changed(n);}return Value{};
    }
    if(n=="DefaultTextWindowGroup"){
        std::vector<Value> values;for(auto& v:args)if(!std::holds_alternative<std::monostate>(v))values.emplace_back(number(v));
        state_.records[n]=std::move(values);changed(n);return Value{};
    }
    if(system_key(n)){
        if(!args.empty()){std::vector<Value> values;for(size_t i=0;i<std::min<size_t>(args.size(),16);++i){auto key=number(args[i]);if(key)values.emplace_back(key);}state_.records[n]=std::move(values);changed(n);}return Value{};
    }
    if(n=="KeyWaitIconPositionSet"){
        if(args.size()==4){for(auto& v:args)number(v);state_.records[n+":"+std::to_string(number(args[0]))]=args;changed(n);}return Value{};
    }
    if(n=="IconSetAutoMode"||n=="IconSetAlreadySkip"||n=="IconSetSkip"||n=="IconSetAlready"){
        if(args.size()>4){std::vector<Value> r;for(size_t i=0;i<5;++i)r.emplace_back(number(args[i]));state_.records[n]=std::move(r);changed(n);}return Value{};
    }
    if(n=="HistoryTextColor"){
        if(args.size()==1||args.size()==2){state_.values["HistoryTextColorEnabled"]=int32_t(1);state_.values["HistoryTextColor"]=number(args[0]);if(args.size()==2)state_.values["HistoryVoicedTextColor"]=number(args[1]);changed(n);}return Value{};
    }
    if(n=="HistoryWindowSet"){
        state_.values["HistoryWindowEnabled"]=int32_t(!args.empty());
        if(!args.empty())state_.values["HistoryWindow"]=number(args[0]);
        if(args.size()>4){std::vector<Value> r;for(size_t i=1;i<5;++i)r.emplace_back(number(args[i]));state_.records["HistoryWindowRect"]=std::move(r);}
        changed(n);return Value{};
    }
    if(n=="HistorySoundNumberSet"){
        state_.values["HistorySoundNumberEnabled"]=int32_t(!args.empty());if(!args.empty())state_.values["HistorySoundNumber"]=number(args[0]);changed(n);return Value{};
    }
    if(n=="HistorySoundIconSet"){
        if(args.size()<3)throw std::runtime_error("HistorySoundIconSet requires three native integer arguments");
        std::vector<Value> r;for(size_t i=0;i<3;++i)r.emplace_back(number(args[i]));state_.records[n]=std::move(r);changed(n);return Value{};
    }
    if(n=="HistorySoundGroupSet"){
        if(!args.empty()){
            if(args.size()>32)throw std::runtime_error("HistorySoundGroupSet exceeds native 32-entry array");
            std::vector<Value> r;for(auto& a:args)r.emplace_back(int32_t(uint32_t(number(a))&255));state_.records[n]=std::move(r);
        }changed(n);return Value{};
    }
    if(n=="SaveDataDisplaySetToFont"||n=="SaveDataDisplaySetToComment1Font"||n=="SaveDataDisplaySetToComment2Font"){
        seed_preview_font_record(state_,n);auto& r=state_.records[n];for(size_t i=0;i<std::min<size_t>(3,args.size());++i)r[i]=number(args[i]);changed(n);return Value{};
    }
    if(n=="SaveDataDisplaySetToGraphic"||n=="SaveDataDisplaySetToDate"||n=="SaveDataDisplaySetToComment1"||n=="SaveDataDisplaySetToComment2"){
        state_.values[n+"Enabled"]=int32_t(args.size()>1);
        if(args.size()>1){state_.values[n+"X"]=number(args[0]);state_.values[n+"Y"]=number(args[1]);}
        if(n=="SaveDataDisplaySetToGraphic"&&args.size()>3){state_.values[n+"W"]=number(args[2]);state_.values[n+"H"]=number(args[3]);}
        if(n=="SaveDataDisplaySetToDate"){
            if(args.size()>2)state_.values[n+"Format"]=string(args[2]);
            if(args.size()>3){if(args.size()>19)throw std::runtime_error("date format exceeds native 16-entry array");std::vector<Value> v;for(size_t i=3;i<args.size();++i)v.emplace_back(number(args[i]));v.resize(16,int32_t(0));state_.records[n+"Fields"]=std::move(v);}
        }
        if(n=="SaveDataDisplaySetToComment2"){
            std::vector<Value> rect(4,int32_t(0));if(args.size()>3)for(size_t i=0;i<4;++i)rect[i]=number(args[i]);state_.records[n+"Rect"]=std::move(rect);
        }changed(n);return Value{};
    }
    return std::nullopt;
}
}

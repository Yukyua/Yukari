#include "yukari/area_services.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace yukari {
namespace {
int32_t integer(const Value& v){auto p=std::get_if<int32_t>(&v);if(!p)throw std::runtime_error("area requires integer argument");return *p;}
std::string text(const Value& v){auto p=std::get_if<std::string>(&v);if(!p)throw std::runtime_error("area requires script string");return p->substr(0,p->find('\0'));}
int32_t bits(uint32_t v){int32_t r;std::memcpy(&r,&v,4);return r;}
int32_t add(int32_t a,int32_t b){return bits(uint32_t(a)+uint32_t(b));}
int32_t subtract(int32_t a,int32_t b){return bits(uint32_t(a)-uint32_t(b));}
int32_t native_integer(double n){n=std::trunc(n);if(!std::isfinite(n)||n<INT32_MIN||n>INT32_MAX)return INT32_MIN;return int32_t(n);}
bool builtin(const std::string& n){for(auto p:{"AreaSetHistoryMode","AreaSetSkip","AreaSetAlreadySkip","AreaSetAutoMode","AreaSetAutoModeExecute","AreaSetWindowErase","AreaSetHistoryNext","AreaSetHistoryPageNext","AreaSetHistoryPrev","AreaSetHistoryPagePrev","AreaSetHistoryLast","AreaSetHistoryHead","AreaSetHistoryExit","AreaSetHistoryMouseR","AreaSetHistoryScrollBarH","AreaSetHistoryScrollBarV"})if(n==p)return true;return false;}
void normalize(AreaDefinition& out,const std::vector<Value>& a){
    if(a.empty()){out.native[0]=0;return;}
    const size_t count=a.size();
    if(count!=5&&count!=11&&count!=12&&count!=14&&count!=15&&count!=16)
        throw std::runtime_error("unsupported original area argument count: "+std::to_string(count));
    auto values=out.native;values[0]=0;std::fill(values.begin()+1,values.begin()+16,-1);values[16]=0;
    for(size_t i=0;i<5;++i)values[i+1]=integer(a[i]);values[0]=1;
    if(count==11||count==14){
        for(size_t i=5;i<10;++i)values[i+1]=integer(a[i]);values[12]=integer(a[10]);
        if(count==14)for(size_t i=11;i<14;++i)values[i+2]=integer(a[i]);
    }else if(count>=12){
        for(size_t i=5;i<10;++i)values[i+1]=integer(a[i]);values[12]=integer(a[10]);values[11]=integer(a[11]);
        // Original switch branch tests argc==14 here, impossible for its
        // 12/15/16 labels. Retain -1 sound fields rather than "fixing" it.
    }
    out.native=values;
}
bool contains(const AreaBounds& r,int32_t x,int32_t y){return x>=r.left&&x<=r.right&&y>=r.top&&y<=r.bottom;}
}
AreaServices::AreaServices(AreaHooks hooks):hooks_(std::move(hooks)){}
void AreaServices::changed(const std::string& name){if(hooks_.changed)hooks_.changed(name,state_);}
void AreaServices::restore(AreaState state){state_=std::move(state);changed("Restore");}
const AreaDefinition* AreaServices::definition(const std::string& name)const{auto p=state_.builtins.find(name);return p==state_.builtins.end()?nullptr:&p->second;}
const AreaDefinition* AreaServices::script_area(int32_t id)const{auto p=std::find_if(state_.scripts.begin(),state_.scripts.end(),[&](const auto& a){return a.id==id;});return p==state_.scripts.end()?nullptr:&*p;}
AreaSprites AreaServices::sprites(const AreaDefinition& area)const{const auto& n=area.native;return {n[1],n[6],n[7],n[8],n[9],n[10],n[12],{n[13],n[14],n[15]}};}
std::optional<AreaBounds> AreaServices::bounds(const AreaDefinition& area)const{
    if(!area.native[0])return std::nullopt;
    if(!hooks_.layer)throw std::runtime_error("area hit requires actual parent-layer query provider");
    auto layer=hooks_.layer(area.native[1]);if(!layer||!layer->visible)return std::nullopt;
    auto x=native_integer(layer->x),y=native_integer(layer->y);
    return AreaBounds{add(area.native[2],x),add(area.native[3],y),add(area.native[4],x),add(area.native[5],y)};
}
bool AreaServices::hit(const std::string& name,int32_t x,int32_t y)const{
    auto area=definition(name);if(!area||(hooks_.eligible&&!hooks_.eligible(name)))return false;
    auto b=bounds(*area);return b&&contains(*b,x,y);
}
std::vector<int32_t> AreaServices::hit_scripts(int32_t x,int32_t y,bool mouse_move)const{
    std::vector<int32_t> result;
    for(const auto& a:state_.scripts){
        if(a.script.empty()||(uint32_t(a.native[16])&0x80000000))continue;
        if(mouse_move&&!(uint32_t(a.native[16])&1))continue;
        if(hooks_.eligible&&!hooks_.eligible("AreaScript:"+std::to_string(a.id)))continue;
        auto b=bounds(a);if(b&&contains(*b,x,y))result.push_back(a.id);
    }return result;
}
std::vector<int32_t> AreaServices::hovered_regions(int32_t x,int32_t y)const{
    std::vector<int32_t> ids;
    for(const auto& a:state_.scripts){
        if(uint32_t(a.native[16])&0x80000000)continue;
        if(hooks_.eligible&&!hooks_.eligible("AreaScript:"+std::to_string(a.id)))continue;
        auto b=bounds(a);if(b&&contains(*b,x,y))ids.push_back(a.id);
    }return ids;
}
std::vector<int32_t> AreaServices::key_scripts(int32_t vk,bool repeated)const{
    std::vector<int32_t> ids;if(repeated||!vk)return ids;
    for(const auto& a:state_.scripts){
        if(uint32_t(a.native[16])&0x80000000)continue;
        if(hooks_.eligible&&!hooks_.eligible("AreaScript:"+std::to_string(a.id)))continue;
        if(std::find(a.keys.begin(),a.keys.end(),vk)!=a.keys.end())ids.push_back(a.id);
    }return ids;
}
std::vector<int32_t> AreaServices::mouse_scripts(int32_t input)const{
    std::vector<int32_t> ids;if(input<1||input>5)return ids;
    for(const auto& a:state_.scripts){
        if(uint32_t(a.native[16])&0x80000000)continue;
        if(hooks_.eligible&&!hooks_.eligible("AreaScript:"+std::to_string(a.id)))continue;
        if(std::find(a.mouse_inputs.begin(),a.mouse_inputs.end(),input)!=a.mouse_inputs.end())ids.push_back(a.id);
    }return ids;
}
std::optional<Value> AreaServices::call(const std::string& name,const std::vector<Value>& args){
    if(builtin(name)){
        auto& area=state_.builtins[name];normalize(area,args);
        if(name=="AreaSetHistoryScrollBarH"){area.native[3]=subtract(area.native[3],area.native[8]);area.native[5]=add(area.native[5],area.native[8]);}
        if(name=="AreaSetHistoryScrollBarV"){area.native[2]=subtract(area.native[2],area.native[8]);area.native[4]=add(area.native[4],area.native[8]);}
        changed(name);return Value{};
    }
    if(name=="AreaSetSkipCheck"||name=="AreaSetAlreadySkipCheck"){
        auto p=definition(name=="AreaSetSkipCheck"?"AreaSetSkip":"AreaSetAlreadySkip");return Value{int32_t(p&&p->native[0]!=0)};
    }
    if(name=="AreaSetHistoryPageSize"){
        if(!args.empty()){state_.history_page_size=integer(args[0]);changed(name);}return Value{state_.history_page_size};
    }
    if(name=="AreaSetDelete"){
        if(!args.empty()){auto id=integer(args[0]);auto& v=state_.scripts;v.erase(std::remove_if(v.begin(),v.end(),[&](const auto& a){return a.id==id;}),v.end());changed(name);}return Value{};
    }
    if(name=="AreaSetKeyInScript"||name=="KeySetScript"){
        if(args.empty())return Value{};
        auto id=integer(args[0]);auto it=std::find_if(state_.scripts.begin(),state_.scripts.end(),[&](const auto& a){return a.id==id;});
        if(it==state_.scripts.end()){state_.scripts.push_back({});it=state_.scripts.end()-1;it->id=id;}
        it->native[0]=0;std::fill(it->native.begin()+1,it->native.begin()+16,-1);it->native[16]=0;it->history_indication=false;
        if(args.size()>4){
            it->script=text(args[1]);it->native[14]=integer(args[2]);it->native[15]=integer(args[3]);
            if(args.size()<=7){it->keys.clear();for(size_t i=4;i<args.size();++i)it->keys.push_back(integer(args[i]));}
        }
        // Native short calls leave the separately stored key table/script
        // intact, even though mouse-area fields are disabled/reset.
        changed(name);return Value{};
    }
    if(name=="AreaSetMouseInScript"){
        if(args.empty())return Value{};
        auto id=integer(args[0]);auto it=std::find_if(state_.scripts.begin(),state_.scripts.end(),[&](const auto& a){return a.id==id;});
        if(it==state_.scripts.end()){state_.scripts.push_back({});it=state_.scripts.end()-1;it->id=id;}
        it->native[0]=0;std::fill(it->native.begin()+1,it->native.begin()+16,-1);it->native[16]=0;it->history_indication=false;
        if(args.size()>2){it->script=text(args[1]);it->mouse_inputs.clear();for(size_t i=2;i<args.size();++i){auto input=integer(args[i]);if(input>=1&&input<=5)it->mouse_inputs.push_back(input);}}
        changed(name);return Value{};
    }
    if(name=="AreaSetMouseLeftScript"||name=="AreaSetMouseAreaScript"||name=="AreaSetIndicate"||name=="AreaSetHistoryIndicate"){
        if(args.empty())return Value{};
        int32_t id=integer(args[0]);
        auto it=std::find_if(state_.scripts.begin(),state_.scripts.end(),[&](const auto& a){return a.id==id;});
        if(it==state_.scripts.end()){state_.scripts.push_back({});it=state_.scripts.end()-1;it->id=id;}
        it->native[0]=0;
        bool indication=name=="AreaSetIndicate"||name=="AreaSetHistoryIndicate";
        std::vector<Value> translated=args;
        if(indication){
            // Native wrappers insert empty script, append -1 placeholders,
            // then pass 2..10=>n+1, 11=>13, >=12=>n+4 to MouseLeftScript.
            if(args.size()<2)throw std::runtime_error("indicate area needs valid original parameter count");
            if(args.size()>12)throw std::runtime_error("indicate area exceeds native fixed argument pointer record");
            translated.insert(translated.begin()+1,std::string{});
            // local_10 (pointer index12) is always the inserted -1; the
            // twelfth original arg occupies local_c (index13), not index12.
            if(args.size()==12)translated.insert(translated.begin()+12,int32_t(-1));
            size_t target=args.size()<=10?args.size()+1:args.size()==11?13:args.size()+4;
            translated.resize(target,int32_t(-1));
        }
        if(translated.size()>6){
            auto script=text(translated[1]);std::vector<Value> region(translated.begin()+2,translated.end());
            AreaDefinition replacement=*it;normalize(replacement,region);replacement.history_indication=false;replacement.script=std::move(script);*it=std::move(replacement);
        }
        if(name=="AreaSetMouseAreaScript"){it->native[16]=bits(uint32_t(it->native[16])|1);it->mouse_area=true;}
        if(name=="AreaSetHistoryIndicate"){it->native[16]=bits(uint32_t(it->native[16])|0x80000000);it->history_indication=true;}
        changed(name);return Value{};
    }
    return std::nullopt;
}
}

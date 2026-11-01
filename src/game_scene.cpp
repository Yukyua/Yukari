#include "yukari/game_scene.hpp"
#include "yukari/sprite_motion.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace yukari {
namespace {
const char* directory="system\\game\\";
const std::array<const char*,5> buttons={"red","yellow","white","green","blue"};
const std::array<const char*,5> characters={"otome","natumi","yuduki","megumi","saki"};
const std::array<SDL_Point,5> character_position={{{9,11},{79,151},{9,291},{79,431},{9,571}}};
const std::array<SDL_Point,5> effect_position={{{447,501},{502,492},{558,491},{617,492},{670,500}}};
std::string resource(const std::string& name){return std::string(directory)+name;}
}
struct MusicGameScene::Impl {
    struct Asset {Bytes bytes;SpriteGroups groups;};
    struct Sprite {int id{};std::string asset;SpriteMotion motion;bool visible{};};
    Scene& scene;Audio& audio;ResourceReader reader;
    std::unique_ptr<MusicGame> game;
    std::map<std::string,Asset> assets;
    std::array<Sprite,5> character_sprites,effects;
    std::array<std::vector<Sprite>,5> notes;
    std::vector<ResourceSource> sound_parts;std::function<ResourceSource(const std::string&)> audio_reader;
    MusicGameOptions options;
    Phase phase{Phase::Idle};uint64_t now{},phase_start{},game_start{};
    bool confirmation{};
    std::array<bool,5> previous_keys{};
    Scene::Frame old;
    TransitionSampling sampling{1000,2,0};
    uint64_t fade_duration{1000};
    Impl(Scene& s,Audio& a,ResourceReader r):scene(s),audio(a),reader(std::move(r)){if(!reader)throw std::runtime_error("GamePart requires a resource reader");}
    Asset& asset(const std::string& name,bool sprite=false){
        auto found=assets.find(name);if(found!=assets.end())return found->second;
        auto bytes=reader(name);if(bytes.empty())throw std::runtime_error("GamePart resource is empty: "+name);
        Asset value;value.bytes=std::move(bytes);if(sprite){value.groups=split_sprite_groups(parse_graphic_frames(value.bytes));if(value.groups.empty())throw std::runtime_error("GamePart sprite has no groups: "+name);}
        return assets.emplace(name,std::move(value)).first->second;
    }
    void load(int id,const std::string& name,int x=0,int y=0){auto& value=asset(name);scene.load_cached(id,"gamepart:"+name,value.bytes,x,y);}
    const Frame& frame(const Sprite& sprite)const{return sprite_motion_group(assets.at(sprite.asset).groups,sprite.motion).at(sprite.motion.frame);}
    void apply(Sprite& sprite){const auto& f=frame(sprite);scene.set_clip(sprite.id,{int(f.x),int(f.y),int(f.w),int(f.h)});scene.set_visible(sprite.id,sprite.visible);}
    void start(Sprite& sprite,int id,const std::string& name,int group,int mode,SDL_Point point,int z,bool visible=true){
        sprite={id,name,{},visible};sprite.motion.group=group;sprite.motion.mode=mode;sprite_motion_reset(asset(name,true).groups,sprite.motion,now);
        load(id,name,point.x,point.y);scene.set_z(id,z);apply(sprite);
    }
    void animate(Sprite& sprite){if(sprite.asset.empty())return;if(sprite_motion_tick(assets.at(sprite.asset).groups,sprite.motion,now))apply(sprite);}
    void reset(Sprite& sprite,int group,bool change_group=true){if(change_group){sprite.motion.group=group;sprite.motion.frame=0;}sprite_motion_reset(assets.at(sprite.asset).groups,sprite.motion,now);sprite.visible=true;apply(sprite);}
    void set_phase(Phase value){phase=value;phase_start=now;}
    void capture(){old=scene.snapshot();}
    void fade(Phase value){set_phase(value);fade_duration=sampling.quality<0?0:scene.transition_timing(2,sampling).total_ms;}
    void background(){
        scene.clear();scene.clear_prefix_cache();int type=game->chart().background;if(type<2||type>6)type=1;
        load(0,resource("minigame_bg0"+std::to_string(type)+".png"));load(10,"system\\null.png");scene.set_visible(10,false);fade(Phase::BackgroundFade);
    }
    void characters_begin(){
        for(unsigned lane=0;lane<5;++lane)start(character_sprites[lane],10000+int(lane),resource(std::string("minigame_")+characters[lane]+".ykg"),options.character_variants[lane]?3:0,1,character_position[lane],0);
        fade(Phase::CharactersFade);
    }
    void notes_begin(){
        for(unsigned lane=0;lane<5;++lane){
            auto note_name=resource(std::string("minigame_button_")+buttons[lane]+".ykg");
            for(size_t n=0;n<game->chart().lanes[lane].size();++n){Sprite sprite;start(sprite,20000+int(lane)*1000+int(n),note_name,0,1,{610,0},10,false);notes[lane].push_back(std::move(sprite));}
            start(effects[lane],11000+int(lane),resource("minigame_effect.ykg"),1,1,effect_position[lane],0,false);
        }
        load(20,resource("minigame_wait.png"),0,-720);scene.set_alpha(20,127);set_phase(Phase::WaitIn);
    }
    void start_music(){
        if(sound_parts.empty())throw std::runtime_error("GamePart SoundFile is empty");
        audio.load_sources(1,sound_parts,true,0);audio.play(1);game_start=now;set_phase(Phase::Playing);
    }
    void result_begin(){
        if(game->successful()){capture();load(100,resource("minigame_finish_good.png"));scene.set_z(100,100);fade(Phase::GoodFade);}
        else{load(20,resource("minigame_finish_bad.png"),0,-720);scene.set_alpha(20,229);set_phase(Phase::BadBounce);}
    }
    void final_black(){capture();scene.clear();scene.clear_prefix_cache();load(100,"system\\black.png");fade(Phase::FinalBlack);}
};
MusicGameScene::MusicGameScene(Scene& scene,Audio& audio,ResourceReader reader):impl_(std::make_unique<Impl>(scene,audio,std::move(reader))){}
MusicGameScene::~MusicGameScene()=default;
void MusicGameScene::set_transition_sampling(TransitionSampling sampling){impl_->sampling=sampling;}
void MusicGameScene::set_audio_reader(std::function<ResourceSource(const std::string&)> reader){impl_->audio_reader=std::move(reader);}
void MusicGameScene::begin(const std::string& chart,const MusicGameOptions& options,uint64_t now){
    auto& m=*impl_;if(m.phase!=Phase::Idle&&m.phase!=Phase::Finished)throw std::runtime_error("GamePart scene is already active");
    // Prepare required data before disturbing the parent's visible scene.
    auto game=std::make_unique<MusicGame>(parse_music_chart(m.reader(chart)),options);
    m.assets.clear();m.sound_parts.clear();
    m.asset("system\\black.png");m.asset("system\\null.png");int bg=game->chart().background;if(bg<2||bg>6)bg=1;
    m.asset(resource("minigame_bg0"+std::to_string(bg)+".png"));m.asset(resource("minigame_wait.png"));{SpriteMotion effect;effect.group=1;effect.mode=1;sprite_motion_reset(m.asset(resource("minigame_effect.ykg"),true).groups,effect,now);}
    for(unsigned lane=0;lane<5;++lane){m.asset(resource(std::string("minigame_button_")+buttons[lane]+".ykg"),true);auto& a=m.asset(resource(std::string("minigame_")+characters[lane]+".ykg"),true);for(unsigned group=0;group<3;++group){SpriteMotion motion;motion.group=int((options.character_variants[lane]?3:0)+group);motion.mode=0;sprite_motion_reset(a.groups,motion,now);}}
    const auto& sound=game->chart().sound_file;size_t at=0;while(at<sound.size()){auto end=sound.find('\t',at);if(end==std::string::npos)end=sound.size();if(end==at)throw std::runtime_error("GamePart SoundFile contains an empty fragment");{auto name=decode(sound.substr(at,end-at));m.sound_parts.push_back(m.audio_reader?m.audio_reader(name):ResourceSource::memory(std::make_shared<const Bytes>(m.reader(name))));}at=end+1;}
    if(m.sound_parts.empty())throw std::runtime_error("GamePart SoundFile is empty");
    m.game=std::move(game);m.options=options;m.now=now;m.confirmation=false;m.previous_keys={};for(auto& n:m.notes)n.clear();m.character_sprites={};m.effects={};
    m.capture();if(m.audio.loaded(1))m.audio.fade_stop(1,1000);m.scene.clear();m.scene.clear_prefix_cache();m.load(0,"system\\black.png");m.fade(Phase::ToBlack);
}
void MusicGameScene::confirm(){impl_->confirmation=true;}
void MusicGameScene::tick(uint64_t now,const std::array<bool,5>& keys,bool abort){
    auto& m=*impl_;if(m.phase==Phase::Idle||m.phase==Phase::Finished)return;if(now<m.now)throw std::runtime_error("GamePart clock moved backwards");m.now=now;
    bool rising=false;for(size_t i=0;i<5;++i){rising|=keys[i]&&!m.previous_keys[i];m.previous_keys[i]=keys[i];}
    for(auto& s:m.character_sprites)m.animate(s);for(auto& s:m.effects)m.animate(s);for(auto& lane:m.notes)for(auto& s:lane)m.animate(s);
    const auto elapsed=now-m.phase_start;
    switch(m.phase){
    case Phase::ToBlack:if(elapsed>=m.fade_duration){m.capture();m.background();}break;
    case Phase::BackgroundFade:if(elapsed>=m.fade_duration){m.capture();m.characters_begin();}break;
    case Phase::CharactersFade:if(elapsed>=m.fade_duration)m.notes_begin();break;
    case Phase::WaitIn:m.scene.set_position(20,0,int(-720+720*std::min<uint64_t>(elapsed,300)/300));if(elapsed>=300){m.confirmation=false;m.set_phase(Phase::Waiting);}break;
    case Phase::Waiting:if(m.confirmation||rising){m.confirmation=false;m.set_phase(Phase::WaitOut);}break;
    case Phase::WaitOut:m.scene.set_position(20,0,-int(720*std::min<uint64_t>(elapsed,300)/300));if(elapsed>=300)m.start_music();break;
    case Phase::Playing:{
        const uint32_t game_time=uint32_t(now-m.game_start);for(unsigned lane=0;lane<5;++lane)for(size_t n=0;n<m.notes[lane].size();++n){const auto p=m.game->position(lane,n,game_time);auto& sprite=m.notes[lane][n];sprite.visible=p.visible;m.scene.set_position(sprite.id,p.x,p.y);m.scene.set_visible(sprite.id,p.visible);}
        for(const auto& event:m.game->tick(game_time,keys,abort)){m.reset(m.character_sprites[event.lane],int(event.character_group));if(event.hit){m.notes[event.lane][event.note].visible=false;m.scene.set_visible(m.notes[event.lane][event.note].id,false);m.reset(m.effects[event.lane],0,false);}}
        if(m.game->finished())m.result_begin();break;}
    case Phase::GoodFade:if(elapsed>=m.fade_duration)m.set_phase(Phase::ResultHold);break;
    case Phase::BadBounce:{
        static const uint64_t duration[]={300,100,100,50,50};static const int y[]={-720,0,-50,0,-10,0};uint64_t step_time=elapsed;size_t step=0;while(step<5&&step_time>=duration[step]){step_time-=duration[step];++step;}if(step==5){m.scene.set_position(20,0,0);m.set_phase(Phase::ResultHold);}else m.scene.set_position(20,0,y[step]+int(int64_t(y[step+1]-y[step])*step_time/int64_t(duration[step])));break;}
    case Phase::ResultHold:if(elapsed>=3000)m.final_black();break;
    case Phase::FinalBlack:if(elapsed>=m.fade_duration){if(m.audio.loaded(1))m.audio.stop(1);m.set_phase(Phase::Finished);}break;
    default:break;
    }
}
void MusicGameScene::render(){auto& m=*impl_;switch(m.phase){case Phase::ToBlack:case Phase::BackgroundFade:case Phase::CharactersFade:case Phase::GoodFade:case Phase::FinalBlack:if(m.sampling.quality<0)m.scene.render();else m.scene.render_transition(m.old,2,m.fade_duration?std::min(1.0,double(m.now-m.phase_start)/m.fade_duration):1.0,{},nullptr,m.sampling);break;default:m.scene.render();break;}}
bool MusicGameScene::finished()const{return impl_->phase==Phase::Finished;}
uint32_t MusicGameScene::result()const{return impl_->game?impl_->game->hits():0;}
bool MusicGameScene::successful()const{return impl_->game&&impl_->game->successful();}
MusicGameScene::Phase MusicGameScene::phase()const{return impl_->phase;}
}

#include <tuple>
#include "yukari/translation_patch.hpp"
#include "yukari/text_schedule.hpp"
#include "yukari/save_preview.hpp"
#include "yukari/runtime.hpp"
#include "yukari/frame_profile.hpp"
#include "yukari/render.hpp"
#include "yukari/audio.hpp"
#include "yukari/audio_gain.hpp"
#include "yukari/mac_cd_player.hpp"
#include "yukari/audio_commands.hpp"
#include "yukari/cd_commands.hpp"
#include "yukari/vm.hpp"
#include "yukari/save.hpp"
#include "yukari/original_scene.hpp"
#include "yukari/original_layer_state.hpp"
#include "yukari/original_system.hpp"
#include "yukari/services.hpp"
#include "yukari/engine_config.hpp"
#include "yukari/movie.hpp"
#include "yukari/midi.hpp"
#include "yukari/game_scene.hpp"
#include "yukari/text_model.hpp"
#include "yukari/history_view.hpp"
#include "yukari/motion_model.hpp"
#include "yukari/mode_visuals.hpp"
#include "yukari/particle_model.hpp"
#include "yukari/pe_resources.hpp"
#include "yukari/native_platform.hpp"
#include "yukari/native_text_box.hpp"
#include "yukari/input.hpp"
#include "yukari/selection_model.hpp"
#include "yukari/settings_services.hpp"
#include "yukari/area_services.hpp"
#include "yukari/platform_services.hpp"
#include <glob.h>
#include "yukari/sprite_motion.hpp"
#include <sstream>
#include <SDL.h>
#include <png.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace yukari {
namespace {
int number(const Value& v) {
    if (auto n = std::get_if<int32_t>(&v)) return *n;
    throw std::runtime_error("engine service expected integer");
}
std::string text(const Value& v) {
    if (auto s = std::get_if<std::string>(&v)) return *s;
    throw std::runtime_error("engine service expected string");
}
int windows_virtual_key(SDL_Keycode k){if(k>=SDLK_a&&k<=SDLK_z)return int(k-SDLK_a+'A');if(k>=SDLK_0&&k<=SDLK_9)return int(k);if(k>=SDLK_F1&&k<=SDLK_F12)return int(k-SDLK_F1+112);switch(k){case SDLK_RETURN:return 13;case SDLK_ESCAPE:return 27;case SDLK_SPACE:return 32;case SDLK_TAB:return 9;case SDLK_BACKSPACE:return 8;case SDLK_LEFT:return 37;case SDLK_UP:return 38;case SDLK_RIGHT:return 39;case SDLK_DOWN:return 40;case SDLK_LSHIFT:case SDLK_RSHIFT:return 16;case SDLK_LCTRL:case SDLK_RCTRL:return 17;case SDLK_LALT:case SDLK_RALT:return 18;default:return 0;}}
struct Args {
    const std::vector<Value>& a;
    int n(size_t i) const { return number(a.at(i)); }
    std::string s(size_t i) const { return text(a.at(i)); }
    size_t size() const { return a.size(); }
};
std::string key(std::string s) { for (char& c:s) if (c>='A'&&c<='Z') c=char(c-'A'+'a'); return s; }
struct Asset { Bytes bytes; std::vector<std::vector<Frame>> groups; };
struct Layer { std::string asset; int x{},y{},alpha=255; bool visible=true; int image_type=2; std::string mask; double transmittance=0; };
struct Sprite : SpriteMotion {
    int scene_id{},parent{},slot{},source{},x{},y{};
    uint64_t started{}; bool visible=true;
};
struct AudioFade { int id,from,to;uint64_t start,end;bool group,stop; };
struct Motion { int id{},x{},y{},end_x{},end_y{},alpha{},end_alpha{}; uint64_t start{},end{}; bool position{}; double alpha_from{},alpha_to{}; };
struct HistoryEntry { std::string text,voice;Bytes native; };
struct TextBox { int x=235,y=457,w=735,h=100,font=0; TextStyle style; std::string content; };
struct Region { std::string kind; std::vector<Value> args; uint32_t branch{},end{};bool fired=false;uint64_t fired_at=0; };
struct MoviePlayback {
    std::unique_ptr<Movie> decoder;
    SDL_Texture* texture{};
    SDL_AudioDeviceID device{};
    uint64_t start{};
    bool cancellable=true;
    ~MoviePlayback(){if(device)SDL_CloseAudioDevice(device);if(texture)SDL_DestroyTexture(texture);}
};

class Runtime {
    Resources resources;
    std::filesystem::path game_directory;
    std::string original_encoding="CP932";
    std::string window_title="Yukari";
    Bytes original_template,system_template;
    int initial_native=-1;
    Scene scene{1024,576,"Yukari"};
    Audio audio;
    Midi midi;
    std::unique_ptr<MacCdPlayer> cd_player;
    std::unique_ptr<CdCommands> cd_commands;
    TranslationPatch translation_patch;
    Vm vm;
    ScalarServices services;
    EngineConfig engine_config;
    int transition_wait_type=0;
    std::unique_ptr<PlatformServices> platform;
    SettingsServices settings{SettingsHooks{{},[this]{return (SDL_GetWindowFlags(scene.window())&SDL_WINDOW_FULLSCREEN)!=0;},[this](bool full){if(SDL_SetWindowFullscreen(scene.window(),full?SDL_WINDOW_FULLSCREEN_DESKTOP:0)<0)throw std::runtime_error(SDL_GetError());}}};
    std::unique_ptr<MoviePlayback> movie;
    std::unique_ptr<MusicGameScene> music_game;Bytes music_parent_state;
    int movie_volume=1000,movie_attenuation=0;
    bool muted=false;
    std::filesystem::path state_dir;

    Bytes menu_vm,menu_runtime,menu_thumbnail;
    OriginalSaveState original_state,menu_native_vm;
    OriginalScene menu_native_scene;
    std::map<int,OriginalLayer> native_layers;
    int pending_slot=0;
    std::string pending_comment;
    std::unordered_map<std::string,Asset> assets;
    std::map<int,Layer> layers;
    AreaServices areas{AreaHooks{[this](int id)->std::optional<AreaLayer>{auto it=layers.find(id);if(it==layers.end())return {};return AreaLayer{it->second.visible,double(it->second.x),double(it->second.y)};},{},{}}};
    std::map<std::pair<int,int>,Sprite> sprites;
    std::map<std::pair<int,bool>,Motion> motions;
    std::map<int,MotionModel> layer_motion;
    std::set<int> modified_pixels;
    std::map<uint64_t,int> particle_scene_ids;
    int next_particle_scene_id=16000000;
    ParticleModel particles;
    std::unique_ptr<ModeVisuals> mode_visuals;
    std::array<std::optional<std::pair<int,int>>,4> mode_icon_sprites;
    uint64_t motion_tick=0;
    int32_t cursor_auto_hide=0;uint64_t cursor_hide_at=0;
    bool load_to_draw_stop=false,auto_window_region=false;
    std::map<int,bool> key_wait_visibility;
    std::optional<SpriteMotion> key_wait_icon;
    int key_wait_icon_parent=13,key_wait_icon_x=0,key_wait_icon_y=0;
    std::map<std::string,std::vector<Value>> configuration;
    std::map<int,uint64_t> audio_ends;
    std::map<int,int> channel_volumes,group_volumes;
    std::map<std::pair<int,bool>,AudioFade> audio_fades;
    std::map<int,TextBox> texts;
    std::map<int,TextStyle> fonts;
    std::unique_ptr<NativeTextBox> native_text_box;
    std::unique_ptr<TextModel> text_model;
    std::map<uint64_t,TextStyle> text_styles;
    uint64_t next_text_style=1;
    std::map<int,std::vector<int>> inline_text_images;
    int next_inline_image=12000000;
    struct PendingText {int id;std::string encoded;size_t offset=0;};
    std::vector<PendingText> pending_text;
    size_t pending_text_index=0;
    bool pending_text_slow=false;
    std::vector<int> hidden_text_layers;
    VmLocation pending_text_location;

    std::map<int,std::pair<std::string,bool>> sounds;
    AudioCommands audio_commands{audio,[this](const std::string& name){return read_resource(name);},sounds};
    int text_layer=13;
    uint64_t text_pages=0;
    int mouse_x=0,mouse_y=0;
    std::string input_value,input_original;int input_limit=26;
    std::vector<HistoryEntry> history;bool history_mode=false;
    std::unique_ptr<HistoryView> history_view;
    std::map<int,bool> history_visibility;
    std::vector<int> history_scene_ids;
    bool history_host_wait=false,history_old_drawing=true,history_drag=false;
    int history_hover=-1;std::optional<int> history_voice_channel;
    uint64_t history_opened=0;
    bool left_down=false,auto_mode=false,skip_mode=false,already_mode=false,ordinary_skip_mode=false;
    uint64_t auto_deadline=0,reveal_next=0,key_wait_started=0;
    int key_wait_tte_mode=0;
    std::map<int,size_t> reveal_bytes;
    std::string pending_menu;
    bool in_menu=false;
    std::function<void()> menu_resume_context;
    bool until_text=false;
    int auto_new=0,auto_advance=0;
    std::vector<std::string> actions;size_t action_index=0;
    struct HostTiming {uint64_t calls{};double ms{},max_ms{};};std::map<std::string,HostTiming> host_timings;
    FrameProfile frame_profile;bool profile_failed=false;double profile_present_ms=0;
    bool action_pause=false;uint64_t action_wait_until=0;
    std::vector<Region> regions;
    uint32_t random;
    uint32_t native_random(){random=random*214013U+2531011U;return (random>>16)&32767U;}
    enum class Wait { None, Time, Audio, Motion, Sprite, Selection, Text, Save, Load, Reveal, Input, Movie, TextHidden, MusicGame, CdTime, History } wait=Wait::None;
    uint64_t now{},deadline{},transition_start{},transition_end{};
    int transition_mode=1;
    Bytes transition_mask;
    std::optional<SDL_Rect> transition_rect;
    TransitionSampling transition_sampling;
    uint64_t effect_start=0,effect_end=0;int effect_type=0,effect_interval=20,effect_amplitude=5;
    int wait_id{},wait_slot{},next_sprite_id=1000000;
    bool drawing=true,quit=false,title=false,fast=false,until_title=false;
    bool collection=false; std::string selection_script;
    int hover=-1, hover_test_x=-1,hover_test_y=-1;
    bool text_selection=false,timed_selection=false,selection_tte_loop=false;
    bool selection_event=false,pending_subcall=false,selection_checkpoint=false;
    std::optional<std::string> pending_restart;
    int select_layer=13,select_x=0,select_y=0,select_width=0,select_height=27,select_mode=0;
    uint32_t select_normal=0xffffffff,select_hover=0xf2d51f,select_read_normal=0xffffffff,select_read_hover=0xffffffff;
    uint64_t select_start=0,select_limit=0,selection_elapsed=0;
    uint32_t select_end=0;
    std::vector<int> choice_layers;
    SelectionModel selector;
    SelectionNavigationOptions selector_navigation;
    std::optional<size_t> selector_focus;
    std::array<int32_t,16> key_history{};
    std::map<std::pair<size_t,unsigned>,std::pair<int,int>> selector_sprites;
    std::map<int,uint64_t> selector_delays;
    std::filesystem::path screenshot;
    uint64_t max_ms=0;
    Scene::Frame previous;

    ResourceSource open_resource(const std::string& name)const{
        if(resources.contains(name))return resources.open(name);auto file=name;std::replace(file.begin(),file.end(),'\\','/');return ResourceSource::file(game_directory/file);
    }
    std::vector<ResourceSource> open_audio_parts(const std::string& name)const{
        std::vector<ResourceSource> parts;std::istringstream names(name);std::string part;while(std::getline(names,part,'\t')){if(part.empty())throw std::runtime_error("empty audio fragment");parts.push_back(open_resource(part));}return parts;
    }
    Bytes read_resource(const std::string& name) const {
        return open_resource(name).read_all();
    }
    void reload_layer_image(int id,const std::string& name){
        if(!layers.count(id))return;auto& layer=layers.at(id);scene.load_deferred(id,key(name),[this,name]{return asset(name).bytes;},layer.x,layer.y);layer.asset=key(name);layer.mask.clear();scene.set_image_type(id,layer.image_type);scene.set_z(id,id*10000);scene.set_alpha(id,layer.alpha);scene.set_visible(id,layer.visible);apply_motion(id);modified_pixels.insert(id);
    }
    MotionModel& motion_for(int id){auto [it,inserted]=layer_motion.try_emplace(id);if(inserted&&layers.count(id)){it->second.x=layers.at(id).x;it->second.y=layers.at(id).y;}return it->second;}
    void apply_motion(int id){if(!layers.count(id))return;auto& m=motion_for(id);auto& l=layers.at(id);l.x=int(m.x);l.y=int(m.y);scene.set_position(id,l.x,l.y);scene.set_scale(id,m.scale_enabled?m.scale_x*1000:1000,m.scale_enabled?m.scale_y*1000:1000);scene.set_rotate(id,m.angle);scene.set_raster(id,m.raster_visible(),m.raster_amplitude(),m.raster_frequency,m.raster_phase);update_parent(id);}
    Asset& asset(const std::string& name) {
        const auto k=key(name);
        auto found=assets.find(k); if(found!=assets.end())return found->second;
        Asset a; a.bytes=resources.read(name);
        if(a.bytes.size()>=6 && std::equal(a.bytes.begin(),a.bytes.begin()+6,"YKG000")) {
            a.groups=split_sprite_groups(parse_graphic_frames(a.bytes));
        }
        return assets.emplace(k,std::move(a)).first->second;
    }
    void position_sprite(Sprite& s) {
        const auto& parent=layers.at(s.parent);
        scene.set_position(s.scene_id,parent.x+s.x,parent.y+s.y);
        scene.set_alpha(s.scene_id,std::find(engine_config.sprite_without_graphic_attribute.begin(),engine_config.sprite_without_graphic_attribute.end(),s.parent)!=engine_config.sprite_without_graphic_attribute.end()?255:parent.alpha);
        scene.set_visible(s.scene_id,parent.visible&&s.visible&&(!selector_delays.count(s.scene_id)||now>=selector_delays.at(s.scene_id)));
    }
    void update_parent(int id) {
        for(auto& item:sprites)if(item.second.parent==id)position_sprite(item.second);
        if(text_model&&text_model->layers().count(id))draw_text(id);
    }
    void erase_layer(int id) {
        for(auto it=sprites.begin();it!=sprites.end();) {
            if(it->second.parent==id){ scene.erase(it->second.scene_id);it=sprites.erase(it); }else ++it;
        }
        for(const char* command:{"GraphicAddColor","GraphicSubColor","GraphicSepia","GraphicMonochrome","GraphicNegaposi","GraphicScale","GraphicScaleCenter","GraphicRotate","GraphicRotateCenter","GraphicFilter"})configuration.erase(std::string(command)+":"+std::to_string(id));
        motions.erase({id,true}); motions.erase({id,false});layer_motion.erase(id);modified_pixels.erase(id);particles.erase_layer(id);
        scene.erase(id);scene.erase(700000+id);scene.erase(710000+id);scene.erase(720000+id);scene.erase(800000+id);texts.erase(id);if(text_model)text_model->erase_layer(id);if(inline_text_images.count(id)){for(int image:inline_text_images.at(id))scene.erase(image);inline_text_images.erase(id);}layers.erase(id);
    }
    void load_layer(int id,const std::string& name,int x,int y,bool visible,int image_type=2,const Bytes& mask={}) {
        if(id<0||id>50000)throw std::runtime_error("graphic layer ID outside supported range");
        erase_layer(id);
        scene.load_deferred(id,key(name),[this,name]{return asset(name).bytes;},x,y,mask);scene.set_z(id,id*10000);scene.set_image_type(id,image_type);
        scene.set_visible(id,visible); layers[id]={key(name),x,y,255,visible,image_type,{}};
    }
    const std::vector<Frame>& group_for(const Sprite& s) {
        auto& groups=assets.at(layers.at(s.source).asset).groups;
        if(s.group<0||size_t(s.group)>=groups.size()||groups[s.group].empty())throw std::runtime_error("sprite motion group not found");
        return groups[s.group];
    }
    void apply_frame(Sprite& s) {
        const auto& groups=assets.at(layers.at(s.source).asset).groups;
        s.next=now;
        const auto& frame=sprite_motion_apply(groups,s,now);
        scene.set_clip(s.scene_id,{int(frame.x),int(frame.y),int(frame.w),int(frame.h)});
        position_sprite(s);
    }
    void create_sprite(int parent,int slot,int x,int y,int source,int group,bool visible=true) {
        if(!layers.count(parent)||!layers.count(source))throw std::runtime_error("sprite parent/source layer missing");
        auto k=std::make_pair(parent,slot);
        auto it=sprites.find(k);if(it!=sprites.end())scene.erase(it->second.scene_id);
        Sprite s; s.scene_id=next_sprite_id++;s.parent=parent;s.slot=slot;s.x=x;s.y=y;s.source=source;s.group=group;s.visible=visible;s.started=now;
        scene.copy(s.scene_id,source);
        scene.set_z(s.scene_id,parent*10000+slot+1);
        const auto& groups=assets.at(layers.at(s.source).asset).groups;
        const auto& frame=sprite_motion_reset(groups,s,now);
        scene.set_clip(s.scene_id,{int(frame.x),int(frame.y),int(frame.w),int(frame.h)});
        position_sprite(s);sprites.insert_or_assign(k,std::move(s));
    }
    void snapshot() { previous=scene.snapshot(); }
    static Bytes bmp_thumbnail(const Bytes& png) {
        png_image image{};image.version=PNG_IMAGE_VERSION;if(!png_image_begin_read_from_memory(&image,png.data(),png.size()))throw std::runtime_error("cannot decode save thumbnail");image.format=PNG_FORMAT_RGB;Bytes rgb(PNG_IMAGE_SIZE(image));if(!png_image_finish_read(&image,nullptr,rgb.data(),0,nullptr)){png_image_free(&image);throw std::runtime_error("cannot decode thumbnail pixels");}
        const uint32_t stride=(image.width*3+3)&~3U;Bytes out(54+stride*image.height);auto put=[&](size_t at,uint32_t v){for(int i=0;i<4;++i)out[at+i]=uint8_t(v>>(i*8));};out[0]='B';out[1]='M';put(2,uint32_t(out.size()));put(10,54);put(14,40);put(18,image.width);put(22,image.height);out[26]=1;out[28]=24;put(34,stride*image.height);
        for(uint32_t y=0;y<image.height;++y)for(uint32_t x=0;x<image.width;++x){size_t a=(size_t(y)*image.width+x)*3,b=54+size_t(image.height-y-1)*stride+x*3;out[b]=rgb[a+2];out[b+1]=rgb[a+1];out[b+2]=rgb[a];}png_image_free(&image);return out;
    }
    std::filesystem::path native_slot_path(int slot,bool source=false) const {std::ostringstream name;name<<"savedata."<<std::setw(3)<<std::setfill('0')<<slot;return (source?game_directory/"save":state_dir)/name.str();}
    std::filesystem::path existing_native_slot(int slot) const {auto p=native_slot_path(slot);return std::filesystem::exists(p)?p:native_slot_path(slot,true);}
    bool slot_exists(int slot) const {return std::filesystem::exists(slot_path(slot))||std::filesystem::exists(existing_native_slot(slot));}
    SaveGame slot_info(int slot) const {
        if(std::filesystem::exists(slot_path(slot)))return decode_save(read_file(slot_path(slot)));
        SaveGame info;auto path=existing_native_slot(slot);auto bytes=read_file(path);inspect_original_save(bytes);
        auto string_at=[&](size_t field){auto at=u32(bytes,field),size=u32(bytes,field+4);require_range(bytes.size(),at,size,"save metadata");if(!size)return std::string{};auto first=bytes.begin()+at,end=first,last=first+size;while(end!=last&&*end)++end;try{return decode(std::string(first,end),original_encoding.c_str());}catch(const std::runtime_error&){return std::string("[encoding]");}};
        info.comment=string_at(0x10);auto at=u32(bytes,0x30),size=u32(bytes,0x34);require_range(bytes.size(),at,size,"native thumbnail");info.thumbnail.assign(bytes.begin()+at,bytes.begin()+at+size);
        auto stamp=std::filesystem::last_write_time(path);auto system_stamp=std::chrono::system_clock::now()+(stamp-decltype(stamp)::clock::now());info.timestamp=std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::system_clock::duration>(system_stamp));return info;
    }
    OriginalScene native_scene() {
        OriginalScene result;
        for(const auto& [id,l]:layers){if(std::find(engine_config.save_data_without_graphic_layer.begin(),engine_config.save_data_without_graphic_layer.end(),id)!=engine_config.save_data_without_graphic_layer.end())continue;OriginalLayer layer;if(native_layers.count(id))layer=native_layers.at(id);layer.id=id;layer.resource=l.asset;layer.x=l.x;layer.y=l.y;layer.visible=l.visible;layer.transmittance=1000*(255-l.alpha)/255;layer.sprites.clear();
            for(auto& [k,b]:sprites)if(b.parent==id&&b.slot<9000){OriginalSprite sprite;sprite.kind=1;sprite.named=true;sprite.slot=b.slot;sprite.visible=b.visible;sprite.x=b.x;sprite.y=b.y;sprite.source_layer=b.source;sprite.group=b.group;sprite.frame=int(b.frame);sprite.mode=b.mode;sprite.wait=b.done?0x7fffffff:int(std::min<uint64_t>(0x7fffffff,b.next>now?b.next-now:0));const auto& frames=group_for(b);const auto& frame=frames.at(b.frame);sprite.width=int(frame.w);sprite.height=int(frame.h);sprite.left=int(frame.x);sprite.top=int(frame.y);sprite.right=int(frame.x+frame.w);sprite.bottom=int(frame.y+frame.h);sprite.animation=frames;layer.sprites.push_back(std::move(sprite));}
            if(text_model->layers().count(id))for(const auto& rec:text_model->layers().at(id).records){OriginalSprite glyph;glyph.kind=rec.kind;glyph.slot=-1;glyph.visible=true;glyph.source_layer=-1;glyph.x=rec.x;glyph.y=rec.y;glyph.width=rec.width;glyph.height=rec.height;glyph.right=rec.width;glyph.bottom=rec.height;glyph.font=rec.font;glyph.frame=glyph.group=rec.image_id;auto style=text_styles.count(rec.style_token)?text_styles.at(rec.style_token):current_text_style(id,rec.font);glyph.color=uint32_t(style.color.r)|(uint32_t(style.color.g)<<8)|(uint32_t(style.color.b)<<16);if(style.native_font&&rec.kind==3){const auto& font=*style.native_font;glyph.font_effect=font.inherit_effect?0:font.effect;glyph.shadow_color=font.inherit_shadow?0xffffffffU:font.shadow_color;glyph.border_color=font.inherit_border?0xffffffffU:font.border_color;}if(rec.kind==3){if(rec.encoded.size()<=2){if(!rec.encoded.empty())glyph.encoded_character=uint8_t(rec.encoded[0])|(rec.encoded.size()>1?uint32_t(uint8_t(rec.encoded[1]))<<8:0);}else{glyph.payload=Bytes(rec.encoded.begin(),rec.encoded.end());glyph.payload.push_back(0);}}layer.sprites.push_back(std::move(glyph));}
            if(layer.native_header.empty()){layer.native_header.resize(0x140);layer.native_header[0]=0x40;layer.native_header[1]=1;layer.image_type=2;for(auto at:{0x80,0x84})for(int i=0;i<4;++i)layer.native_header[at+i]=255;layer.native_header[0x30]=layer.native_header[0x31]=layer.native_header[0x32]=255;}
            if(texts.count(id)){auto& box=texts.at(id);auto put=[&](size_t at,uint32_t value){for(int i=0;i<4;++i)layer.native_header[at+i]=uint8_t(value>>(8*i));};if(configuration.count("GraphicTextNoWait:"+std::to_string(id)))layer.native_header[0x114]=number(configuration.at("GraphicTextNoWait:"+std::to_string(id))[1])!=0;put(0x2c,uint32_t(box.font));put(0x30,uint32_t(box.style.color.r)|(uint32_t(box.style.color.g)<<8)|(uint32_t(box.style.color.b)<<16));put(0x34,uint32_t(text_model->layer(id).flow.x));put(0x38,uint32_t(text_model->layer(id).flow.y));put(0x134,uint32_t(text_model->layer(id).flow.anchor_x));put(0x138,uint32_t(text_model->layer(id).flow.anchor_y));put(0x3c,uint32_t(box.x));put(0x40,uint32_t(box.y));put(0x44,uint32_t(box.x+box.w));put(0x48,uint32_t(box.y+box.h));}
            OriginalLayerState state=decode_original_layer_state(layer.native_header);state.image_type=l.image_type;state.x=l.x;state.y=l.y;state.visible=l.visible;state.transmittance=int(l.transmittance);if(layer_motion.count(id))state.capture_motion(layer_motion.at(id));state.x=l.x;state.y=l.y;state.visible=l.visible;
            if(auto fade=motions.find({id,false});fade!=motions.end())state.transmittance=int(fade->second.alpha_to);
            for(const auto& [name,args]:configuration){if(args.empty()||!std::holds_alternative<int32_t>(args[0])||number(args[0])!=id)continue;Args a{args};auto command=name.substr(0,name.find(':'));if(a.size()>1){if(command=="GraphicAddColor")state.add_color=uint32_t(a.n(1));else if(command=="GraphicSubColor")state.sub_color=uint32_t(a.n(1));else if(command=="GraphicSepia")state.sepia=a.n(1)!=0;else if(command=="GraphicMonochrome")state.monochrome=a.n(1)!=0;else if(command=="GraphicNegaposi")state.negative=a.n(1)!=0;}if(a.size()>2){if(command=="GraphicScaleCenter"){state.scale_center_x=a.n(1);state.scale_center_y=a.n(2);}else if(command=="GraphicRotateCenter"){state.rotate_center_x=a.n(1);state.rotate_center_y=a.n(2);}}}
            layer.image_type=l.image_type;layer.mask=l.mask;layer.transmittance=state.transmittance;layer.native_header=encode_original_layer_state(state,layer.native_header);
            result.layers.push_back(std::move(layer));
        }
        for(const auto& [id,p]:sounds)if(!p.first.empty()&&audio.loaded(id)&&!audio.one_only(id)){OriginalSound sound;sound.id=id;sound.resource=p.first;sound.loop=audio.loop(id);sound.playing=audio.playing(id);sound.volume=audio.volume(id);result.sounds.push_back(std::move(sound));}
        return result;
    }
    void restore_native_scene(const OriginalScene& saved) {
        const auto retained_motion=layer_motion;std::map<int,double> retained_alpha;for(int id:engine_config.save_data_without_graphic_transmittance)retained_alpha[id]=layers.count(id)?layers.at(id).transmittance:0;
        reveal_bytes.clear();pending_text.clear();pending_text_index=0;{std::vector<int> ids;for(const auto& p:text_model->layers())ids.push_back(p.first);for(int id:ids)text_model->erase_layer(id);}inline_text_images.clear();scene.clear();key_wait_icon.reset();scene.clear_prefix_cache();if(mode_visuals)mode_visuals->clear_icon_instances();mode_icon_sprites={};layers.clear();layer_motion.clear();modified_pixels.clear();particle_scene_ids.clear();particles.clear();sprites.clear();texts.clear();motions.clear();previous={};transition_end=effect_end=0;audio.clear();audio_fades.clear();audio_ends.clear();sounds.clear();native_layers.clear();
        for(const auto& layer:saved.layers){if(layer.resource.empty()){if(layer.image_type==0)scene.create_canvas(layer.id,scene.width(),scene.height(),layer.x,layer.y);else scene.create_filter(layer.id,layer.image_type>=10001&&layer.image_type<=10004?layer.image_type-10000:4);scene.set_z(layer.id,layer.id*10000);layers[layer.id]={"",layer.x,layer.y,255,layer.visible,layer.image_type,{}};}else load_layer(layer.id,layer.resource,layer.x,layer.y,layer.visible,layer.image_type,layer.mask.empty()?Bytes{}:read_resource(layer.mask));auto& l=layers.at(layer.id);l.mask=layer.mask;l.transmittance=retained_alpha.count(layer.id)?retained_alpha.at(layer.id):layer.transmittance;l.alpha=255-int(std::clamp(l.transmittance,0.,1000.)*255/1000);scene.set_alpha(layer.id,l.alpha);native_layers[layer.id]=layer;
            auto state=decode_original_layer_state(layer.native_header);if(retained_motion.count(layer.id))motion_for(layer.id)=retained_motion.at(layer.id);state.apply_motion(motion_for(layer.id));apply_motion(layer.id);scene.set_scale_center(layer.id,state.scale_center_x,state.scale_center_y);scene.set_rotate_center(layer.id,state.rotate_center_x,state.rotate_center_y);if(!layer.resource.empty()){scene.set_add_color(layer.id,state.add_color);scene.set_sub_color(layer.id,state.sub_color);scene.set_sepia(layer.id,state.sepia==1);scene.set_monochrome(layer.id,state.monochrome==1);scene.set_negaposi(layer.id,state.negative==1);}
            const int32_t id=layer.id;configuration["GraphicAddColor:"+std::to_string(id)]={id,int32_t(state.add_color)};configuration["GraphicSubColor:"+std::to_string(id)]={id,int32_t(state.sub_color)};configuration["GraphicSepia:"+std::to_string(id)]={id,int32_t(state.sepia==1)};configuration["GraphicMonochrome:"+std::to_string(id)]={id,int32_t(state.monochrome==1)};configuration["GraphicNegaposi:"+std::to_string(id)]={id,int32_t(state.negative==1)};configuration["GraphicScaleCenter:"+std::to_string(id)]={id,state.scale_center_x,state.scale_center_y};configuration["GraphicRotateCenter:"+std::to_string(id)]={id,state.rotate_center_x,state.rotate_center_y};
            if(layer.native_header.size()>=0x50){TextBox box;box.font=int(u32(layer.native_header,0x2c));box.x=int(u32(layer.native_header,0x3c));box.y=int(u32(layer.native_header,0x40));box.w=std::max(1,int(u32(layer.native_header,0x44))-box.x);box.h=std::max(1,int(u32(layer.native_header,0x48))>box.y?int(u32(layer.native_header,0x48))-box.y:int(u32(layer.native_header,0x48)));auto color=u32(layer.native_header,0x30);box.style.color={uint8_t(color),uint8_t(color>>8),uint8_t(color>>16),255};texts[layer.id]=box;}
        }
        for(const auto& layer:saved.layers){
            auto& box=texts[layer.id];sync_text_layer(layer.id,true);auto& model=text_model->layer(layer.id);
            if(!text_model->fonts().count(box.font)){auto style=fonts.count(box.font)?fonts.at(box.font):box.style;text_model->call("DefaultFontSet",{int32_t(box.font),std::string{},int32_t(style.font_size),int32_t(400),int32_t(style.font_size+style.letter_spacing),int32_t(style.line_height?style.line_height:style.font_size+5)});}
            if(layer.native_header.size()>=0x140){configuration["GraphicTextNoWait:"+std::to_string(layer.id)]={int32_t(layer.id),int32_t(layer.native_header[0x114]!=0)};model.flow.x=int32_t(u32(layer.native_header,0x34));model.flow.y=int32_t(u32(layer.native_header,0x38));model.flow.anchor_x=int32_t(u32(layer.native_header,0x134));model.flow.anchor_y=int32_t(u32(layer.native_header,0x138));}
            for(const auto& b:layer.sprites){if(b.kind==1&&b.named&&layers.count(b.source_layer)){create_sprite(layer.id,b.slot,b.x,b.y,b.source_layer,b.group,b.visible);auto& target=sprites.at({layer.id,b.slot});target.frame=std::min<size_t>(std::max(0,b.frame),group_for(target).size()-1);apply_frame(target);}
                else if(b.kind==3||b.kind==4||b.kind==5||b.kind==7){TextRecord rec;rec.kind=b.kind;rec.x=b.x;rec.y=b.y;rec.width=b.width;rec.height=b.height;rec.font=b.font;rec.image_id=b.group;if(b.kind==3){if(b.encoded_character){rec.encoded+=char(b.encoded_character&255);if(b.encoded_character>>8)rec.encoded+=char(b.encoded_character>>8);}else rec.encoded.assign(b.payload.begin(),b.payload.end());}auto style=current_text_style(layer.id,b.font);if(style.native_font&&b.kind==3){auto& font=*style.native_font;font.inherit_effect=b.font_effect<1;font.inherit_border=b.border_color==0xffffffffU;font.inherit_shadow=b.shadow_color==0xffffffffU;if(!font.inherit_effect)font.effect=b.font_effect;if(!font.inherit_border)font.border_color=b.border_color;if(!font.inherit_shadow)font.shadow_color=b.shadow_color;}style.color={uint8_t(b.color),uint8_t(b.color>>8),uint8_t(b.color>>16),255};rec.style_token=intern_text_style(style);model.records.push_back(std::move(rec));}
            }
            box.content=text_model->str_in(layer.id);if(!model.records.empty())draw_text(layer.id);
        }
        for(const auto& sound:saved.sounds)if(!sound.resource.empty()){audio.load_sources(sound.id,open_audio_parts(sound.resource));sounds[sound.id]={sound.resource,sound.loop};audio.set_volume(sound.id,std::clamp(sound.volume,0,1000));channel_volumes[sound.id]=std::clamp(sound.volume,0,1000);if(sound.playing){audio.play(sound.id,sound.loop);if(!sound.loop)audio_ends[sound.id]=now+audio.duration_ms(sound.id);}}
        text_layer=13;drawing=true;collection=false;regions.clear();hover=-1;wait=Wait::None;
    }
    std::filesystem::path slot_path(int slot) const {
        if(slot<0||slot>999)throw std::runtime_error("invalid save slot");
        std::ostringstream name;name<<"savedata."<<std::setw(3)<<std::setfill('0')<<slot<<".yukari";return state_dir/name.str();
    }
    static void write_values(std::ostream& out,const std::vector<Value>& values) {
        out<<values.size()<<' ';for(const auto& v:values){out<<v.index()<<' ';if(auto n=std::get_if<int32_t>(&v))out<<*n<<' ';else if(auto t=std::get_if<std::string>(&v))out<<std::quoted(*t)<<' ';}
    }
    static std::vector<Value> read_values(std::istream& in) {
        size_t count=0;in>>count;if(count>10000)throw std::runtime_error("invalid saved value count");std::vector<Value> values;
        while(count--){int type=0;in>>type;if(type==0)values.push_back({});else if(type==1){int32_t n;in>>n;values.push_back(n);}else if(type==2){std::string t;in>>std::quoted(t);values.push_back(t);}else throw std::runtime_error("invalid saved value type");}return values;
    }
    static void write_text_style(std::ostream& out,const TextStyle& s){out<<std::quoted(s.font_path.string())<<' '<<s.font_size<<' '<<s.line_height<<' '<<s.letter_spacing<<' ';for(auto c:{s.color,s.border_color,s.shadow_color})out<<unsigned(c.r)<<' '<<unsigned(c.g)<<' '<<unsigned(c.b)<<' '<<unsigned(c.a)<<' ';out<<s.border_width<<' '<<s.shadow_offset.x<<' '<<s.shadow_offset.y<<' '<<bool(s.native_font);if(s.native_font){const auto& f=*s.native_font;out<<' '<<std::quoted(f.family)<<' '<<f.height<<' '<<f.width<<' '<<f.weight<<' '<<f.charset<<' '<<f.quality<<' '<<f.effect<<' '<<f.border_thickness<<' '<<f.shadow_thickness<<' '<<f.border_color<<' '<<f.shadow_color<<' '<<f.inherit_effect<<' '<<f.inherit_border<<' '<<f.inherit_shadow;}}
    static TextStyle read_text_style(std::istream& in,bool modern){TextStyle s;std::string path;in>>std::quoted(path)>>s.font_size>>s.line_height>>s.letter_spacing;s.font_path=path;for(auto c:{&s.color,&s.border_color,&s.shadow_color}){unsigned r,g,b,a;in>>r>>g>>b>>a;if(r>255||g>255||b>255||a>255)throw std::runtime_error("invalid saved text color");*c={uint8_t(r),uint8_t(g),uint8_t(b),uint8_t(a)};}in>>s.border_width>>s.shadow_offset.x>>s.shadow_offset.y;if(modern){bool native;in>>native;if(native){NativeFontSpec f;in>>std::quoted(f.family)>>f.height>>f.width>>f.weight>>f.charset>>f.quality>>f.effect>>f.border_thickness>>f.shadow_thickness>>f.border_color>>f.shadow_color>>f.inherit_effect>>f.inherit_border>>f.inherit_shadow;s.native_font=f;}}return s;}
    Bytes runtime_state() const {
        std::ostringstream out;out<<std::setprecision(17)<<"YUKARI_RUNTIME_9 "<<text_layer<<' '<<text_pages<<' '<<int(wait)<<' '<<layers.size()<<'\n';
        for(const auto& [id,l]:layers)out<<id<<' '<<std::quoted(l.asset)<<' '<<l.x<<' '<<l.y<<' '<<l.alpha<<' '<<l.visible<<' '<<l.image_type<<' '<<std::quoted(l.mask)<<' '<<l.transmittance<<'\n';
        out<<sprites.size()<<'\n';for(const auto& [k,b]:sprites)out<<b.parent<<' '<<b.slot<<' '<<b.source<<' '<<b.group<<' '<<b.x<<' '<<b.y<<' '<<b.frame<<' '<<b.done<<' '<<b.visible<<' '<<b.mode<<'\n';
        out<<texts.size()<<'\n';for(const auto& [id,b]:texts)out<<id<<' '<<b.x<<' '<<b.y<<' '<<b.w<<' '<<b.h<<' '<<b.font<<' '<<std::quoted(b.content)<<' '<<unsigned(b.style.color.r)<<' '<<unsigned(b.style.color.g)<<' '<<unsigned(b.style.color.b)<<'\n';
        out<<fonts.size()<<'\n';for(const auto& [id,f]:fonts)out<<id<<' '<<f.font_size<<' '<<f.line_height<<' '<<f.letter_spacing<<'\n';
        out<<configuration.size()<<'\n';for(const auto& [name,v]:configuration){out<<std::quoted(name)<<' ';write_values(out,v);out<<'\n';}
        size_t sound_count=0;for(const auto& item:sounds)if(audio.loaded(item.first))++sound_count;out<<sound_count<<'\n';for(const auto& [id,p]:sounds)if(audio.loaded(id))out<<id<<' '<<std::quoted(p.first)<<' '<<audio.loop(id)<<' '<<audio.playing(id)<<' '<<audio.volume(id)<<'\n';
        out<<history.size()<<'\n';for(const auto& entry:history)out<<std::quoted(entry.text)<<' '<<std::quoted(entry.voice)<<' '<<std::quoted(std::string(entry.native.begin(),entry.native.end()))<<'\n';
        out<<std::quoted(window_title)<<'\n';
        out<<text_selection<<' '<<timed_selection<<' '<<select_layer<<' '<<select_x<<' '<<select_y<<' '<<select_width<<' '<<select_height<<' '<<select_mode<<' '<<select_normal<<' '<<select_hover<<' '<<select_read_normal<<' '<<select_read_hover<<' '<<(now>=select_start?now-select_start:0)<<' '<<select_limit<<' '<<selection_elapsed<<' '<<select_end<<' '<<std::quoted(selection_script)<<' '<<movie_volume<<'\n';
        out<<regions.size()<<'\n';for(const auto& r:regions){out<<std::quoted(r.kind)<<' '<<r.branch<<' '<<r.end<<' '<<r.fired<<' '<<r.fired_at<<' ';write_values(out,r.args);out<<'\n';}
        auto clocks=services.state();out<<clocks.play_seconds<<' '<<clocks.play_started<<' ';for(auto t:clocks.timers)out<<t<<' ';out<<now<<'\n';
        auto playback=audio.snapshot();out<<std::quoted(std::string(playback.begin(),playback.end()))<<'\n';
        out<<auto_mode<<' '<<skip_mode<<' '<<already_mode<<' '<<ordinary_skip_mode<<'\n';
        out<<settings.state().values.size()<<'\n';for(const auto& [k,v]:settings.state().values){out<<std::quoted(k)<<' ';write_values(out,{v});out<<'\n';}out<<settings.state().records.size()<<'\n';for(const auto& [k,v]:settings.state().records){out<<std::quoted(k)<<' ';write_values(out,v);out<<'\n';}
        auto model=text_model->snapshot();out<<std::quoted(std::string(model.begin(),model.end()))<<'\n';
        out<<text_styles.size()<<'\n';for(const auto& [id,style]:text_styles){out<<id<<' ';write_text_style(out,style);out<<'\n';}
        out<<next_text_style<<' '<<pending_text_index<<' '<<pending_text_slow<<' '<<(reveal_next>now?reveal_next-now:0)<<' '<<std::quoted(pending_text_location.script)<<' '<<pending_text_location.word<<' '<<pending_text.size()<<'\n';for(const auto& p:pending_text)out<<p.id<<' '<<p.offset<<' '<<std::quoted(p.encoded)<<'\n';out<<hidden_text_layers.size()<<' ';for(int id:hidden_text_layers)out<<id<<' ';out<<'\n';
        out<<layer_motion.size()<<'\n';for(const auto& [id,m]:layer_motion){auto b=m.snapshot();out<<id<<' '<<std::quoted(std::string(b.begin(),b.end()))<<'\n';}
        out<<motions.size()<<'\n';for(const auto& [k,m]:motions)out<<m.id<<' '<<m.x<<' '<<m.y<<' '<<m.end_x<<' '<<m.end_y<<' '<<m.alpha<<' '<<m.end_alpha<<' '<<m.start<<' '<<m.end<<' '<<m.position<<' '<<m.alpha_from<<' '<<m.alpha_to<<'\n';out<<now<<'\n';
        auto particle_state=particles.snapshot();out<<std::quoted(std::string(particle_state.begin(),particle_state.end()))<<'\n';
        out<<modified_pixels.size()<<'\n';for(int id:modified_pixels){auto b=scene.layer_pixels(id);out<<id<<' '<<std::quoted(std::string(b.begin(),b.end()))<<'\n';}
        out<<random<<' '<<cursor_auto_hide<<' '<<load_to_draw_stop<<' '<<movie_attenuation<<'\n';for(int vk:key_history)out<<vk<<' ';out<<'\n';out<<key_wait_tte_mode<<' '<<(now-key_wait_started)<<' '<<selection_tte_loop<<'\n';out<<bool(key_wait_icon)<<' ';if(key_wait_icon){const auto& icon=*key_wait_icon;out<<key_wait_icon_parent<<' '<<key_wait_icon_x<<' '<<key_wait_icon_y<<' '<<icon.group<<' '<<icon.frame<<' '<<icon.mode<<' '<<icon.done<<' '<<(icon.next>now?icon.next-now:0);}out<<'\n';
        if(mode_visuals){const auto& visual=mode_visuals->state();for(const auto& icon:visual.icons)out<<icon.parent<<' '<<icon.x<<' '<<icon.y<<' '<<icon.source<<' '<<icon.group<<' ';out<<'\n';for(const auto& graphic:visual.graphics)out<<graphic.enabled<<' '<<graphic.applied<<' '<<graphic.layer<<' '<<std::quoted(graphic.replacement)<<' '<<std::quoted(graphic.backup)<<'\n';}
        if(cd_commands){const auto& cd=cd_commands->state();out<<cd.script_track<<' '<<cd.device_track<<' '<<cd.script_loop<<' '<<cd.device_loop<<' '<<cd.silent<<' '<<cd.name_attempted<<' '<<cd.track_epoch<<' '<<cd.playback_epoch<<'\n';}else out<<"0 0 0 0 0 0 0 0\n";out<<key_wait_visibility.size()<<' ';for(const auto& [id,visible]:key_wait_visibility)out<<id<<' '<<visible<<' ';out<<'\n';
        auto data=out.str();return Bytes(data.begin(),data.end());
    }
    void restore_runtime(const Bytes& bytes) {
        close_history(false,false);
        std::istringstream in(std::string(bytes.begin(),bytes.end()));std::string magic;int saved_wait;size_t count;
        in>>magic>>text_layer>>text_pages>>saved_wait>>count;if((magic!="YUKARI_RUNTIME_1"&&magic!="YUKARI_RUNTIME_2"&&magic!="YUKARI_RUNTIME_3"&&magic!="YUKARI_RUNTIME_4"&&magic!="YUKARI_RUNTIME_5"&&magic!="YUKARI_RUNTIME_6"&&magic!="YUKARI_RUNTIME_7"&&magic!="YUKARI_RUNTIME_8"&&magic!="YUKARI_RUNTIME_9")||count>50000)throw std::runtime_error("invalid runtime save header");
        clear_choices();clear_selector();movie.reset();history_mode=false;reveal_bytes.clear();pending_text.clear();pending_text_index=0;{std::vector<int> ids;for(const auto& p:text_model->layers())ids.push_back(p.first);for(int id:ids)text_model->erase_layer(id);}inline_text_images.clear();scene.clear();key_wait_icon.reset();scene.clear_prefix_cache();if(mode_visuals)mode_visuals->clear_icon_instances();mode_icon_sprites={};layers.clear();layer_motion.clear();modified_pixels.clear();particle_scene_ids.clear();particles.clear();sprites.clear();texts.clear();motions.clear();previous={};transition_end=effect_end=0;audio.clear();audio_fades.clear();sounds.clear();audio_ends.clear();channel_volumes.clear();group_volumes.clear();native_layers.clear();regions.clear();
        while(count--){int id;Layer l;in>>id>>std::quoted(l.asset)>>l.x>>l.y>>l.alpha>>l.visible;if(magic=="YUKARI_RUNTIME_5"||magic=="YUKARI_RUNTIME_6"||magic=="YUKARI_RUNTIME_7"||(magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9"))in>>l.image_type>>std::quoted(l.mask);if((magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9"))in>>l.transmittance;else l.transmittance=double(255-l.alpha)*1000/255;if(l.asset.empty()){if(l.image_type==0)scene.create_canvas(id,scene.width(),scene.height(),l.x,l.y);else scene.create_filter(id,l.image_type>=10001&&l.image_type<=10004?l.image_type-10000:4);scene.set_z(id,id*10000);layers[id]=l;}else load_layer(id,l.asset,l.x,l.y,l.visible,l.image_type,l.mask.empty()?Bytes{}:resources.read(l.mask));layers[id].mask=l.mask;layers[id].alpha=l.alpha;layers[id].transmittance=l.transmittance;scene.set_alpha(id,l.alpha);}
        in>>count;if(count>100000)throw std::runtime_error("invalid saved sprite count");
        std::vector<Sprite> saved_sprites;
        while(count--){Sprite b;in>>b.parent>>b.slot>>b.source>>b.group>>b.x>>b.y>>b.frame>>b.done>>b.visible>>b.mode;saved_sprites.push_back(b);}
        in>>count;if(count>50000)throw std::runtime_error("invalid saved text count");
        while(count--){int id,r,g,b;TextBox box;in>>id>>box.x>>box.y>>box.w>>box.h>>box.font>>std::quoted(box.content)>>r>>g>>b;box.style.color={uint8_t(r),uint8_t(g),uint8_t(b),255};texts[id]=box;}
        fonts.clear();in>>count;if(count>10000)throw std::runtime_error("invalid saved font count");while(count--){int id;TextStyle f;in>>id>>f.font_size>>f.line_height>>f.letter_spacing;fonts[id]=f;}
        configuration.clear();in>>count;if(count>100000)throw std::runtime_error("invalid saved configuration count");while(count--){std::string name;in>>std::quoted(name);configuration[name]=read_values(in);}
        in>>count;if(count>10000)throw std::runtime_error("invalid saved audio count");while(count--){int id;bool loop,playing;std::string name;in>>id>>std::quoted(name)>>loop>>playing;int volume=1000;if(magic=="YUKARI_RUNTIME_3"||magic=="YUKARI_RUNTIME_4"||magic=="YUKARI_RUNTIME_5"||magic=="YUKARI_RUNTIME_6"||magic=="YUKARI_RUNTIME_7"||(magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9"))in>>volume;channel_volumes[id]=volume;if(!name.empty()){sounds[id]={name,loop};if(magic!="YUKARI_RUNTIME_5"&&magic!="YUKARI_RUNTIME_6"&&magic!="YUKARI_RUNTIME_7"&&magic!="YUKARI_RUNTIME_8"&&magic!="YUKARI_RUNTIME_9"){std::vector<ResourceSource> parts;std::istringstream names(name);std::string part;while(std::getline(names,part,'\t'))parts.push_back(open_resource(part));audio.load_sources(id,parts);audio.set_volume(id,volume);if(playing){audio.play(id,loop);if(!loop)audio_ends[id]=now+audio.duration_ms(id);}}}}
        history.clear();if(magic=="YUKARI_RUNTIME_2"||magic=="YUKARI_RUNTIME_3"||magic=="YUKARI_RUNTIME_4"||magic=="YUKARI_RUNTIME_5"||magic=="YUKARI_RUNTIME_6"||magic=="YUKARI_RUNTIME_7"||(magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9")){in>>count;if(count>1000)throw std::runtime_error("invalid saved history count");while(count--){HistoryEntry entry;std::string data;in>>std::quoted(entry.text)>>std::quoted(entry.voice)>>std::quoted(data);entry.native=Bytes(data.begin(),data.end());history.push_back(std::move(entry));}}
        if(magic=="YUKARI_RUNTIME_4"||magic=="YUKARI_RUNTIME_5"||magic=="YUKARI_RUNTIME_6"||magic=="YUKARI_RUNTIME_7"||(magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9")){in>>std::quoted(window_title);scene.set_title(window_title);}
        if(magic=="YUKARI_RUNTIME_5"||magic=="YUKARI_RUNTIME_6"||magic=="YUKARI_RUNTIME_7"||(magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9")){uint64_t elapsed;in>>text_selection>>timed_selection>>select_layer>>select_x>>select_y>>select_width>>select_height>>select_mode>>select_normal>>select_hover>>select_read_normal>>select_read_hover>>elapsed>>select_limit>>selection_elapsed>>select_end>>std::quoted(selection_script)>>movie_volume;select_start=now-elapsed;in>>count;if(count>10000)throw std::runtime_error("invalid saved selection region count");while(count--){Region r;in>>std::quoted(r.kind)>>r.branch>>r.end>>r.fired>>r.fired_at;r.args=read_values(in);regions.push_back(std::move(r));}ServiceState clocks;in>>clocks.play_seconds>>clocks.play_started;for(auto& t:clocks.timers)in>>t;uint64_t saved_now;in>>saved_now;uint32_t shift=uint32_t(now-saved_now);for(auto& t:clocks.timers)if(t)t+=shift;clocks.play_started+=shift;services.restore(clocks);std::string playback;in>>std::quoted(playback);{Bytes state(playback.begin(),playback.end());for(const auto& source:Audio::sources_from_snapshot(state))if(sounds.count(source.id)&&(!audio.loaded(source.id)||audio.streamed(source.id)!=source.streamed||(source.streamed&&audio.stream_loop_start(source.id)!=source.loop_start))){std::vector<ResourceSource> parts;std::istringstream names(sounds.at(source.id).first);std::string part;while(std::getline(names,part,'\t'))parts.push_back(open_resource(part));audio.load_sources(source.id,parts,source.streamed,source.loop_start);}audio.restore_playback(state);}in>>auto_mode>>skip_mode>>already_mode>>ordinary_skip_mode;SettingsState state;in>>count;if(count>10000)throw std::runtime_error("invalid setting count");while(count--){std::string k;in>>std::quoted(k);auto v=read_values(in);if(v.size()!=1)throw std::runtime_error("invalid setting scalar");state.values[k]=v[0];}in>>count;if(count>10000)throw std::runtime_error("invalid setting records");while(count--){std::string k;in>>std::quoted(k);state.records[k]=read_values(in);}settings.restore(std::move(state));}
        if(magic=="YUKARI_RUNTIME_6"||magic=="YUKARI_RUNTIME_7"||(magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9")){
            std::string encoded;in>>std::quoted(encoded);text_model->restore(Bytes(encoded.begin(),encoded.end()));const auto saved_text_encoding=text_model->encoding();text_model->set_encoding(original_encoding);text_styles.clear();in>>count;if(count>1000000)throw std::runtime_error("invalid saved text style count");while(count--){uint64_t id;in>>id;text_styles[id]=read_text_style(in,magic=="YUKARI_RUNTIME_9");}uint64_t remaining;in>>next_text_style>>pending_text_index>>pending_text_slow>>remaining>>std::quoted(pending_text_location.script)>>pending_text_location.word>>count;if(count>10000)throw std::runtime_error("invalid pending text target count");pending_text.clear();while(count--){PendingText p;in>>p.id>>p.offset>>std::quoted(p.encoded);if(p.offset>p.encoded.size())throw std::runtime_error("invalid pending text offset");if(saved_text_encoding!=original_encoding&&!p.encoded.empty()){const auto prefix=p.encoded.substr(0,p.offset),suffix=p.encoded.substr(p.offset);auto converted_prefix=encode_original_text(decode(prefix,saved_text_encoding.c_str()),original_encoding.c_str());p.encoded=converted_prefix+encode_original_text(decode(suffix,saved_text_encoding.c_str()),original_encoding.c_str());p.offset=converted_prefix.size();}pending_text.push_back(std::move(p));}if(pending_text_index>pending_text.size())throw std::runtime_error("invalid pending text index");reveal_next=now+remaining;hidden_text_layers.clear();in>>count;if(count>10000)throw std::runtime_error("invalid hidden text count");while(count--){int id;in>>id;hidden_text_layers.push_back(id);}
            for(const auto& [id,f]:text_model->fonts())fonts[id]=logical_font_style(f);
        }else{
            for(const auto& [id,f]:fonts)text_model->call("DefaultFontSet",{int32_t(id),std::string{},int32_t(f.font_size),int32_t(400),int32_t(f.font_size+f.letter_spacing),int32_t(f.line_height?f.line_height:f.font_size+5)});
            for(auto& [id,b]:texts){sync_text_layer(id,true);auto content=encode_original_text(b.content,original_encoding.c_str());auto& l=text_model->layer(id);l.style_token=intern_text_style(current_text_style(id));for(size_t at=0;at<content.size();){if(content[at]=='\n'){text_model->line_feed(id);++at;continue;}auto n=text_model->output_step(id,content.substr(at));if(!n){l.output_prepared=false;l.pending_output.clear();break;}at+=n;}}
        }
        std::map<int,MotionModel> saved_motion;std::map<int,Bytes> saved_pixels;
        if(magic=="YUKARI_RUNTIME_7"||(magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9")){
            in>>count;if(count>50000)throw std::runtime_error("invalid saved motion count");while(count--){int id;std::string b;in>>id>>std::quoted(b);saved_motion[id].restore(Bytes(b.begin(),b.end()));}
            in>>count;if(count>100000)throw std::runtime_error("invalid saved alpha motion count");while(count--){Motion m;in>>m.id>>m.x>>m.y>>m.end_x>>m.end_y>>m.alpha>>m.end_alpha>>m.start>>m.end>>m.position;if((magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9"))in>>m.alpha_from>>m.alpha_to;else{m.alpha_from=double(255-m.alpha)*1000/255;m.alpha_to=double(255-m.end_alpha)*1000/255;}motions[{m.id,m.position}]=m;}uint64_t saved_clock;in>>saved_clock;for(auto& [k,m]:motions){m.start+=now-saved_clock;m.end+=now-saved_clock;}
            std::string p;in>>std::quoted(p);particles.restore(Bytes(p.begin(),p.end()),[this](const std::string& name){return asset(name).groups;});
            in>>count;if(count>50000)throw std::runtime_error("invalid modified image count");while(count--){int id;std::string b;in>>id>>std::quoted(b);saved_pixels[id]=Bytes(b.begin(),b.end());}
            in>>random>>cursor_auto_hide>>load_to_draw_stop;if((magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9"))in>>movie_attenuation;for(auto& vk:key_history)in>>vk;if((magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9")){uint64_t elapsed_key_wait;in>>key_wait_tte_mode>>elapsed_key_wait>>selection_tte_loop;key_wait_started=now-elapsed_key_wait;bool has_icon;in>>has_icon;key_wait_icon.reset();if(has_icon){SpriteMotion icon;uint64_t remaining;in>>key_wait_icon_parent>>key_wait_icon_x>>key_wait_icon_y>>icon.group>>icon.frame>>icon.mode>>icon.done>>remaining;icon.next=now+remaining;key_wait_icon=icon;auto& image=asset("system.ykg");scene.load_cached(17000000,"system.ykg",image.bytes);draw_key_wait_icon();}ModeVisualState visual;for(auto& icon:visual.icons)in>>icon.parent>>icon.x>>icon.y>>icon.source>>icon.group;for(auto& graphic:visual.graphics)in>>graphic.enabled>>graphic.applied>>graphic.layer>>std::quoted(graphic.replacement)>>std::quoted(graphic.backup);if(mode_visuals)mode_visuals->restore(std::move(visual));CdState cd;in>>cd.script_track>>cd.device_track>>cd.script_loop>>cd.device_loop>>cd.silent>>cd.name_attempted>>cd.track_epoch>>cd.playback_epoch;uint32_t shift=uint32_t(now-saved_clock);if(cd.track_epoch)cd.track_epoch+=shift;if(cd.playback_epoch)cd.playback_epoch+=shift;if(cd_commands)cd_commands->restore(cd);}in>>count;if(count>32)throw std::runtime_error("invalid saved key-wait list");key_wait_visibility.clear();while(count--){int id;bool visible;in>>id>>visible;key_wait_visibility[id]=visible;}if(platform)platform->call("CursorAutoHide",{cursor_auto_hide});
        }
        if(!in)throw std::runtime_error("truncated runtime save");
        if(magic!="YUKARI_RUNTIME_5"&&magic!="YUKARI_RUNTIME_6"&&magic!="YUKARI_RUNTIME_7"&&magic!="YUKARI_RUNTIME_8"&&magic!="YUKARI_RUNTIME_9")for(const auto& [name,v]:configuration){Args a{v};if(name.rfind("SoundGroupSet:",0)==0&&a.size()>=3)audio.set_group(a.n(0),a.n(1),a.n(2));else if(name.rfind("SoundGroupVolumeSet:",0)==0&&a.size()>=2){group_volumes[a.n(0)]=a.n(1);audio.set_group_volume(a.n(0),a.n(1));}}
        if(magic!="YUKARI_RUNTIME_5"&&magic!="YUKARI_RUNTIME_6"&&magic!="YUKARI_RUNTIME_7"&&magic!="YUKARI_RUNTIME_8"&&magic!="YUKARI_RUNTIME_9")for(const auto& [name,v]:configuration){auto n=name.substr(0,name.find(':'));if(n!="ScriptSkipMode"&&n!="SubMenuEnableMode")settings.call(n,v);areas.call(n,v);}
        areas.restore(AreaState{});for(const auto& [name,v]:configuration)areas.call(name.substr(0,name.find(':')),v);
        auto graphic_settings=configuration;
        for(const auto& [name,args]:graphic_settings){auto colon=name.find(':');auto command=name.substr(0,colon);if(command=="GraphicAddColor"||command=="GraphicSubColor"||command=="GraphicSepia"||command=="GraphicMonochrome"||command=="GraphicNegaposi"||command=="GraphicScale"||command=="GraphicScaleCenter"||command=="GraphicRotate"||command=="GraphicRotateCenter")host(HostCall{command,{},args,{},{}});}
        if(magic=="YUKARI_RUNTIME_7"||(magic=="YUKARI_RUNTIME_8"||magic=="YUKARI_RUNTIME_9")){layer_motion=std::move(saved_motion);for(const auto& [id,m]:layer_motion)apply_motion(id);for(const auto& [id,b]:saved_pixels){scene.restore_layer_pixels(id,b);modified_pixels.insert(id);}motion_tick=now;draw_particles();}
        for(const auto& b:saved_sprites){create_sprite(b.parent,b.slot,b.x,b.y,b.source,b.group,b.visible);auto& target=sprites.at({b.parent,b.slot});target.frame=b.frame;target.done=b.done;target.mode=b.mode;apply_frame(target);}
        for(auto& [id,b]:texts)draw_text(id);
        if(saved_wait<0||saved_wait>int(Wait::TextHidden))throw std::runtime_error("invalid saved runtime wait kind");wait=Wait(saved_wait);drawing=true;collection=false;hover=-1;pending_subcall=vm.pending_subcall();selection_checkpoint=false;selection_event=false;if(wait==Wait::Selection){if(text_selection)draw_choices();rebuild_selector();}
    }
    void write_state_file(const std::filesystem::path& path,const Bytes& bytes){std::filesystem::create_directories(state_dir);auto tmp=path;tmp+=".tmp";std::ofstream out(tmp,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));out.close();if(!out)throw std::runtime_error("cannot write state file");std::filesystem::rename(tmp,path);}
    template<class F> auto timed_work(const std::string& name,F&& work){
        if(!frame_profile.active())return work();
        const auto start=std::chrono::steady_clock::now();auto result=work();
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        auto& timing=host_timings[name];++timing.calls;timing.ms+=ms;timing.max_ms=std::max(timing.max_ms,ms);return result;
    }
    void save_slot(int slot,const std::string& comment,const std::string& internal_name={}) {
        close_history(true,false);
        SaveGame game;game.timestamp=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());game.comment=comment;
        const bool parent_checkpoint=in_menu||(internal_name.empty()&&vm.in_subcall()&&!menu_vm.empty());game.thumbnail=timed_work("SaveThumbnail",[&]{return parent_checkpoint?menu_thumbnail:scene.thumbnail();});game.vm=timed_work("SaveVmSnapshot",[&]{return parent_checkpoint?menu_vm:vm.snapshot();});game.runtime=timed_work("SaveRuntimeSnapshot",[&]{return parent_checkpoint?menu_runtime:runtime_state();});
        std::filesystem::create_directories(state_dir);auto data=timed_work("SaveYukariEncode",[&]{return encode_save(game);});auto path=internal_name.empty()?slot_path(slot):state_dir/(internal_name+".yukari"),temp=path;temp+=".tmp";
        std::ofstream out(temp,std::ios::binary);out.write(reinterpret_cast<const char*>(data.data()),std::streamsize(data.size()));out.close();if(!out)throw std::runtime_error("cannot write save slot");std::filesystem::rename(temp,path);
        auto native=parent_checkpoint?menu_native_vm:vm.export_original();
        OriginalSaveWriteOptions options;options.comment1=comment;options.comment2=configuration.count("Index")&&!configuration.at("Index").empty()?text(configuration.at("Index")[0]):"";options.title=window_title;options.encoding=original_encoding.c_str();options.thumbnail_bmp=bmp_thumbnail(game.thumbnail);options.template_bytes=original_template;for(const auto& entry:history)options.history.insert(options.history.end(),entry.native.begin(),entry.native.end());if(slot!=0&&engine_config.save_data_into_select_point_data.value_or(true)&&std::filesystem::exists(native_slot_path(0))){options.previous=read_file(native_slot_path(0));}else if(slot!=0&&original_template.size()>=72){auto at=u32(original_template,0x38),size=u32(original_template,0x3c);require_range(original_template.size(),at,size,"previous choice save");options.previous=Bytes(original_template.begin()+at,original_template.begin()+at+size);}
        auto graphics=parent_checkpoint?menu_native_scene:native_scene();options.append_scene=[&](Bytes& out,uint32_t){encode_original_scene(out,graphics);};
        auto native_bytes=timed_work("SaveNativeEncode",[&]{return encode_original_save(native,options);});auto native_path=internal_name.empty()?native_slot_path(slot):state_dir/internal_name,native_temp=native_path;native_temp+=".tmp";std::ofstream native_out(native_temp,std::ios::binary);native_out.write(reinterpret_cast<const char*>(native_bytes.data()),std::streamsize(native_bytes.size()));native_out.close();if(!native_out)throw std::runtime_error("cannot write original save slot");std::filesystem::rename(native_temp,native_path);
        timed_work("SavePersist",[&]{persist();return 0;});
        std::cout<<"SAVE_READY slot="<<slot<<'\n';
    }
    void start_load_hook(){
        configuration.erase("LoadHookParentWait");const auto found=settings.state().values.find("SaveDataLoadToCallScript");
        if(found==settings.state().values.end())return;const auto script=text(found->second);if(script.empty())return;
        configuration["LoadHookParentWait"]={int32_t(wait)};vm.call(script);wait=Wait::None;
    }
    void load_slot(int slot,bool native_only=false) {
        close_history(false,false);
        if(native_only||!std::filesystem::exists(slot_path(slot))){import_system(!std::filesystem::exists(native_slot_path(slot)));auto bytes=read_file(existing_native_slot(slot));auto state=decode_original_vm(bytes,0,original_encoding.c_str());original_template=bytes;auto graphics=decode_original_scene(bytes);vm.import_original(state);original_state=state;restore_native_scene(graphics);read_history(bytes);in_menu=false;menu_resume_context={};menu_vm.clear();menu_runtime.clear();std::cout<<"LOAD_ORIGINAL_READY slot="<<slot<<'\n';start_load_hook();return;}
        auto game=decode_save(read_file(slot_path(slot)));vm.restore(game.vm);restore_runtime(game.runtime);in_menu=false;menu_resume_context={};menu_vm.clear();menu_runtime.clear();std::cout<<"LOAD_READY slot="<<slot<<'\n';start_load_hook();
    }
    void show_input() {TextStyle style;style.font_size=22;scene.text(9500000,input_value+"|",360,265,440,32,style);scene.set_z(9500000,95000000);drawing=true;}
    void finish_input(bool accept) {if(!accept)input_value=input_original;scene.erase(9500000);SDL_StopTextInput();if(!accept&&texts.count(809))draw_text(809);vm.resume(int32_t(accept));wait=Wait::None;}
    std::optional<HistoryLayer> history_layer(int id)const {
        auto found=layers.find(id);if(found==layers.end())return {};HistoryLayer layer;layer.x=found->second.x;layer.y=found->second.y;
        if(auto box=texts.find(id);box!=texts.end())layer.text_rect={box->second.x,box->second.y,box->second.x+box->second.w,box->second.y+box->second.h};
        else layer.text_rect={0,0,scene.width(),scene.height()};return layer;
    }
    HistoryExtent history_icon_extent(int source,int group){
        if(!layers.count(source)||group<0)return {};auto& image=asset(layers.at(source).asset);if(size_t(group)>=image.groups.size()||image.groups[group].empty())return {};const auto& frame=image.groups[group][0];return {int(frame.w),int(frame.h)};
    }
    void history_sound(int id){if(id>=0&&audio.loaded(id))audio.play(id);}
    void clear_history_draw(){for(int id:history_scene_ids)scene.erase(id);history_scene_ids.clear();}
    void draw_history(){
        if(!history_mode||!history_view)return;clear_history_draw();int next=9000000;
        auto image=[&](int source,int group,int x,int y){if(!layers.count(source)||group<0)return;const auto& source_layer=layers.at(source);auto& data=asset(source_layer.asset);if(size_t(group)>=data.groups.size()||data.groups[group].empty())return;const auto& frame=data.groups[group][0];int id=next++;scene.load_cached(id,key(source_layer.asset),data.bytes,x,y);scene.set_clip(id,{int(frame.x),int(frame.y),int(frame.w),int(frame.h)});scene.set_z(id,90000000+int(history_scene_ids.size()));history_scene_ids.push_back(id);};
        auto options=HistoryOptions::from_settings(settings.state(),text_layer);
        if(options.window_enabled&&layers.count(options.window_layer)){int id=next++;auto& layer=layers.at(options.window_layer);scene.copy(id,options.window_layer,layer.x,layer.y);scene.set_visible(id,true);scene.set_alpha(id,layer.alpha);scene.set_z(id,90000000);history_scene_ids.push_back(id);}
        std::map<int,std::vector<PositionedText>> runs;
        for(const auto& glyph:history_view->layout().glyphs){if(!glyph.visible)continue;if(glyph.glyph.kind==3){auto style=current_text_style(glyph.glyph.layer,glyph.glyph.font_or_image);const auto color=glyph.color;style.color={uint8_t(color),uint8_t(color>>8),uint8_t(color>>16),255};runs[glyph.layer].push_back({glyph.utf8,glyph.x,glyph.y,style});}
            else if(glyph.glyph.kind==4||glyph.glyph.kind==5){const bool original=glyph.glyph.kind==4;const auto& path=original?text_model->original_font_resource():text_model->graphic_font_resource();auto frame=font_image_frame(original,glyph.glyph.font_or_image);int id=next++;scene.load_cached(id,key(path),asset(path).bytes,glyph.x,glyph.y);scene.set_clip(id,{int(frame.x),int(frame.y),int(frame.w),int(frame.h)});if(original)scene.set_glyph_color(id,glyph.color);scene.set_z(id,90000001);history_scene_ids.push_back(id);}}
        for(const auto& [layer,text]:runs){int id=next++;scene.positioned_text(id,text,{0,0,scene.width(),scene.height()});scene.set_z(id,90000001);history_scene_ids.push_back(id);}
        for(const auto& icon:history_view->layout().icons){auto frame=(history_hover==int(10000+icon.page)&&icon.hover_frame)?icon.hover_frame:icon.normal_frame;if(!frame)frame=icon.hover_frame;if(frame)image(icon.source,frame->group,frame->x,frame->y);}
        for(const auto& region:history_view->layout().regions)if(region.sprites){const auto& sprite=*region.sprites;const auto parent=history_layer(sprite.parent);if(!parent)continue;int group=region.disabled?sprite.pressed_frame:history_hover==region.action?sprite.hover_frame:sprite.normal_frame;if(group<0)group=sprite.normal_frame;image(sprite.source,group,parent->x+sprite.x,parent->y+sprite.y);}
        drawing=true;
    }
    void history_hover_at(int x,int y){
        if(!history_mode||!history_view)return;int target=-1;for(auto it=history_view->layout().regions.rbegin();it!=history_view->layout().regions.rend();++it)if(!it->disabled&&x>=it->bounds.left&&x<=it->bounds.right&&y>=it->bounds.top&&y<=it->bounds.bottom){target=it->action;break;}
        if(target!=history_hover){history_hover=target;for(const auto& region:history_view->layout().regions)if(region.action==target&&region.sprites)history_sound(region.sprites->sounds[0]);draw_history();}
    }
    void close_history(bool resume_host=true,bool allow_advance=true){
        if(!history_mode)return;const bool advance_requested=history_view&&history_view->take_advance_request();if(history_view)history_view->close();clear_history_draw();history_mode=false;history_drag=false;history_hover=-1;
        for(const auto& [id,visible]:history_visibility)if(layers.count(id)){layers[id].visible=visible;scene.set_visible(id,visible);update_parent(id);}history_visibility.clear();if(key_wait_icon)draw_key_wait_icon();
        const auto elapsed=now-history_opened;auto_deadline+=elapsed;select_start+=elapsed;key_wait_started+=elapsed;deadline+=elapsed;reveal_next+=elapsed;motion_tick=now;drawing=history_old_drawing;
        if(history_host_wait){history_host_wait=false;if(resume_host){vm.resume();wait=Wait::None;}}
        else if(advance_requested&&allow_advance)advance();
        std::cout<<"HISTORY_CLOSED advance="<<(advance_requested&&allow_advance)<<'\n';
    }
    bool show_history(bool host_call=false){
        if(history_mode)return true;
        if(!history_view){HistoryHooks hooks;hooks.layer=[this](int id){return history_layer(id);};hooks.glyph_extent=[this](const HistoryGlyph& glyph){if(glyph.kind==4||glyph.kind==5){auto frame=font_image_frame(glyph.kind==4,glyph.font_or_image);return HistoryExtent{int(frame.w),int(frame.h)};}std::string raw(1,char(glyph.character&255));if(glyph.character&0xff00)raw+=char((glyph.character>>8)&255);auto size=scene.measure_text(decode(raw,original_encoding.c_str()),current_text_style(glyph.layer,glyph.font_or_image));return HistoryExtent{size.x,size.y};};hooks.icon_extent=[this](int source,int group){return history_icon_extent(source,group);};
            hooks.play_voice=[this](const std::string& name,std::optional<int32_t> channel){int id=channel.value_or(-1);if(!channel){while(audio.loaded(id))--id;}audio.load_sources(id,{open_resource(name)},true);audio.play(id);sounds[id]={name,false};history_voice_channel=id;std::cout<<"HISTORY_VOICE channel="<<id<<'\n';return audio.playing(id);};
            hooks.stop_voice=[this](uint32_t fade){if(history_voice_channel)audio.fade_stop(*history_voice_channel,fade,true);history_voice_channel.reset();};hooks.stop_other_voices=[this](uint32_t fade){const auto group=settings.state().records.find("HistorySoundGroupSet");for(const auto& [id,source]:sounds){(void)source;bool match=false;if(group!=settings.state().records.end())for(const auto& g:group->second){const auto number=::yukari::number(g);if(configuration.count("SoundGroupSet:"+std::to_string(number))){const auto& range=configuration.at("SoundGroupSet:"+std::to_string(number));if(range.size()>2&&id>=::yukari::number(range[1])&&id<=::yukari::number(range[2]))match=true;}}if(match)audio.fade_stop(id,fade);}};
            history_view=std::make_unique<HistoryView>(std::move(hooks));}
        auto options=HistoryOptions::from_settings(settings.state(),text_layer);options.rows_margin=engine_config.history_rows_margin.value_or(30);options.line_margin=engine_config.history_line_margin.value_or(0);options.icon_position_type=engine_config.history_sound_icon_position_type.value_or(9);if(engine_config.history_sound_icon_position){options.icon_x=engine_config.history_sound_icon_position->x;options.icon_y=engine_config.history_sound_icon_position->y;}options.screen_rect=AreaBounds{0,0,scene.width(),scene.height()};options.encoding=original_encoding;
        if(history.empty()&&!options.window_enabled)return false;
        history_visibility.clear();auto visibility=[&](int id,bool visible){if(layers.count(id)){history_visibility.try_emplace(id,layers.at(id).visible);layers[id].visible=visible;scene.set_visible(id,visible);update_parent(id);}};
        if(auto group=settings.state().records.find("DefaultTextWindowGroup");group!=settings.state().records.end())for(const auto& value:group->second)visibility(number(value),false);else visibility(text_layer,false);
        if(options.window_enabled)visibility(options.window_layer,true);
        try{Bytes data;for(const auto& entry:history)data.insert(data.end(),entry.native.begin(),entry.native.end());history_view->open(decode_history_pages(data,original_encoding.c_str()),options,&areas);}catch(...){for(const auto& [id,visible]:history_visibility)if(layers.count(id)){layers[id].visible=visible;scene.set_visible(id,visible);update_parent(id);}history_visibility.clear();throw;}
        if(!history_view->layout().open)return false;
        history_mode=true;history_host_wait=host_call;history_opened=now;history_old_drawing=drawing;history_hover=-1;history_drag=false;auto_mode=skip_mode=ordinary_skip_mode=already_mode=false;if(host_call)wait=Wait::History;if(key_wait_icon)scene.set_visible(17000000,false);draw_history();
        std::cout<<"HISTORY_READY pages="<<history.size()<<" visible="<<history_view->layout().pages.size()<<" first="<<history_view->layout().first<<" last="<<history_view->layout().last_start<<" glyphs="<<history_view->layout().glyphs.size()<<" regions="<<history_view->layout().regions.size()<<'\n';return true;
    }
    void history_action(int label){if(!history_mode||!history_view)return;if(label==7&&history_view->layout().right_exit_sound)history_sound(*history_view->layout().right_exit_sound);for(const auto& region:history_view->layout().regions)if(region.action==label&&region.sprites)history_sound(region.sprites->sounds[1]);history_view->action(label);if(!history_view->layout().open)close_history();else{history_hover=-1;draw_history();}}
    void history_click(int x,int y){if(!history_mode||!history_view)return;for(auto it=history_view->layout().regions.rbegin();it!=history_view->layout().regions.rend();++it)if(!it->disabled&&x>=it->bounds.left&&x<=it->bounds.right&&y>=it->bounds.top&&y<=it->bounds.bottom){if(it->action==13){history_drag=true;history_view->drag_vertical(y);draw_history();}else history_action(it->action);return;}}
    void collect_history(std::vector<int> selected={}) {
        if(in_menu)return;
        if(selected.empty()){auto group=settings.state().records.find("DefaultTextWindowGroup");if(group!=settings.state().records.end())for(const auto& v:group->second)if(auto id=std::get_if<int32_t>(&v))selected.push_back(*id);if(selected.empty())selected.push_back(text_layer);}
        HistoryEntry entry;auto voice=vm.variable("String",20);if(auto value=std::get_if<std::string>(&voice))entry.voice=*value;
        Bytes records;auto word=[&](uint32_t v){for(int i=0;i<4;++i)records.push_back(uint8_t(v>>(8*i)));};uint32_t count=0;bool drawable=false;
        for(int id:selected){if(!text_model->layers().count(id))continue;auto& layer=text_model->layer(id);int x=layer.flow.left,y=layer.flow.top;bool first=true;
            for(auto& r:layer.records){if(r.history_recorded)continue;if(first){if(r.kind>=3&&r.kind<=5){x=r.x;y=r.y;first=false;}else continue;}
                if(r.kind==3&&(r.encoded.empty()||r.encoded.size()>2))continue;if(r.kind!=3&&r.kind!=4&&r.kind!=5&&r.kind!=7)continue;
                r.history_recorded=true;word(uint32_t(id));word(uint32_t(r.kind));word(uint32_t(r.x-x));word(uint32_t(r.y-y));uint32_t character=0;if(r.kind==3){character=uint8_t(r.encoded[0]);if(r.encoded.size()>1)character|=uint32_t(uint8_t(r.encoded[1]))<<8;entry.text+=decode(r.encoded,original_encoding.c_str());}else if(r.kind==7)entry.text+='\n';else entry.text+=(r.kind==4?"@o(":"@g(")+std::to_string(r.image_id)+")";word(character);word(uint32_t(r.kind==4||r.kind==5?r.image_id:r.font));auto style=text_styles.count(r.style_token)?text_styles.at(r.style_token):current_text_style(id,r.font);word(uint32_t(style.color.r)|(uint32_t(style.color.g)<<8)|(uint32_t(style.color.b)<<16));++count;if(r.kind!=7)drawable=true;
            }
        }
        if(!drawable)return;auto prefix=[&](uint32_t v){for(int i=0;i<4;++i)entry.native.push_back(uint8_t(v>>(8*i)));};prefix(count);entry.native.insert(entry.native.end(),records.begin(),records.end());auto encoded=encode_original_text(entry.voice,original_encoding.c_str());prefix(encoded.empty()?0:uint32_t(encoded.size()+1));if(!encoded.empty()){entry.native.insert(entry.native.end(),encoded.begin(),encoded.end());entry.native.push_back(0);}history.push_back(std::move(entry));if(history.size()>1000)history.erase(history.begin());
    }
    void read_history(const Bytes& bytes) {
        history.clear();size_t at=u32(bytes,0x28),end=at+u32(bytes,0x2c);require_range(bytes.size(),at,end-at,"original history");
        while(at<end){const auto begin=at;require_range(end,at,4,"history count");size_t count=u32(bytes,at);at+=4;if(count>100000)throw std::runtime_error("invalid original history count");require_range(end,at,count*28,"history records");HistoryEntry entry;int y=-1;
            for(size_t i=0;i<count;++i){size_t p=at+i*28;if(u32(bytes,p+4)!=3)continue;uint32_t c=u32(bytes,p+16);std::string raw;raw+=char(c&255);if(c>>8)raw+=char((c>>8)&255);auto glyph=decode(raw,original_encoding.c_str());int next_y=int(u32(bytes,p+12));if(y>=0&&next_y>y)entry.text+='\n';entry.text+=glyph;y=next_y;}
            at+=count*28;require_range(end,at,4,"history voice size");size_t size=u32(bytes,at);at+=4;require_range(end,at,size,"history voice");if(size){size_t length=0;while(length<size&&bytes[at+length])++length;entry.voice=decode(std::string(bytes.begin()+at,bytes.begin()+at+length),original_encoding.c_str());}at+=size;entry.native=Bytes(bytes.begin()+begin,bytes.begin()+at);history.push_back(std::move(entry));if(history.size()>1000)throw std::runtime_error("original history exceeds 1000 pages");
        }
    }
    static TextStyle logical_font_style(const TextFont& font){
        TextStyle style;style.font_size=int(std::clamp<int64_t>(std::abs(int64_t(font.height)),1,512));style.line_height=font.line_height;NativeFontSpec native;native.family=font.family;native.height=font.height;native.width=font.width;native.weight=font.weight;style.native_font=native;return style;
    }
    TextStyle current_text_style(int id,int font=-1)const {
        auto box=texts.find(id);TextStyle style=box==texts.end()?TextStyle{}:box->second.style;
        int selected=font>=0?font:(box==texts.end()?0:box->second.font);
        if(fonts.count(selected)){auto base=fonts.at(selected);base.color=style.color;base.border_color=style.border_color;base.shadow_color=style.shadow_color;style=base;}
        auto raw_color=configuration.find("TextColorRaw:"+std::to_string(id));if(raw_color!=configuration.end()&&!raw_color->second.empty()){uint32_t raw=uint32_t(number(raw_color->second[0]));if(raw>0xffffffU&&raw!=0xffffffffU){auto value=vm.variable("GlobalFlag",int32_t(raw&0xffffffU));uint32_t c=uint32_t(number(value));style.color={uint8_t(c),uint8_t(c>>8),uint8_t(c>>16),255};}}
        if(vm.current_already()&&configuration.count("TextAlreadyColor")&&!configuration.at("TextAlreadyColor").empty()){uint32_t c=uint32_t(number(configuration.at("TextAlreadyColor")[0]));if(c!=0xffffffffU)style.color={uint8_t(c),uint8_t(c>>8),uint8_t(c>>16),255};}
        if(configuration.count("DefaultFontEffect")&&!configuration.at("DefaultFontEffect").empty()){int effect=number(configuration.at("DefaultFontEffect")[0]);const int border=std::clamp(engine_config.font_border_thick_size.value_or(2),0,16),shadow=std::clamp(engine_config.font_shadow_thick_size.value_or(2),0,512);style.border_width=effect==3||effect==4?(border+1)/2:0;style.shadow_offset=effect==2||effect==4?SDL_Point{shadow,shadow}:SDL_Point{0,0};}
        for(const auto& item:{std::make_pair("Shadow",&style.shadow_color),std::make_pair("Border",&style.border_color)}){
            auto own=configuration.find(std::string("Text")+item.first+"ColorRaw:"+std::to_string(id));bool inherited=own==configuration.end()||uint32_t(number(own->second[0]))==0xffffffffU;
            auto global=configuration.find(std::string("DefaultText")+item.first+"Color");if(inherited&&global!=configuration.end()){uint32_t c=uint32_t(number(global->second[0]));if(c==0xffffffffU){if(std::string(item.first)=="Shadow")style.shadow_offset={0,0};else style.border_width=0;}else *item.second={uint8_t(c),uint8_t(c>>8),uint8_t(c>>16),255};}
        }
        if(style.native_font){auto& native=*style.native_font;native.effect=configuration.count("DefaultFontEffect")&&!configuration.at("DefaultFontEffect").empty()?font_effect_from_script(number(configuration.at("DefaultFontEffect")[0])):2;native.border_thickness=engine_config.font_border_thick_size.value_or(2);native.shadow_thickness=engine_config.font_shadow_thick_size.value_or(2);
            auto color=[&](const std::string& part){auto own=configuration.find("Text"+part+"ColorRaw:"+std::to_string(id));if(own!=configuration.end()&&!own->second.empty()&&uint32_t(number(own->second[0]))!=0xffffffffU)return uint32_t(number(own->second[0]));auto global=configuration.find("DefaultText"+part+"Color");return global!=configuration.end()&&!global->second.empty()?uint32_t(number(global->second[0])):uint32_t(0);};native.border_color=color("Border");native.shadow_color=color("Shadow");auto inherited=[&](const std::string& part){auto own=configuration.find("Text"+part+"ColorRaw:"+std::to_string(id));return own==configuration.end()||own->second.empty()||uint32_t(number(own->second[0]))==0xffffffffU;};native.inherit_border=inherited("Border");native.inherit_shadow=inherited("Shadow");
        }return style;
    }
    uint64_t intern_text_style(const TextStyle& style){
        auto same_color=[](SDL_Color a,SDL_Color b){return a.r==b.r&&a.g==b.g&&a.b==b.b&&a.a==b.a;};
        for(const auto& [id,s]:text_styles)if(s.font_path==style.font_path&&s.font_size==style.font_size&&s.line_height==style.line_height&&s.letter_spacing==style.letter_spacing&&same_color(s.color,style.color)&&same_color(s.border_color,style.border_color)&&same_color(s.shadow_color,style.shadow_color)&&s.border_width==style.border_width&&s.shadow_offset.x==style.shadow_offset.x&&s.shadow_offset.y==style.shadow_offset.y&&s.native_font==style.native_font)return id;
        auto id=next_text_style++;text_styles[id]=style;return id;
    }
    void sync_text_layer(int id,bool reset=false){
        auto& box=texts[id];auto& l=text_model->layer(id);l.font=box.font;
        auto& f=l.flow;f.left=box.x;f.top=box.y;f.right=box.x+box.w;f.bottom=box.y+box.h;
        if(reset)text_model->reset_cursor(id);
    }
    const Frame& font_image_frame(bool original,int image){
        const auto& path=original?text_model->original_font_resource():text_model->graphic_font_resource();if(path.empty())throw std::runtime_error("inline font resource is not registered");
        auto& a=asset(path);if(image<0||size_t(image)>=a.groups.size()||a.groups[image].empty())throw std::runtime_error("inline font image id is outside graphic frame table");return a.groups[image][0];
    }
    void draw_text(int id) {
        if(!text_model->layers().count(id))return;
        // Keep hidden text authoritative in TextModel, but defer glyph work
        // until its parent is shown. update_parent() redraws before presentation.
        if(layers.count(id)&&!layers.at(id).visible){
            scene.erase(800000+id);for(int image:inline_text_images[id])scene.erase(image);inline_text_images[id].clear();return;
        }
        for(int image:inline_text_images[id])scene.erase(image);inline_text_images[id].clear();
        const auto& state=text_model->layers().at(id);std::vector<PositionedText> runs;
        const int px=layers.count(id)?layers.at(id).x:0,py=layers.count(id)?layers.at(id).y:0;
        for(const auto& rec:state.records){
            if(rec.kind==3){auto style=text_styles.count(rec.style_token)?text_styles.at(rec.style_token):current_text_style(id,rec.font);if(fonts.count(rec.font)){auto selected=fonts.at(rec.font);selected.color=style.color;selected.border_color=style.border_color;selected.shadow_color=style.shadow_color;selected.border_width=style.border_width;selected.shadow_offset=style.shadow_offset;if(style.native_font){auto native=*style.native_font;if(selected.native_font){native.family=selected.native_font->family;native.height=selected.native_font->height;native.width=selected.native_font->width;native.weight=selected.native_font->weight;}selected.native_font=native;}if(!style.font_path.empty())selected.font_path=style.font_path;style=selected;}runs.push_back({decode(rec.encoded,original_encoding.c_str()),px+rec.x,py+rec.y,style});}
            else if(rec.kind==4||rec.kind==5){bool original=rec.kind==4;const auto& path=original?text_model->original_font_resource():text_model->graphic_font_resource();const auto frame=font_image_frame(original,rec.image_id);int image=next_inline_image++;scene.load_cached(image,key(path),asset(path).bytes,px+rec.x,py+rec.y);scene.set_clip(image,{int(frame.x),int(frame.y),int(frame.w),int(frame.h)});if(original){const auto style=text_styles.count(rec.style_token)?text_styles.at(rec.style_token):current_text_style(id,rec.font);scene.set_glyph_color(image,uint32_t(style.color.r)|(uint32_t(style.color.g)<<8)|(uint32_t(style.color.b)<<16));}scene.set_z(image,id*10000+9001+int(inline_text_images[id].size()));if(layers.count(id)){scene.set_visible(image,layers.at(id).visible);scene.set_alpha(image,layers.at(id).alpha);}inline_text_images[id].push_back(image);}
        }
        scene.positioned_text(800000+id,runs,{0,0,scene.width(),scene.height()});scene.set_z(800000+id,id*10000+9000);
        if(layers.count(id)){scene.set_visible(800000+id,layers.at(id).visible);scene.set_alpha(800000+id,layers.at(id).alpha);}
    }
    void draw_key_wait_icon(){
        if(!key_wait_icon||!layers.count(key_wait_icon_parent))return;const auto& image=asset("system.ykg");const auto& frame=sprite_motion_group(image.groups,*key_wait_icon).at(key_wait_icon->frame);const auto& parent=layers.at(key_wait_icon_parent);scene.set_clip(17000000,{int(frame.x),int(frame.y),int(frame.w),int(frame.h)});scene.set_position(17000000,parent.x+key_wait_icon_x,parent.y+key_wait_icon_y);scene.set_alpha(17000000,parent.alpha);scene.set_visible(17000000,parent.visible);scene.set_z(17000000,key_wait_icon_parent*10000+9998);
    }
    void create_key_wait_icon(){
        if(settings.integer("KeyWaitIcon")!=1||!layers.count(text_layer)||!text_model->layers().count(text_layer))return;auto& image=asset("system.ykg");SpriteMotion motion;motion.group=settings.integer("KeyWaitIconNumber");motion.mode=2;sprite_motion_reset(image.groups,motion,now);key_wait_icon=motion;key_wait_icon_parent=text_layer;int mode=0,x=0,y=0;auto position=settings.state().records.find("KeyWaitIconPositionSet:"+std::to_string(text_layer));if(position!=settings.state().records.end()&&position->second.size()==4){mode=number(position->second[1]);x=number(position->second[2]);y=number(position->second[3]);}auto point=key_wait_icon_position(mode,x,y,text_model->layers().at(text_layer).flow);key_wait_icon_x=point.first;key_wait_icon_y=point.second;scene.load_cached(17000000,"system.ykg",image.bytes);draw_key_wait_icon();
    }
    void begin_key_wait(){
        create_key_wait_icon();key_wait_visibility.clear();for(const auto& name:{"KeyWaitGraphicShow","KeyWaitGraphicHide"})if(configuration.count(name))for(const auto& value:configuration.at(name)){int id=number(value);if(layers.count(id)){bool desired=std::string(name)=="KeyWaitGraphicShow";auto& layer=layers.at(id);if(layer.visible!=desired){key_wait_visibility.try_emplace(id,layer.visible);layer.visible=desired;scene.set_visible(id,desired);update_parent(id);}}}
    }
    void end_key_wait(){key_wait_icon.reset();scene.erase(17000000);for(const auto& [id,visible]:key_wait_visibility)if(layers.count(id)){layers.at(id).visible=visible;scene.set_visible(id,visible);update_parent(id);}key_wait_visibility.clear();}
    void text_ready(const VmLocation& location,bool voice_wait=false){configuration["KeyVoiceWaitPending"]={int32_t(voice_wait)};wait=Wait::Text;uint32_t character_delay=configuration.count("AutoCharacterDelay")?uint32_t(number(configuration.at("AutoCharacterDelay")[0])):0;auto_deadline=now+uint32_t(uint32_t(settings.integer("AutoSpeed"))+character_delay);configuration.erase("AutoCharacterDelay");++text_pages;std::cout<<"TEXT_READY page="<<text_pages<<" script="<<location.script<<" word="<<location.word<<'\n';}
    bool pump_text(bool immediate=false){
        for(;;){
            bool pending=false;for(const auto& p:pending_text)pending|=p.offset<p.encoded.size();
            if(!pending){pending_text.clear();pending_text_index=0;drawing=true;return true;}
            TextScheduleState state;state.next_target=pending_text_index;for(const auto& p:pending_text)state.targets.push_back({p.id,p.encoded,p.offset});
            TextSchedule schedule;schedule.restore(std::move(state));
            auto result=schedule.cycle([this](int id,std::string_view source){
                if(!text_model->layers().count(id))return source.size();auto& layer=text_model->layer(id);layer.style_token=intern_text_style(current_text_style(id,layer.font));
                auto consumed=text_model->output_step(id,std::string(source));if(consumed)texts[id].content=text_model->str_in(id);return consumed;
            });
            const auto& next=schedule.state();pending_text_index=next.next_target;for(size_t i=0;i<pending_text.size();++i)pending_text[i].offset=next.targets[i].offset;
            for(int id:result.changed_layers)draw_text(id);
            if(result.status==TextScheduleCycle::RoundComplete){uint32_t delay=configuration.count("AutoCharacterDelay")?uint32_t(number(configuration.at("AutoCharacterDelay")[0])):0;configuration["AutoCharacterDelay"]={int32_t(delay+uint32_t(settings.integer("AutoModeCharactorWait")))};}
            if(result.status==TextScheduleCycle::PageBlocked){text_ready(pending_text_location);return false;}
            if(pending_text_slow&&!immediate){wait=Wait::Reveal;reveal_next=now+uint32_t(settings.integer("TextSpeed"));return false;}
        }
    }
    void finish_reveal(){if(wait!=Wait::Reveal)return;if(pump_text(true)){vm.resume();wait=Wait::None;}}
    void toggle_skip(bool read_only){if(read_only)already_mode=!already_mode;else ordinary_skip_mode=!ordinary_skip_mode;skip_mode=already_mode||ordinary_skip_mode;if(skip_mode&&settings.integer("ModeExclusive"))auto_mode=false;}
    void finish_text_hidden(){for(int id:hidden_text_layers)if(layers.count(id)){layers[id].visible=true;scene.set_visible(id,true);update_parent(id);}hidden_text_layers.clear();if(configuration.count("WindowEraseWait")){wait=Wait(number(configuration.at("WindowEraseWait")[0]));configuration.erase("WindowEraseWait");}else{wait=Wait::None;vm.resume();}drawing=true;}
    void advance() { if(wait==Wait::TextHidden){finish_text_hidden();return;}if(wait==Wait::Text){configuration.erase("KeyVoiceWaitPending");if(key_wait_tte_mode){selection_elapsed=now-key_wait_started;if(key_wait_tte_mode==2)selection_elapsed%=10000;key_wait_tte_mode=0;}end_key_wait();if(configuration.count("DefaultKeyinSoundStopSet")&&number(configuration.at("DefaultKeyinSoundStopSet")[0])){int voice=number(vm.variable("Flag",270));if(voice)audio.stop(voice);}if(!pending_text.empty()){if(pending_text_index>=pending_text.size())throw std::runtime_error("invalid pending text page");int id=pending_text[pending_text_index].id;collect_history();if(text_model->layers().count(text_layer)){text_model->clear(text_layer);texts[text_layer].content.clear();draw_text(text_layer);}text_model->reset_cursor(id);text_model->layer(id).flow.overflow=false;if(pump_text(!pending_text_slow)){vm.resume();wait=Wait::None;}}else{vm.resume();wait=Wait::None;}} }
    uint32_t wait_function_mask()const{return uint32_t(settings.integer(wait==Wait::Selection?(text_selection?"SelectFunction":"MouseSelectFunction"):"KeyWaitFunction"));}
    void erase_wait_windows(){configuration["WindowEraseWait"]={int32_t(wait)};hidden_text_layers.clear();for(const auto& [id,box]:texts){(void)box;if(layers.count(id)&&layers[id].visible){hidden_text_layers.push_back(id);layers[id].visible=false;scene.set_visible(id,false);update_parent(id);}}wait=Wait::TextHidden;drawing=true;}
    void open_menu(const std::string& script) {
        if(in_menu||script.empty()||(wait!=Wait::Text&&wait!=Wait::Selection))return;
        if(wait==Wait::Selection){
            auto saved_regions=regions;auto saved_script=selection_script;auto parameters=std::make_tuple(select_layer,select_x,select_y,select_width,select_height,select_mode,select_normal,select_hover,select_read_normal,select_read_hover);bool textual=text_selection,timed=timed_selection,loop=selection_tte_loop;auto end=select_end;auto elapsed=now-select_start;
            menu_resume_context=[this,saved_regions,saved_script,parameters,textual,timed,loop,end,elapsed]{regions=saved_regions;selection_script=saved_script;std::tie(select_layer,select_x,select_y,select_width,select_height,select_mode,select_normal,select_hover,select_read_normal,select_read_hover)=parameters;text_selection=textual;timed_selection=timed;selection_tte_loop=loop;select_end=end;select_start=now-elapsed;wait=Wait::Selection;collection=false;hover=-1;if(text_selection)draw_choices();rebuild_selector();};
        }else menu_resume_context={};
        pending_menu=script;
    }
    bool system_interaction(const SDL_Event& e){
        if(wait!=Wait::Text&&(wait!=Wait::Selection||collection))return false;const auto mask=wait_function_mask();
        auto submenu=[&]{auto it=settings.state().values.find("SubMenuScript");if(!(mask&4)||!vm.submenu_enabled()||it==settings.state().values.end()||text(it->second).empty())return false;if(wait==Wait::Selection&&(timed_selection||(configuration.count("SelectionHasArguments")&&number(configuration.at("SelectionHasArguments")[0]))))return false;auto_mode=false;skip_mode=ordinary_skip_mode=already_mode=false;open_menu(text(it->second));return true;};
        if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_RIGHT)return submenu();
        if(e.type==SDL_MOUSEWHEEL&&(mask&0x101)==0x101&&e.wheel.y>0){show_history();return true;}
        if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT){
            const int x=e.button.x,y=e.button.y;
            // Native KeyWait consumes the left click and repeats the wait after
            // clearing an active mode when AutoModeCancelLKey is enabled.
            if(auto_mode&&settings.integer("AutoModeCancelLKey")){
                auto_mode=false;
                std::cout<<"AUTO_MODE value=0 reason=left-click-cancel\n";
                return true;
            }
            if((mask&1)&&areas.hit("AreaSetHistoryMode",x,y)){show_history();return true;}
            if((mask&2)&&areas.hit("AreaSetWindowErase",x,y)){erase_wait_windows();return true;}
            if((mask&0x40)&&areas.hit("AreaSetAlreadySkip",x,y)){toggle_skip(true);return true;}
            if((mask&0x20)&&areas.hit("AreaSetSkip",x,y)){toggle_skip(false);return true;}
            if((mask&0x10)&&areas.hit(auto_mode?"AreaSetAutoModeExecute":"AreaSetAutoMode",x,y)){
                auto_mode=!auto_mode;
                if(auto_mode&&settings.integer("ModeExclusive"))ordinary_skip_mode=already_mode=skip_mode=false;
                std::cout<<"AUTO_MODE value="<<(auto_mode?1:0)<<" reason=area\n";
                return true;
            }
            if(mask&8)for(int id:areas.hit_scripts(x,y)){auto area=areas.script_area(id);if(area&&!area->script.empty()){vm.set_script_state(0);open_menu(area->script);return true;}}
        }
        if(e.type==SDL_KEYDOWN){for(int vk:virtual_keys(e.key.keysym)){
            if(mask&8)for(int id:areas.key_scripts(vk,e.key.repeat)){auto area=areas.script_area(id);if(area&&!area->script.empty()){vm.set_script_state(0);open_menu(area->script);return true;}}
            if((mask&0x201)==0x201&&settings.key_matches("SystemKeySetHistoryMode",vk,e.key.repeat)){show_history();return true;}
            if((mask&0x82)==0x82&&settings.key_matches("SystemKeySetWindowErase",vk,e.key.repeat)){erase_wait_windows();return true;}
            if((mask&0x10)&&settings.key_matches("SystemKeySetAutoMode",vk,e.key.repeat)){auto_mode=!auto_mode;if(auto_mode&&settings.integer("ModeExclusive"))ordinary_skip_mode=already_mode=skip_mode=false;return true;}
            if((mask&0x60)&&settings.key_matches("SystemKeySetSkip",vk,e.key.repeat)){toggle_skip(bool(mask&0x40)&&number(vm.variable("GlobalFlag",3))!=0);return true;}
        }}return false;
    }
    HostResult delay(uint64_t ms) { deadline=now+ms;wait=Wait::Time;return {true,{}}; }
    void finish_movie(){movie.reset();drawing=true;wait=Wait::None;vm.resume();}
    void restart_engine(const std::string& entry){
        close_history(false,false);
        auto shared=vm.export_original();persist();clear_choices();movie.reset();scene.clear();key_wait_icon.reset();scene.clear_prefix_cache();if(mode_visuals)mode_visuals->clear_icon_instances();mode_icon_sprites={};layers.clear();layer_motion.clear();modified_pixels.clear();particle_scene_ids.clear();particles.clear();sprites.clear();texts.clear();fonts.clear();motions.clear();native_layers.clear();regions.clear();reveal_bytes.clear();pending_text.clear();pending_text_index=0;{std::vector<int> ids;for(const auto& p:text_model->layers())ids.push_back(p.first);for(int id:ids)text_model->erase_layer(id);}inline_text_images.clear();previous={};transition_end=effect_end=0;audio.stop_all();audio_fades.clear();audio_ends.clear();
        vm.reset(entry);for(size_t i=0;i<shared.flags.size();++i)if(shared.flags[i])vm.set_variable("Flag",int32_t(i),shared.flags[i]);for(const auto& item:shared.strings)vm.set_variable("String",item.first,item.second);
        in_menu=false;pending_menu.clear();menu_vm.clear();menu_runtime.clear();pending_subcall=selection_checkpoint=selection_event=false;history_mode=false;text_layer=13;text_selection=timed_selection=false;auto_mode=skip_mode=ordinary_skip_mode=already_mode=false;wait=Wait::None;drawing=true;collection=false;title=false;hover=-1;std::cout<<"RESTART_READY entry="<<entry<<'\n';
    }
    void present(){auto start=std::chrono::steady_clock::now();SDL_RenderPresent(scene.renderer());profile_present_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();}
    void render() {
        if(history_mode){scene.render();present();return;}
        if(music_game){music_game->render();present();return;}
        if(movie){SDL_SetRenderTarget(scene.renderer(),nullptr);SDL_SetRenderDrawColor(scene.renderer(),0,0,0,255);SDL_RenderClear(scene.renderer());if(movie->decoder->frame()){SDL_Rect destination{0,0,scene.width(),scene.height()};SDL_RenderCopy(scene.renderer(),movie->texture,nullptr,&destination);}present();return;}
        if(!drawing)return;
        if(previous&&now<effect_end){int step=int((now-effect_start)/std::max(1,effect_interval)),x=0,y=0;
            auto offset=Scene::effect_offset(effect_type,effect_amplitude,unsigned(step));x=offset.x;y=offset.y;
            scene.render_frozen(previous,x,y);
        }
        else {
            if(previous&&now<transition_end) {
                const auto duration=std::max<uint64_t>(1,transition_end-transition_start);
                scene.render_transition(previous,transition_mode,double(now-transition_start)/duration,transition_mask,transition_rect?&*transition_rect:nullptr,transition_sampling);
            }else scene.render();
        }
        if(auto_window_region)native_window_region(scene.window(),scene.thumbnail(scene.width(),scene.height()));
        present();
    }
    void draw_particles(){
        std::set<uint64_t> alive;
        for(const auto& [parent,layer]:particles.layers())if(layers.count(parent)){
            size_t index=0;const auto& l=layers.at(parent);
            for(const auto& p:layer.particles){alive.insert(p.serial);auto it=particle_scene_ids.find(p.serial);if(it==particle_scene_ids.end()){int id=next_particle_scene_id++;scene.load_cached(id,key(p.resource),asset(p.resource).bytes);it=particle_scene_ids.emplace(p.serial,id).first;}int id=it->second;const auto& frame=p.frame();scene.set_clip(id,{int(frame.x),int(frame.y),int(frame.w),int(frame.h)});scene.set_position(id,l.x+p.x,l.y+p.y);scene.set_visible(id,l.visible);scene.set_alpha(id,l.alpha);scene.set_z(id,parent*10000+5000+int(index++));}
        }
        for(auto it=particle_scene_ids.begin();it!=particle_scene_ids.end();)if(!alive.count(it->first)){scene.erase(it->second);it=particle_scene_ids.erase(it);}else ++it;
    }
    void tick() {
        if(history_mode){motion_tick=now;if(fast)audio.advance_virtual(16);return;}
        if(music_game){if(fast)audio.advance_virtual(16);const auto* keyboard=SDL_GetKeyboardState(nullptr);std::array<bool,5> keys{{keyboard[SDL_SCANCODE_Z]!=0,keyboard[SDL_SCANCODE_S]!=0,keyboard[SDL_SCANCODE_X]!=0,keyboard[SDL_SCANCODE_D]!=0,keyboard[SDL_SCANCODE_C]!=0}};music_game->tick(now,keys);if(music_game->finished()){int result=int(music_game->result());music_game.reset();auto state=std::move(music_parent_state);restore_runtime(state);vm.resume(int32_t(result));wait=Wait::None;motion_tick=now;}return;}
        if(mode_visuals){mode_visuals->update(auto_mode,already_mode,ordinary_skip_mode);mode_visuals->update_already(vm.current_already());}
        const uint32_t delta=uint32_t(now-motion_tick);motion_tick=now;
        if(cursor_hide_at&&now>=cursor_hide_at){SDL_ShowCursor(SDL_DISABLE);cursor_hide_at=0;}
        for(auto& [id,m]:layer_motion)if(layers.count(id)){m.visible=layers.at(id).visible;if(m.tick(delta,(skip_mode&&vm.current_frame_skip_mode()!=0)?std::max(1,settings.integer("SkipScrollSpeed")):1))apply_motion(id);}
        if(particles.tick(delta,0,false,[this](int id){return layers.count(id)?std::make_pair(double(layers.at(id).x),double(layers.at(id).y)):std::make_pair(0.,0.);}))draw_particles();
        if(key_wait_icon){sprite_motion_tick(asset("system.ykg").groups,*key_wait_icon,now);draw_key_wait_icon();}
        if(fast)audio.advance_virtual(16);
        if(wait==Wait::Selection&&!collection&&!selection_event){
            for(auto& region:regions)if((region.kind=="TimeInterrupt"||region.kind=="IntervalInterrupt")&&!region.args.empty()){
                uint64_t interval=uint32_t(number(region.args[0]));if((!region.fired||region.kind=="IntervalInterrupt")&&now-select_start>=interval+region.fired_at){region.fired=true;region.fired_at=now-select_start;vm.interrupt_selection(region.branch,region.end);selection_event=true;wait=Wait::None;break;}
            }

        }
        if(movie){bool changed=movie->decoder->advance(now-movie->start);if(changed){auto frame=movie->decoder->frame();if(SDL_UpdateTexture(movie->texture,nullptr,frame->rgba.data(),frame->width*4)<0)throw std::runtime_error(SDL_GetError());}for(auto& chunk:movie->decoder->take_audio())if(movie->device){for(auto& sample:chunk.samples)sample*=muted?0.f:float(direct_sound_pcm_gain(movie_attenuation));if(SDL_QueueAudio(movie->device,chunk.samples.data(),Uint32(chunk.samples.size()*sizeof(float)))<0)throw std::runtime_error(SDL_GetError());}if(movie->decoder->finished(now-movie->start)&&(!movie->device||SDL_GetQueuedAudioSize(movie->device)==0))finish_movie();}
        for(auto it=audio_fades.begin();it!=audio_fades.end();){auto& f=it->second;double t=f.end<=f.start?1.0:std::min(1.0,double(now-f.start)/double(f.end-f.start));int volume=f.from+int((f.to-f.from)*t);if(f.group){group_volumes[f.id]=volume;audio.set_group_volume(f.id,volume);}else{channel_volumes[f.id]=volume;audio.set_volume(f.id,volume);}if(t>=1){if(f.stop){audio.stop(f.id);audio.set_volume(f.id,f.from);channel_volumes[f.id]=f.from;audio_ends.erase(f.id);}it=audio_fades.erase(it);}else ++it;}

        if(wait==Wait::Reveal&&now>=reveal_next){if(pump_text()){vm.resume();wait=Wait::None;}}
        if(wait==Wait::Text&&!history_mode){
            const bool voice_wait=configuration.count("KeyVoiceWaitPending")&&!configuration.at("KeyVoiceWaitPending").empty()&&number(configuration.at("KeyVoiceWaitPending")[0])!=0;
            const bool pending_voice=audio.voice_wait_pending();
            if(ordinary_skip_mode||(already_mode&&vm.current_already()))advance();
            else if(auto_mode){if(pending_voice)auto_deadline=now+100;else if(now>=auto_deadline)advance();}
            else if(voice_wait&&!pending_voice)advance();
        }
        for(auto it=motions.begin();it!=motions.end();) {
            auto& m=it->second;
            double t=m.end<=m.start?1.0:std::min(1.0,double(now-m.start)/double(m.end-m.start));
            auto& l=layers.at(m.id);
            if(m.position){l.x=m.x+int((m.end_x-m.x)*t);l.y=m.y+int((m.end_y-m.y)*t);scene.set_position(m.id,l.x,l.y);}
            else {l.transmittance=m.alpha_from+(m.alpha_to-m.alpha_from)*t;l.alpha=255-int(l.transmittance*255/1000);scene.set_alpha(m.id,l.alpha);}
            update_parent(m.id);
            if(t>=1)it=motions.erase(it);else ++it;
        }
        for(auto& item:sprites) {
            auto& s=item.second;
            if(selector_delays.count(s.scene_id)){if(now<selector_delays.at(s.scene_id))continue;selector_delays.erase(s.scene_id);apply_frame(s);}
            const auto& groups=assets.at(layers.at(s.source).asset).groups;
            if(sprite_motion_tick(groups,s,now)) {
                const auto& frame=group_for(s).at(s.frame);
                scene.set_clip(s.scene_id,{int(frame.x),int(frame.y),int(frame.w),int(frame.h)});
                position_sprite(s);
            }
        }
    }
    void import_system(bool source=false) {
        auto path=(source?game_directory/"save":state_dir)/"System.dat";if(!std::filesystem::exists(path))path=game_directory/"save"/"System.dat";if(!std::filesystem::exists(path))return;
        auto data=read_file(path);if(data.size()<0x7c||u32(data,0x10)!=0x7c)return;
        auto saved=decode_original_system(data,original_encoding.c_str());system_template=saved.template_bytes;auto original=game_directory/"save"/"System.dat";if(std::filesystem::exists(original))system_template=read_file(original);
        for(int i=0;i<10000;++i){vm.set_variable("GlobalFlag",i,saved.flags[i]);vm.set_variable("GlobalString",i,saved.strings[i]);}
    }
    void persist() {
        std::filesystem::create_directories(state_dir);
        {auto bytes=vm.save_already(original_encoding.c_str());auto temp=state_dir/"Already.tmp";std::ofstream file(temp,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));file.close();if(!file)throw std::runtime_error("cannot write already-read state");std::filesystem::rename(temp,state_dir/"Already.dat");}
        std::ofstream out(state_dir/"yukari-system.tmp",std::ios::binary);
        if(!out)throw std::runtime_error("cannot write Yukari system state");
        out<<"YUKARI_SYSTEM_1\n";
        for(int i=0;i<10000;++i){int n=number(vm.variable("GlobalFlag",i));if(n)out<<"F "<<i<<' '<<n<<'\n'; auto s=text(vm.variable("GlobalString",i));if(!s.empty())out<<"S "<<i<<' '<<std::quoted(s)<<'\n';}
        out.close();if(!out)throw std::runtime_error("cannot flush Yukari system state");
        std::filesystem::rename(state_dir/"yukari-system.tmp",state_dir/"yukari-system.dat");
        if(system_template.empty()){auto path=game_directory/"save"/"System.dat";if(std::filesystem::exists(path))system_template=read_file(path);}
        if(!system_template.empty()){OriginalSystemState saved;saved.template_bytes=system_template;saved.encoding=original_encoding;for(int i=0;i<10000;++i){saved.flags.push_back(number(vm.variable("GlobalFlag",i)));saved.strings.push_back(text(vm.variable("GlobalString",i)));}auto data=encode_original_system(saved);std::ofstream native(state_dir/"System.tmp",std::ios::binary);native.write(reinterpret_cast<const char*>(data.data()),std::streamsize(data.size()));native.close();if(!native)throw std::runtime_error("cannot write original system state");std::filesystem::rename(state_dir/"System.tmp",state_dir/"System.dat");}
    }
    void restore() {
        std::ifstream in(state_dir/"yukari-system.dat",std::ios::binary);if(!in)in.open(state_dir/"system.dat",std::ios::binary);if(!in)return;
        std::string header;std::getline(in,header);if(header!="YUKARI_SYSTEM_1")return;
        char type;int id;
        while(in>>type>>id){if(type=='F'){int value=0;if(!(in>>value))throw std::runtime_error("truncated Yukari system state");vm.set_variable("GlobalFlag",id,int32_t(value));}else if(type=='S'){std::string value;if(!(in>>std::quoted(value)))throw std::runtime_error("truncated Yukari system state");vm.set_variable("GlobalString",id,value);}else throw std::runtime_error("invalid Yukari system state");}
    }
    bool registration(const std::string& name) {
        static const std::set<std::string> names={
            "SaveEnableMode","LoadEnableMode","SubMenuEnableMode","SelectTransitionSet","ScriptSkipMode","SelectFunctionSet","MouseSelectFunctionSet",
            "SaveDataLoadToCallScript","SaveDataDisplaySetToFont","SaveDataDisplaySetToGraphic","SaveDataDisplaySetToDate","SaveDataDisplaySetToComment1","SaveDataDisplaySetToComment2",
            "SoundGroupAutomodeSkipSet","TextWindowSet","TextColor","TextBorderColor","TextShadowColor","FontSet","RubyFontSet","DefaultTextWindowGroup","KeyWaitIconPositionSet",
            "DefaultFontSet","OriginalFontSet","DefaultFontEffect","GraphicTextRect","AreaSetSkip","AreaSetAlreadySkip","AreaSetAutoMode","AreaSetMouseLeftScript","AreaSetHistoryMode","AreaSetIndicate",
            "AreaSetAlreadySkip","AreaSetAutoMode","AreaSetMouseLeftScript","AreaSetHistoryMode","AreaSetIndicate","AreaSetHistoryExit","AreaSetHistoryNext","AreaSetHistoryPageNext","AreaSetHistoryPagePrev",
            "AreaSetHistoryPageSize","AreaSetHistoryPrev","AreaSetHistoryScrollBarV","AreaSetSkip","IconSetAlreadySkip","IconSetAutoMode","IconSetSkip","KeySetScript",
            "SystemKeySetHistoryMode","SystemKeySetWindowErase","HistoryWindowSet","HistoryTextColor","HistorySoundGroupSet","HistorySoundIconSet","HistorySoundNumberSet","HistoryToVoiceContinue","HistoryExitToNext",
            "CursorAutoHide","KeyWaitFunctionSet","DefaultKeyinSoundStopSet","SubMenuScript","AutoSpeedSet","AutoModeCharactorWaitSet","TransitionModeSet","AutoModeCancelLKey","KeyWaitIcon","TextSpeedSet"
        };
        return names.count(name)!=0;
    }
    void remember(const HostCall& call) {
        std::string name=call.function;
        if(call.arguments.size()>1&&std::holds_alternative<int32_t>(call.arguments[0]))name+=':'+std::to_string(number(call.arguments[0]));
        configuration[name]=call.arguments;
    }
    void case_region(const HostCall& call) {
        if(collection&&call.block_begin){auto args=call.arguments;auto kind=call.function;if(kind=="Case2"){if(args.empty()||number(args[0])==0)return;args.erase(args.begin());kind="Case";}if(kind=="MouseCase"){args.insert(args.begin(),int32_t(1));if(args.size()>=11)args.insert(args.begin()+11,int32_t(-1));kind="MouseCase2";}regions.push_back({kind,std::move(args),*call.block_begin,call.block_end.value_or(0)});}
    }
    void clear_selector(){for(const auto& [key,pair]:selector_sprites){auto it=sprites.find(pair);if(it!=sprites.end()){selector_delays.erase(it->second.scene_id);scene.erase(it->second.scene_id);sprites.erase(it);}}selector_sprites.clear();selector=SelectionModel{};}
    void rebuild_selector(){
        for(const auto& [key,pair]:selector_sprites){auto it=sprites.find(pair);if(it!=sprites.end()){selector_delays.erase(it->second.scene_id);scene.erase(it->second.scene_id);sprites.erase(it);}}selector_sprites.clear();selector=SelectionModel{};selector_focus.reset();SelectionDeclarations declarations;
        size_t text_row=0;for(const auto& r:regions){SelectionItem item;HostCall c{r.kind,{selection_script,0},r.args,r.branch,r.end};SelectionRegistration options;options.already=vm.is_already(selection_script,r.branch);options.text_render_mode=select_mode;
            if(r.kind=="Case"){Args a{r.args};int px=layers.count(select_layer)?layers.at(select_layer).x:0,py=layers.count(select_layer)?layers.at(select_layer).y:0;int x=a.size()>=3?a.n(1):select_x,y=a.size()>=3?a.n(2):select_y+int(text_row)*select_height;auto style=texts[select_layer].style;if(fonts.count(texts[select_layer].font))style=fonts.at(texts[select_layer].font);auto extent=scene.measure_text(a.s(0),style);options.text_rect=SelectionRect{px+x,py+y,px+x+extent.x,py+y+std::max(select_height,extent.y)};options.text_position=std::make_pair(px+x,py+y);++text_row;}
            auto before=declarations.model.items().size();auto decoded=declarations.consume(c,options);if(declarations.model.items().size()>before)item=declarations.model.items().back();else if(decoded==SelectionRegistrationResult::Added)continue;
            if(decoded==SelectionRegistrationResult::Unhandled){if(r.kind!="MouseCase2"&&r.kind!="Case"&&r.kind!="IndicateCase")continue;Args a{r.args};item.branch=r.branch;item.end=r.end;if(r.kind=="Case"){int px=layers.count(select_layer)?layers.at(select_layer).x:0,py=layers.count(select_layer)?layers.at(select_layer).y:0;item.rect=a.size()>=7?SelectionRect{px+a.n(3),py+a.n(4),px+a.n(5),py+a.n(6)}:SelectionRect{px+a.n(1),py+a.n(2),px+a.n(1)+(select_width?select_width:scene.width()),py+a.n(2)+select_height};}else if(r.kind=="MouseCase2"){if(a.size()<5)continue;item.blocked=a.n(0)==0;item.rect={a.n(1),a.n(2),a.n(3),a.n(4)};}else{item.kind=SelectionItem::Indicate;item.blocked=true;item.rect={a.n(0),a.n(1),a.n(2),a.n(3)};if(a.size()>9)item.sprites[1]={a.n(4),a.n(5),a.n(6),a.n(7),a.n(9),0};}}
            else if(decoded!=SelectionRegistrationResult::Added)continue;
            const size_t index=selector.add(item);for(unsigned slot=0;slot<3;++slot){const auto& spec=item.sprites[slot];if(!spec.present()||!layers.count(spec.parent)||!layers.count(spec.source))continue;int sprite_slot=10000+int(index)*3+int(slot);create_sprite(spec.parent,sprite_slot,spec.x,spec.y,spec.source,spec.group,false);selector_sprites[{index,slot}]={spec.parent,sprite_slot};}
        }
        selector_navigation=declarations.navigation;
        for(const auto& decoration:declarations.decorations){if(decoration.sprite){const auto& spec=*decoration.sprite;if(spec.present()&&layers.count(spec.parent)&&layers.count(spec.source)){int slot=20000+int(selector_sprites.size());create_sprite(spec.parent,slot,spec.x,spec.y,spec.source,spec.group,true);selector_sprites[{selector.items().size()+selector_sprites.size(),0}]={spec.parent,slot};}}else if(!decoration.text.empty()){int id=9300000+int(choice_layers.size());auto style=texts[select_layer].style;auto extent=scene.measure_text(decoration.text,style);scene.text(id,decoration.text,decoration.x-(decoration.centered?extent.x/2:0),decoration.y,std::max(1,extent.x+8),std::max(select_height,extent.y+8),style);scene.set_z(id,select_layer*10000+9600);choice_layers.push_back(id);}}
        update_selector(mouse_x,mouse_y);
    }
    void update_selector(int x,int y,bool mouse=true){
        SelectionInput input;input.x=x;input.y=y;if(mouse)selector_focus=selector.hit(x,y);auto changes=selector.update(input,selector_focus);for(const auto& change:changes.sprites){auto found=selector_sprites.find({change.item,change.slot});if(found==selector_sprites.end())continue;auto it=sprites.find(found->second);if(it==sprites.end())continue;auto& sp=it->second;sp.visible=change.visible;if(change.reset){apply_frame(sp);int delay=selector.items()[change.item].sprites[change.slot].delay_ms;if(delay>0)selector_delays[sp.scene_id]=now+uint32_t(delay);}position_sprite(sp);}for(int id:changes.play_sounds)if(audio.loaded(id))audio.play(id);for(int id:changes.stop_sounds)if(audio.loaded(id))audio.stop(id);
    }
    bool selector_event(const SDL_Event& e){
        SelectionInput input;input.x=mouse_x;input.y=mouse_y;input.focused=selector_focus;input.key_history=key_history;const auto* keyboard=SDL_GetKeyboardState(nullptr);for(int scan=0;scan<SDL_NUM_SCANCODES;++scan)if(keyboard[scan]){SDL_Keysym key{};key.scancode=SDL_Scancode(scan);key.sym=SDL_GetKeyFromScancode(key.scancode);for(int vk:virtual_keys(key))if(vk>0&&vk<256)input.key_state[size_t(vk)]=0x80;}const auto held=SDL_GetMouseState(nullptr,nullptr);input.mouse_held={{bool(held&SDL_BUTTON_LMASK),bool(held&SDL_BUTTON_RMASK),bool(held&SDL_BUTTON_MMASK)}};std::optional<SelectionCandidate> candidate;
        if(e.type==SDL_KEYDOWN){for(int32_t vk:virtual_keys(e.key.keysym)){if(!e.key.repeat){std::move_backward(key_history.begin(),key_history.end()-1,key_history.end());key_history[0]=vk;}input.key_history=key_history;input.message=0x100;input.wparam=uint32_t(vk);input.lparam=e.key.repeat?0x40000000:0;input.query=SelectionInput::KeyHistoryChanged;candidate=selector.candidate(input);if(!candidate){input.query=SelectionInput::KeyDown;candidate=selector.candidate(input);}if(!candidate){input.query=SelectionInput::SelectingKey;candidate=selector.candidate(input);}if(!candidate){auto nav=selector.navigate(vk,selector_focus,selector_navigation);if(nav.focused!=selector_focus){selector_focus=nav.focused;update_selector(mouse_x,mouse_y,false);if(text_selection&&selector_focus){for(size_t i=0;i<regions.size();++i)if(regions[i].branch==selector.items()[*selector_focus].branch){hover=int(i);draw_choices(hover);break;}}}bool decide=settings.key_matches("SystemKeySetDecide",vk,e.key.repeat)||(!settings.state().records.count("SystemKeySetDecide")&&vk==13);if((nav.activate||decide)&&selector_focus){const auto& item=selector.items()[*selector_focus];candidate=SelectionCandidate{*selector_focus,item.branch,item.end};}}if(candidate)break;}}
        else if(e.type==SDL_KEYUP){input.message=0x101;auto keys=virtual_keys(e.key.keysym);if(!keys.empty())input.wparam=uint32_t(keys.front());}
        else if(e.type==SDL_MOUSEMOTION)input.message=0x200;
        else if(e.type==SDL_MOUSEBUTTONDOWN||e.type==SDL_MOUSEBUTTONUP){int button=e.button.button==SDL_BUTTON_LEFT?0:e.button.button==SDL_BUTTON_RIGHT?1:2;input.message=0x201+button*3+(e.type==SDL_MOUSEBUTTONUP);input.query=SelectionInput::MouseInput;candidate=selector.candidate(input);}
        else if(e.type==SDL_MOUSEWHEEL){input.message=0x20a;input.wparam=uint32_t(int16_t(e.wheel.y*120))<<16;input.query=SelectionInput::MouseInput;candidate=selector.candidate(input);}
        if(!candidate&&input.message){input.query=SelectionInput::WindowMessage;candidate=selector.candidate(input);}if(candidate&&!selector.items()[candidate->item].ignore_input){int sound=selector.items()[candidate->item].select_sound;if(sound>=0&&audio.loaded(sound))audio.play(sound);finish_selection(candidate->branch);return true;}return false;
    }
    void clear_choices(){for(int id:choice_layers)scene.erase(id);choice_layers.clear();}
    void draw_choices(int selected=-1){
        clear_choices();size_t row=0;
        for(size_t i=0;i<regions.size();++i){auto& r=regions[i];if(r.kind!="Case")continue;Args a{r.args};auto style=texts[select_layer].style;if(fonts.count(texts[text_layer].font)){auto base=fonts.at(texts[text_layer].font);base.border_color=style.border_color;style=base;}
            bool read=vm.is_already(selection_script,r.branch);uint32_t color=int(i)==selected?(read&&select_read_hover!=0xffffffff?select_read_hover:select_hover):(read&&select_read_normal!=0xffffffff?select_read_normal:select_normal);if(color!=0xffffffff)style.color={uint8_t(color),uint8_t(color>>8),uint8_t(color>>16),255};
            int x=a.size()>=3?a.n(1):select_x,y=a.size()>=3?a.n(2):select_y+int(row)*select_height;int id=9100000+int(i);int px=layers.count(select_layer)?layers.at(select_layer).x:0,py=layers.count(select_layer)?layers.at(select_layer).y:0;
            if(select_mode!=1){scene.text(id,a.s(0),px+x,py+y,std::max(1,scene.width()-x),std::max(select_height,style.font_size+10),style);scene.set_z(id,select_layer*10000+9500+int(i));choice_layers.push_back(id);}++row;
        }
    }
    void finish_selection(uint32_t branch){clear_selector();if(timed_selection){selection_elapsed=now-select_start;if(selection_tte_loop){selection_elapsed%=10000;if(!selection_elapsed)selection_elapsed=1;}}clear_choices();vm.choose(branch);wait=Wait::None;collection=false;timed_selection=false;}
    std::optional<HostResult> host(const HostCall& call) {return timed_work(call.function,[&]{return dispatch_host(call);});}
    std::optional<HostResult> dispatch_host(const HostCall& call) {
        Args a{call.arguments}; const auto& n=call.function;
        if(n=="Restart"){
            if(a.size()==0||a.n(0)==0){SDL_MessageBoxButtonData buttons[2]={{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,1,"Return to title"},{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,"Cancel"}};SDL_MessageBoxData prompt{SDL_MESSAGEBOX_INFORMATION,scene.window(),"Yukari","Return to the title screen?",2,buttons,nullptr};int chosen=0;if(SDL_ShowMessageBox(&prompt,&chosen)<0)throw std::runtime_error(SDL_GetError());if(chosen!=1)return HostResult{};}
            pending_restart=a.size()>1?a.s(1):"start.yks";wait=Wait::Time;deadline=UINT64_MAX;return HostResult{true,{}};
        }
        if(n=="GraphicScrollLoop"||n=="GraphicScaleScroll"||n=="GraphicRotateScroll"||n=="GraphicRotateScrollLoop"||n=="GraphicRasterScroll"){
            if(a.size()&&layers.count(a.n(0))){std::vector<int32_t> v;for(const auto& arg:call.arguments)v.push_back(number(arg));motion_for(a.n(0)).command(n,v);apply_motion(a.n(0));}return HostResult{};
        }
        if(n=="GraphicCopy"||n=="GraphicCopy2"){
            if(a.size()==8&&layers.count(a.n(0))&&layers.count(a.n(3))){scene.blit(a.n(0),a.n(1),a.n(2),a.n(3),{a.n(4),a.n(5),n=="GraphicCopy"?a.n(6)-a.n(4):a.n(6),n=="GraphicCopy"?a.n(7)-a.n(5):a.n(7)});modified_pixels.insert(a.n(0));}return HostResult{};
        }
        if(n=="GraphicSwap"){
            if(a.size()<2||a.n(0)==a.n(1))return HostResult{};const int first=a.n(0),second=a.n(1);if(!layers.count(first)&&!layers.count(second))return HostResult{};
            scene.swap_layers(first,second);
            auto swap_keys=[&](auto& map){auto l=map.extract(first),r=map.extract(second);if(!l.empty()){l.key()=second;map.insert(std::move(l));}if(!r.empty()){r.key()=first;map.insert(std::move(r));}};
            swap_keys(layers);swap_keys(layer_motion);swap_keys(texts);swap_keys(native_layers);
            bool left_modified=modified_pixels.erase(first),right_modified=modified_pixels.erase(second);if(left_modified)modified_pixels.insert(second);if(right_modified)modified_pixels.insert(first);
            std::optional<TextLayerState> left_text,right_text;if(text_model->layers().count(first))left_text=text_model->layers().at(first);if(text_model->layers().count(second))right_text=text_model->layers().at(second);text_model->erase_layer(first);text_model->erase_layer(second);if(left_text)text_model->layer(second)=*left_text;if(right_text)text_model->layer(first)=*right_text;
            for(int id:{first,second}){scene.erase(800000+id);for(int image:inline_text_images[id])scene.erase(image);inline_text_images[id].clear();if(layers.count(id))scene.set_z(id,id*10000);}
            auto remap=[&](int id){return id==first?second:id==second?first:id;};std::map<std::pair<int,int>,Sprite> moved;for(auto& [k,sp]:sprites){sp.parent=remap(sp.parent);sp.source=remap(sp.source);scene.set_z(sp.scene_id,sp.parent*10000+sp.slot+1);position_sprite(sp);moved[{sp.parent,sp.slot}]=std::move(sp);}sprites=std::move(moved);
            auto records=configuration;for(const auto& [name,args]:records)if(name.rfind("Graphic",0)==0&&name.find(':')!=std::string::npos&&!args.empty()&&std::holds_alternative<int32_t>(args[0])&&(number(args[0])==first||number(args[0])==second))configuration.erase(name);for(const auto& [name,args]:records)if(name.rfind("Graphic",0)==0&&name.find(':')!=std::string::npos&&!args.empty()&&std::holds_alternative<int32_t>(args[0])&&(number(args[0])==first||number(args[0])==second)){auto v=args;v[0]=int32_t(remap(number(v[0])));configuration[name.substr(0,name.find(':')+1)+std::to_string(number(v[0]))]=std::move(v);}
            update_parent(first);update_parent(second);return HostResult{};
        }
        if(n=="GraphicCache"){if(a.size())scene.cache_prefix(a.n(0));return HostResult{};}
        if(n=="GraphicCacheDelete"){scene.clear_prefix_cache();return HostResult{};}
        if(n=="SnowEffect"||n=="Snow"||n=="SakuraEffect"){
            const bool sakura=n=="SakuraEffect";if(!a.size()){if(sakura){auto ids=particles.stop_sakura();for(int id:ids)erase_layer(id);}else particles.stop_snow();return HostResult{};}
            size_t begin=sakura?2:6;if(a.size()>begin){std::vector<int> ids;for(size_t i=begin;i<a.size();++i){int id=a.n(i);if(!layers.count(id)){scene.create_filter(id,4);scene.set_z(id,id*10000);layers[id]={"",0,0,255,true,10004,{}};}ids.push_back(id);}auto& source=asset(a.s(0));particles.configure(sakura?ParticleModel::Mode::Sakura:ParticleModel::Mode::Snow,a.s(0),a.n(1),sakura?ParticleModel::Rect{0,0,scene.width(),scene.height()}:ParticleModel::Rect{a.n(2),a.n(3),a.n(4),a.n(5)},ids,source.groups);}return HostResult{};
        }
        if(n=="GamePart_PlayMusic"){
            if(!a.size())return HostResult{false,int32_t(0)};MusicGameOptions options;if(a.size()>1)options.success_border=a.n(1);if(a.size()>2)options.speed=a.n(2);for(size_t i=0;i<5&&i+3<a.size();++i)options.character_variants[i]=a.n(i+3)!=0;if(a.size()>8)options.background=a.n(8);
            auto game=std::make_unique<MusicGameScene>(scene,audio,[this](const std::string& name){return read_resource(name);});game->set_audio_reader([this](const std::string& name){return open_resource(name);});int quality=settings.state().values.count("TransitionDrawMode")?settings.integer("TransitionDrawMode"):2,duration=quality==0?250:quality==1?500:1000;game->set_transition_sampling({duration,quality,transition_wait_type});music_parent_state=runtime_state();game->begin(a.s(0),options,now);music_game=std::move(game);wait=Wait::MusicGame;return HostResult{true,{}};
        }
        if(n=="SpriteIs"){bool found=false;if(a.size()>=2)for(const auto& [k,sp]:sprites)if(sp.parent==a.n(0)&&sp.visible&&sp.slot==a.n(1)){found=true;break;}return HostResult{false,int32_t(found)};}
        if(n=="KeyWaitGraphicShow"||n=="KeyWaitGraphicHide"){if(a.size()>16)throw std::runtime_error("KeyWaitGraphic list exceeds original 16 slots");configuration[n]=call.arguments;return HostResult{};}
        if(n=="HistoryMode"){return HostResult{show_history(true),{}};}
        if(n=="GraphicSwapAutoModeExecute"||n=="GraphicSwapSkipModeExecute"){auto which=n=="GraphicSwapAutoModeExecute"?ModeGraphic::Auto:ModeGraphic::Skip;if(!a.size()){configuration.erase(n);if(mode_visuals)mode_visuals->disable_graphic(which);}else if(a.size()>1){configuration[n]=call.arguments;if(mode_visuals)mode_visuals->set_graphic(which,a.n(0),a.s(1));}return HostResult{};}
        if(n=="AlreadyDataWrite"){persist();return HostResult{false,int32_t(1)};}
        if(n=="SaveDataLoadToDrawStop"){if(a.size())load_to_draw_stop=a.n(0)!=0;return HostResult{false,int32_t(load_to_draw_stop)};}
        if(n=="Confirmation"){
            if(!a.size())return HostResult{false,int32_t(0)};int kind=a.size()>2?a.n(2):0;if(kind<0||kind>2)return HostResult{false,int32_t(0)};std::string caption=a.size()>1?a.s(0):"Yukari",message=a.size()>1?a.s(1):a.s(0);SDL_MessageBoxButtonData buttons[2]={{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,1,kind==2?"Yes":"OK"},{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,0,kind==2?"No":"Cancel"}};SDL_MessageBoxData box{SDL_MESSAGEBOX_WARNING,scene.window(),caption.c_str(),message.c_str(),kind==1?1:2,buttons,nullptr};int chosen=0;if(SDL_ShowMessageBox(&box,&chosen)<0)throw std::runtime_error(SDL_GetError());return HostResult{false,int32_t(chosen==1)};
        }
        if(n=="SavePopMenu"||n=="LoadPopMenu"||n=="FontPopMenu"){
            SDL_Point point{mouse_x,mouse_y};if(a.size()>=2)point={a.n(0),a.n(1)};int wx,wy;SDL_RenderLogicalToWindow(scene.renderer(),float(point.x),float(point.y),&wx,&wy);point={wx,wy};std::vector<std::string> labels;
            if(n=="FontPopMenu"){labels=native_font_families();if(auto selected=native_popup_menu(scene.window(),point,labels)){auto file=native_font_file(labels[*selected]);for(auto& [id,font]:fonts)font.font_path=file;for(auto& [id,box]:texts){box.style.font_path=file;draw_text(id);}}}
            else{for(int slot=1;slot<=10;++slot)labels.push_back(std::to_string(slot)+": "+(slot_exists(slot)?slot_info(slot).comment:"----------------------------------------"));if(auto selected=native_popup_menu(scene.window(),point,labels)){pending_slot=int(*selected)+1;if(n=="SavePopMenu"){pending_comment=window_title;wait=Wait::Save;}else if(slot_exists(pending_slot))wait=Wait::Load;else return HostResult{};return HostResult{true,{}};}}return HostResult{};
        }
        if(auto result=areas.call(n,call.arguments)){remember(call);return HostResult{false,*result};}
        if(platform)if(auto result=platform->call(n,call.arguments))return HostResult{false,*result};
        if(cd_commands)if(auto command=cd_commands->handle(call,CdPolicy{skip_mode&&vm.current_frame_skip_mode()!=0,true})){if(command->wait_ms){deadline=now+*command->wait_ms;wait=Wait::CdTime;return HostResult{true,{}};}return command->result;}
        if(n=="HistorySoundGroupSet"){settings.call(n,call.arguments);remember(call);}
        if(auto command=audio_commands.handle(call,AudioPolicy{(ordinary_skip_mode||(already_mode&&vm.current_already()))&&vm.current_frame_skip_mode()!=0,false,(ordinary_skip_mode||(already_mode&&vm.current_already()))&&vm.current_frame_skip_mode()!=0})){if(n=="SoundGroupSet"||n=="SoundGroupVolumeSet"||n=="SoundGroupSilentSet"||n=="SoundGroupAutomodeSkipSet")remember(call);if(command->wait_ms)return delay(*command->wait_ms);if(command->wait_sound){wait=Wait::Audio;wait_id=*command->wait_sound;return command->result;}return command->result;}
        settings.seed("AutoMode",int32_t(auto_mode));settings.seed("SkipMode",int32_t(ordinary_skip_mode));settings.seed("AlreadySkipMode",int32_t(already_mode));
        SettingsFrame frame{int32_t(vm.current_frame_skip_mode()),vm.submenu_enabled()};
        if(auto result=settings.call(n,call.arguments,&frame)){if(mode_visuals&&a.size()>4&&(n=="IconSetAutoMode"||n=="IconSetAlreadySkip"||n=="IconSetSkip"||n=="IconSetAlready")){auto icon=n=="IconSetAutoMode"?ModeIndicator::Auto:n=="IconSetAlreadySkip"?ModeIndicator::AlreadySkip:n=="IconSetSkip"?ModeIndicator::Skip:ModeIndicator::Already;mode_visuals->set_icon(icon,{a.n(0),a.n(1),a.n(2),a.n(3),a.n(4)});}vm.set_current_frame_skip_mode(frame.script_skip_mode);vm.set_submenu_enabled(frame.submenu_enabled);auto_mode=settings.integer("AutoMode")!=0;already_mode=settings.integer("AlreadySkipMode")!=0;ordinary_skip_mode=settings.integer("SkipMode")!=0;skip_mode=already_mode||ordinary_skip_mode;if(a.size())remember(call);return HostResult{false,*result};}
        if(n=="ScriptSubCall"){pending_subcall=true;wait=Wait::Save;return HostResult{true,{}};}
        if(n=="SelectMode"){if(a.size()){select_mode=a.n(0);if(select_mode==0){select_hover=a.size()>1?uint32_t(a.n(1)):0xf2d51f;select_normal=a.size()>2?uint32_t(a.n(2)):0xffffffff;select_read_hover=a.size()>3?uint32_t(a.n(3)):0xffffffff;select_read_normal=a.size()>4?uint32_t(a.n(4)):0xffffffff;}}remember(call);return HostResult{};}
        if(n=="SelectPosition"){if(a.size())select_layer=a.n(0);select_x=a.size()>=3?a.n(1):0;select_y=a.size()>=3?a.n(2):0;select_width=a.size()>=5?a.n(3):0;select_height=a.size()>=5?a.n(4):27;remember(call);return HostResult{};}
        if(n=="TTE"){bool pass=selection_elapsed!=0&&(a.size()==0||selection_elapsed<uint32_t(a.n(0)));if(a.size()&&pass)selection_elapsed=0;HostResult result;result.skip_block=a.size()&&!pass;return result;}
        if(n=="MovieVolumeSet"||n=="MovieVolumeGet"){if(a.size()==1){movie_volume=a.n(0);auto attenuation=native_movie_attenuation(movie_volume,engine_config.sound_volume_type.value_or(false));if(native_sound_attenuation_valid(attenuation))movie_attenuation=attenuation;}return HostResult{false,int32_t(movie_volume)};}
        if(n=="MoviePlay"){
            if(!a.size())return HostResult{};std::string name=a.s(0);std::replace(name.begin(),name.end(),'\\','/');auto playback=std::make_unique<MoviePlayback>();playback->decoder=std::make_unique<Movie>(game_directory/name);auto info=playback->decoder->info();playback->texture=SDL_CreateTexture(scene.renderer(),SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,info.width,info.height);if(!playback->texture)throw std::runtime_error(SDL_GetError());
            if(info.has_audio&&!fast){SDL_AudioSpec wanted{};wanted.freq=48000;wanted.format=AUDIO_F32SYS;wanted.channels=2;wanted.samples=1024;playback->device=SDL_OpenAudioDevice(nullptr,0,&wanted,nullptr,0);if(!playback->device)throw std::runtime_error(SDL_GetError());SDL_PauseAudioDevice(playback->device,0);}
            playback->start=now;playback->cancellable=a.size()<2||a.n(1)!=0;movie=std::move(playback);wait=Wait::Movie;return HostResult{true,{}};
        }
        if(auto result=services.call(n,call.arguments)){if(result->delay_ms)return delay(*result->delay_ms);return HostResult{false,result->value};}
        if(n=="DefaultFontSet"){
            auto result=text_model->call(n,call.arguments);if(a.size()==4||a.size()==5||a.size()==6||a.size()==7){const auto& f=text_model->fonts().at(a.n(0));fonts[a.n(0)]=logical_font_style(f);}remember(call);return HostResult{false,result.value_or(Value{})};
        }
        if(n=="DefaultFontEffect"){remember(call);return HostResult{};}
        if(n=="TextWindowInvisible"){
            hidden_text_layers.clear();auto group=settings.state().records.find("DefaultTextWindowGroup");if(group!=settings.state().records.end())for(const auto& value:group->second){int id=number(value);if(layers.count(id)&&layers.at(id).visible){hidden_text_layers.push_back(id);layers[id].visible=false;scene.set_visible(id,false);update_parent(id);}}
            if(hidden_text_layers.empty())return HostResult{};wait=Wait::TextHidden;drawing=true;return HostResult{true,{}};
        }
        if(n=="HistoryRecord"){std::vector<int> ids;for(const auto& value:call.arguments)if(text_model->layers().count(number(value)))ids.push_back(number(value));if(!ids.empty())collect_history(std::move(ids));return HostResult{};}
        if(n=="TextWindowSet"||n=="GraphicTextRect"||n=="NameWindowSet"){
            if(n=="TextWindowSet"&&a.size())text_layer=a.n(0);
            if(a.size()>=5){auto& b=texts[a.n(0)];b.x=a.n(1);b.y=a.n(2);b.w=a.n(3)-b.x;b.h=a.n(4)-b.y;sync_text_layer(a.n(0),true);}
            if(n=="NameWindowSet")text_model->call(n,call.arguments);remember(call);return HostResult{};
        }
        if(n=="FontSet"){if(a.size()>=2){sync_text_layer(a.n(0));texts[a.n(0)].font=a.n(1);text_model->call(n,call.arguments);}return HostResult{};}
        if(n=="GraphicFont"||n=="OriginalFont"){
            if(a.size()){sync_text_layer(text_layer);auto& layer=text_model->layer(text_layer);layer.style_token=next_text_style++;text_styles[layer.style_token]=current_text_style(text_layer,layer.font);const auto frame=font_image_frame(n=="OriginalFont",a.n(0));text_model->graphic_font(text_layer,a.n(0),int(frame.w),int(frame.h));if(n=="OriginalFont")layer.records.back().kind=4;draw_text(text_layer);}return HostResult{};
        }
        if(n=="RubyFontSet"||n=="OriginalFontSet"||n=="GraphicFontSet"||n=="TextBoxSet"||n=="TextBoxDelete"||n=="TextBoxStringGet"||n=="TextBoxStringSet"||n=="StrIn"){auto value=text_model->call(n,call.arguments);return HostResult{false,value.value_or(Value{})};}
        if(n=="LF"){int id=a.size()?a.n(0):text_layer;if(!texts.count(id))return HostResult{};text_model->line_feed(id);texts[id].content=text_model->str_in(id);draw_text(id);return HostResult{};}
        if(n=="TextColor"||n=="TextBorderColor"||n=="TextShadowColor"||n=="TextColorFromGlobalFlag"){
            if(a.size()!=1&&a.size()!=2)return HostResult{};int id=a.size()==1?text_layer:a.n(0);uint32_t raw=uint32_t(a.n(a.size()-1));if(n=="TextColorFromGlobalFlag"){if(a.size()==2)raw|=0xff000000U;}else if(raw>0xffffff)raw=0xffffffffU;
            SDL_Color color{uint8_t(raw),uint8_t(raw>>8),uint8_t(raw>>16),255};auto& b=texts[id];if(n=="TextColor"||n=="TextColorFromGlobalFlag"){b.style.color=color;configuration["TextColorRaw:"+std::to_string(id)]={int32_t(raw)};}else if(n=="TextBorderColor"){b.style.border_color=color;configuration["TextBorderColorRaw:"+std::to_string(id)]={int32_t(raw)};}else{b.style.shadow_color=color;configuration["TextShadowColorRaw:"+std::to_string(id)]={int32_t(raw)};}return HostResult{};
        }
        if(n=="TextAlreadyColor"){if(a.size()<=1)configuration[n]={a.size()?Value{int32_t(a.n(0))}:Value{int32_t(-1)}};return HostResult{};}
        if(n=="DefaultTextShadowColor"||n=="DefaultTextBorderColor"){if(a.size()){int c=a.n(0);configuration[n]={int32_t(c<0||c>0xffffff?-1:c)};}return HostResult{};}
        if(n=="GraphicTextNoWait"){if(a.size()){int id=a.n(0);configuration[n+":"+std::to_string(id)]={int32_t(id),int32_t(a.size()>1?a.n(1)!=0:1)};}return HostResult{};}
        if(n=="DefaultTextWindowGroup"){if(a.size())text_layer=a.n(0);remember(call);return HostResult{};}
        if(n=="StrOut"||n=="StrOutNW"||n=="StrOutNWC"||n=="NameOut"){
            pending_text.clear();pending_text_index=0;pending_text_location=call.location;
            auto add=[&](int id,const std::string& value){if(!texts.count(id))sync_text_layer(id,true);else sync_text_layer(id);auto encoded=encode_original_text(value,original_encoding.c_str());if((n=="NameOut"||n=="StrOutNWC")&&!encoded.empty()){auto& l=text_model->layer(id);auto extent=text_model->measure(l.font,encoded);l.flow.x+=(l.flow.right-l.flow.x-extent.first)/2;l.flow.anchor_x=l.flow.x;}pending_text.push_back({id,std::move(encoded),0});};
            if(n=="NameOut"){if(a.size())add(text_model->name_window(),a.s(0));}else if(a.size()==1)add(text_layer,a.s(0));else for(size_t i=0;i+1<a.size();i+=2)add(a.n(i),a.s(i+1));
            pending_text_slow=n=="StrOut"&&!fast&&!ordinary_skip_mode&&!(already_mode&&vm.current_already())&&settings.integer("TextSpeed")>0&&!(a.size()==1&&configuration.count("GraphicTextNoWait:"+std::to_string(text_layer))&&number(configuration.at("GraphicTextNoWait:"+std::to_string(text_layer))[1]));
            if(pump_text())return HostResult{};return HostResult{true,{}};
        }
        if(n=="KeyVoiceWait"||n=="KeyWait"||n=="KeyWaitNA"||n=="KeyWaitTTE"||n=="KeyWaitTTELoop"){if(n=="KeyWaitNA"&&vm.current_already())return HostResult{};key_wait_tte_mode=n=="KeyWaitTTE"?1:n=="KeyWaitTTELoop"?2:0;key_wait_started=now;begin_key_wait();text_ready(call.location,n=="KeyVoiceWait");return HostResult{true,{}};}
        if(n=="PF"){bool body=a.size()==0;for(size_t i=0;i<a.size();++i)if(a.n(i)==text_layer)body=true;if(body)collect_history();for(auto& item:texts){bool clear=a.size()==0;for(size_t i=0;i<a.size();++i)if(a.n(i)==item.first)clear=true;if(clear){item.second.content.clear();text_model->clear(item.first);draw_text(item.first);}}return HostResult{};}
        if(n=="Effect"){snapshot();effect_type=a.n(0);effect_interval=int(Scene::effect_step_ms(effect_type,a.size()>1?a.n(1):100,transition_wait_type==1,scene.renderer_info().accelerated));effect_amplitude=a.size()>2?a.n(2):5;effect_start=now;effect_end=now+uint64_t(effect_interval)*Scene::effect_steps(effect_type);drawing=true;return delay(effect_end-now);}
        if(n=="FullScreenModeGet")return HostResult{false,int32_t((SDL_GetWindowFlags(scene.window())&SDL_WINDOW_FULLSCREEN)!=0)};
        if(n=="FullScreenMode"){SDL_SetWindowFullscreen(scene.window(),a.n(0)?SDL_WINDOW_FULLSCREEN_DESKTOP:0);return HostResult{};}
        if(n=="MouseXPointGet"||n=="MouseYPointGet"){return HostResult{false,int32_t(n=="MouseXPointGet"?mouse_x:mouse_y)};}
        if(n=="InputDialog"){input_value=input_original=a.s(0);input_limit=a.size()>2?std::clamp(a.n(2),0,1024):26;scene.erase(800809);show_input();SDL_Rect rect{360,265,440,32};SDL_SetTextInputRect(&rect);SDL_StartTextInput();wait=Wait::Input;return HostResult{true,{}};}
        if(n=="InputDialogGetText")return HostResult{false,input_value};
        if(n=="SaveDataSave"){pending_slot=a.n(0);pending_comment=a.size()>1?a.s(1):"";wait=Wait::Save;return HostResult{true,{}};}
        if(n=="SaveDataCheck")return HostResult{false,int32_t(slot_exists(a.n(0)))};
        if(n=="SaveDataLoad"){pending_slot=a.n(0);wait=Wait::Load;return HostResult{true,{}};}
        if(n=="SaveDataCopy"){if(a.size()>1&&slot_exists(a.n(0))){std::filesystem::create_directories(state_dir);if(std::filesystem::exists(slot_path(a.n(0))))std::filesystem::copy_file(slot_path(a.n(0)),slot_path(a.n(1)),std::filesystem::copy_options::overwrite_existing);auto source=existing_native_slot(a.n(0));if(std::filesystem::exists(source))std::filesystem::copy_file(source,native_slot_path(a.n(1)),std::filesystem::copy_options::overwrite_existing);}return HostResult{};}
        if(n=="SaveDataDelete"){int slot=a.size()?a.n(0):0;std::filesystem::remove(slot_path(slot));std::filesystem::remove(native_slot_path(slot));return HostResult{};}
        if(n=="SaveDataCommentChange"){if(a.size()>1){auto path=slot_path(a.n(0));if(std::filesystem::exists(path)){auto game=decode_save(read_file(path));game.comment=a.s(1);write_state_file(path,encode_save(game));}auto native=existing_native_slot(a.n(0));if(std::filesystem::exists(native)){OriginalSaveMetadataChange change;change.comment1=a.s(1);change.encoding=original_encoding.c_str();write_state_file(native_slot_path(a.n(0)),rewrite_original_save_metadata(read_file(native),change));}}return HostResult{};}
        if(n=="SaveDataThumbnailChange"){
            if(a.size()>1&&slot_exists(a.n(0))){auto png=scene.resource_thumbnail(read_resource(a.s(1)));auto path=slot_path(a.n(0));if(std::filesystem::exists(path)){auto game=decode_save(read_file(path));game.thumbnail=png;write_state_file(path,encode_save(game));}auto native=existing_native_slot(a.n(0));if(std::filesystem::exists(native)){OriginalSaveMetadataChange change;change.thumbnail_bmp=bmp_thumbnail(png);change.encoding=original_encoding.c_str();write_state_file(native_slot_path(a.n(0)),rewrite_original_save_metadata(read_file(native),change));}}return HostResult{};
        }
        if(n=="SaveDataCommentGet"){if(slot_exists(a.n(0)))return HostResult{false,slot_info(a.n(0)).comment};return HostResult{false,std::string{}};}
        if(n=="GraphicCreateSaveData") {
            if(a.size()!=4||!slot_exists(a.n(1)))return HostResult{false,int32_t(0)};
            const int id=a.n(0),slot=a.n(1);auto game=slot_info(slot);SavePreviewMetadata metadata;metadata.exists=true;metadata.thumbnail=game.thumbnail;metadata.comment1=game.comment;
            auto native_path=existing_native_slot(slot);if(std::filesystem::exists(native_path)){auto native=read_original_save_metadata(read_file(native_path),original_encoding.c_str());metadata.comment1=native.comment1;metadata.comment2=native.comment2;}
            std::time_t stamp=std::time_t(game.timestamp);std::tm tm{};gmtime_r(&stamp,&tm);metadata.date={tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec,true};
            auto layout=save_preview_layout(settings.state(),metadata,a.n(2),a.n(3),{0,original_encoding});
            erase_layer(id);scene.create_canvas(id,scene.width(),scene.height(),a.n(2),a.n(3));scene.set_z(id,id*10000);layers[id]={"",a.n(2),a.n(3),255,true,0,{}};
            texts[id]={0,0,scene.width(),scene.height(),0,{},""};auto& destination=text_model->layer(id);
            SavePreviewDrawHooks hooks;
            hooks.bitmap=[&](const SavePreviewBitmap& image,int,int){constexpr int temporary=19000000;scene.load(temporary,image.image);scene.paint(id,temporary,{image.x,image.y,image.width,image.height});scene.erase(temporary);};
            hooks.text=[&](const SavePreviewText& value,int,int){
                auto style=current_text_style(id,value.font);if(style.native_font){style.native_font->effect=value.effect;style.native_font->inherit_effect=false;}uint32_t color=value.color;if(color>0xffffffU&&color!=0xffffffffU)color=uint32_t(number(vm.variable("GlobalFlag",int32_t(color&0xffffffU))));style.color={uint8_t(color),uint8_t(color>>8),uint8_t(color>>16),255};
                style.border_width=value.effect==3||value.effect==4?(std::clamp(engine_config.font_border_thick_size.value_or(2),0,16)+1)/2:0;int shadow=std::clamp(engine_config.font_shadow_thick_size.value_or(2),0,8);style.shadow_offset=value.effect==2||value.effect==4?SDL_Point{shadow,shadow}:SDL_Point{0,0};
                TextModelHooks metric;metric.measure=[&](int,const std::string& raw){auto m=scene.measure_text(decode(raw,original_encoding.c_str()),style);return std::make_pair(m.x,m.y);};
                metric.dbcs_lead=[this](uint8_t c){if(original_encoding=="CP936"||original_encoding=="GBK")return c>=0x81&&c<=0xfe;return (c>=0x81&&c<=0x9f)||(c>=0xe0&&c<=0xfc);};
                metric.glyph_size=[&](int,const std::string& raw){auto m=scene.measure_glyph(decode(raw,original_encoding.c_str()),style);return std::make_pair(m.x,m.y);};
                TextModel model(metric,original_encoding);model.rules()=text_model->rules();auto font=text_model->fonts().find(value.font);
                if(font!=text_model->fonts().end()){const auto& f=font->second;model.call("DefaultFontSet",{int32_t(value.font),f.family,f.height,f.width,f.weight,f.cell_width,f.line_height});}
                else model.call("DefaultFontSet",{int32_t(value.font),std::string("Default"),int32_t(style.font_size),int32_t(0)});
                auto& paragraph=model.layer(id);paragraph.font=value.font;paragraph.style_token=intern_text_style(style);
                if(value.rect){paragraph.flow.left=value.rect->left;paragraph.flow.top=value.rect->top;paragraph.flow.right=value.rect->right;paragraph.flow.bottom=value.rect->bottom;paragraph.flow.reset();}else{paragraph.flow.x=paragraph.flow.anchor_x=value.x;paragraph.flow.y=paragraph.flow.anchor_y=value.y;}
                auto encoded=encode_original_text(value.text,original_encoding.c_str());size_t offset=0;
                while(offset<encoded.size()){auto consumed=model.output_step(id,encoded.substr(offset));if(!consumed)break;offset+=consumed;}
                if(offset<encoded.size()&&value.rect){paragraph.output_prepared=false;paragraph.pending_output.clear();paragraph.flow.x=paragraph.flow.anchor_x;paragraph.flow.y=paragraph.flow.anchor_y;paragraph.flow.overflow=false;model.output_step(id,encode_original_text("…",original_encoding.c_str()));}
                destination.records.insert(destination.records.end(),paragraph.records.begin(),paragraph.records.end());texts[id].content+=value.text;
            };
            draw_save_preview(layout,hooks);modified_pixels.insert(id);draw_text(id);drawing=true;return HostResult{false,int32_t(1)};
        }
        if(n=="WindowNameSet"){window_title=a.s(0);scene.set_title(window_title);return HostResult{};}
        if(n=="RGB") {
            uint32_t color=0;
            if(a.size()==3)color=(uint32_t(a.n(2))<<16)|(uint32_t(a.n(1))<<8)|uint32_t(a.n(0));
            else if(a.size()==1){uint32_t c=uint32_t(a.n(0));color=((c&255)<<16)|(c&0xff00)|((c>>16)&255);}
            else throw std::runtime_error("invalid RGB argument count");
            return HostResult{false,int32_t(color)};
        }
        if(n=="Random") {
            if(a.size()==0)return HostResult{false,int32_t(native_random())};if(a.size()>2)return HostResult{};
            int64_t width=a.size()==1?int64_t(a.n(0))+1:int64_t(a.n(1))-a.n(0)+1;
            if(width<=0||width>32767)throw std::runtime_error("Random range makes the original rejection loop undefined or nonterminating");
            uint32_t sample,limit=32767U-32767U%uint32_t(width);do{sample=native_random();}while(sample>=limit);
            return HostResult{false,int32_t(uint32_t(sample%uint32_t(width))+(a.size()==2?uint32_t(a.n(0))+1:0))};
        }
        if(n=="GraphicLoad"||n=="GraphicLoadHide"||n=="GraphicLoadADD"||n=="GraphicLoadSUB"||n=="GraphicLoadHideADD"||n=="GraphicLoadHideSUB") {
            Bytes mask;if(a.size()==3||a.size()==5)mask=resources.read(a.s(2));int type=n.size()>=3&&n.substr(n.size()-3)=="ADD"?4:n.size()>=3&&n.substr(n.size()-3)=="SUB"?5:2;
            load_layer(a.n(0),a.s(1),a.size()>=4?a.n(a.size()==5?3:2):0,a.size()>=4?a.n(a.size()==5?4:3):0,n.find("Hide")==std::string::npos,type,mask);if(a.size()==3||a.size()==5)layers.at(a.n(0)).mask=a.s(2);
            if(a.size()==2||a.size()==3){auto size=scene.size(a.n(0));auto& l=layers.at(a.n(0));l.x=(1024-size.x)/2;l.y=(576-size.y)/2;scene.set_position(a.n(0),l.x,l.y);}
            return HostResult{};
        }
        if(n=="GraphicDelete"){erase_layer(a.n(0));return HostResult{};}
        if(n=="GraphicFilter"){if(a.size()!=2)return HostResult{};int id=a.n(0);erase_layer(id);scene.create_filter(id,a.n(1));scene.set_z(id,id*10000);layers[id]={"",0,0,255,true,10000+a.n(1),{}};remember(call);return HostResult{};}
        if(n=="GraphicScale"||n=="GraphicScaleCenter"||n=="GraphicRotate"||n=="GraphicRotateCenter"){
            if(!a.size()||!layers.count(a.n(0)))return HostResult{};int id=a.n(0);if(n=="GraphicScale"&&a.size()>=3){motion_for(id).command(n,{a.n(0),a.n(1),a.n(2)});scene.set_scale(id,a.n(1),a.n(2));}else if(n=="GraphicScaleCenter"&&a.size()>=3)scene.set_scale_center(id,a.n(1),a.n(2));else if(n=="GraphicRotate"&&a.size()>=2){motion_for(id).command(n,{a.n(0),a.n(1)});scene.set_rotate(id,a.n(1));}else if(n=="GraphicRotateCenter"&&a.size()>=3)scene.set_rotate_center(id,a.n(1),a.n(2));remember(call);return HostResult{};
        }
        if(n=="GraphicVisible"){if(!a.size()||!layers.count(a.n(0)))return HostResult{false,int32_t(-1)};auto& l=layers.at(a.n(0));if(a.size()>1){l.visible=a.n(1)!=0;scene.set_visible(a.n(0),l.visible);update_parent(a.n(0));}return HostResult{false,int32_t(l.visible)};}
        if(n=="GraphicPositionX"||n=="GraphicPositionY"){if(a.size()==1&&layers.count(a.n(0)))return HostResult{false,int32_t(n=="GraphicPositionX"?layers.at(a.n(0)).x:layers.at(a.n(0)).y)};return HostResult{};}
        if(n=="GraphicAddColor"||n=="GraphicSubColor"||n=="GraphicSepia"||n=="GraphicMonochrome"||n=="GraphicNegaposi"){
            if(a.size()!=2)return HostResult{};
            int id=a.n(0);if(!layers.count(id))return HostResult{};
            if(n=="GraphicAddColor")scene.set_add_color(id,uint32_t(a.n(1)));
            else if(n=="GraphicSubColor")scene.set_sub_color(id,uint32_t(a.n(1)));
            else if(n=="GraphicSepia")scene.set_sepia(id,a.n(1)!=0);
            else if(n=="GraphicMonochrome")scene.set_monochrome(id,a.n(1)!=0);
            else scene.set_negaposi(id,a.n(1)!=0);
            remember(call);return HostResult{};
        }
        if(n=="GraphicHide"||n=="GraphicShow"){auto it=layers.find(a.n(0));if(it!=layers.end()){it->second.visible=n=="GraphicShow";scene.set_visible(a.n(0),it->second.visible);update_parent(a.n(0));}return HostResult{};}
        if(n=="GraphicMove"){auto& l=layers.at(a.n(0));motion_for(a.n(0)).move(a.n(1),a.n(2));motions.erase({a.n(0),true});l.x=a.n(1);l.y=a.n(2);scene.set_position(a.n(0),l.x,l.y);update_parent(a.n(0));return HostResult{};}
        if(n=="GraphicTransmittance"){auto it=layers.find(a.n(0));if(it==layers.end())return HostResult{};auto& l=it->second;l.transmittance=std::clamp(a.n(1),0,1000);l.alpha=255-int(l.transmittance*255/1000);scene.set_alpha(a.n(0),l.alpha);update_parent(a.n(0));return HostResult{};}
        if(n=="GraphicScroll"){if(a.size()&&layers.count(a.n(0))){std::vector<int32_t> args;for(const auto& v:call.arguments)args.push_back(number(v));motion_for(a.n(0)).command(n,args);}return HostResult{};}
        if(n=="GraphicTransmittanceScroll") {
            int id=a.n(0);auto& l=layers.at(id);Motion m;m.id=id;m.start=now;m.position=n=="GraphicScroll";
            if(m.position){m.x=l.x;m.y=l.y;m.end_x=a.n(2);m.end_y=a.n(3);m.end=now+std::max(0,a.n(1));}
            else{m.alpha_from=l.transmittance;m.alpha_to=std::clamp(a.n(1),0,1000);m.alpha=l.alpha;m.end_alpha=255-std::clamp(a.n(1),0,1000)*255/1000;m.end=now+std::max(0,a.n(2));}
            motions[{id,m.position}]=m;return HostResult{};
        }
        if(n=="GraphicScrollWait"||n=="GraphicTransmittanceScrollWait"){wait=Wait::Motion;wait_id=a.n(0);return HostResult{true,{}};}
        if(n=="SpriteCreate"||n=="SpriteCreateHide"){create_sprite(a.n(0),a.n(1),a.n(2),a.n(3),a.n(4),a.n(5),n=="SpriteCreate");return HostResult{};}
        if(n=="SpriteMove"){auto it=sprites.find({a.n(0),a.n(1)});if(it!=sprites.end()){it->second.x=a.n(2);it->second.y=a.n(3);position_sprite(it->second);}return HostResult{};}
        if(n=="GraphicRefresh"){for(const auto& entry:text_model->layers()){int id=entry.first;for(auto& rec:text_model->layer(id).records){auto style=current_text_style(id,rec.font);if(text_styles.count(rec.style_token)){const auto& saved=text_styles.at(rec.style_token);style.color=saved.color;if(saved.native_font&&style.native_font){const auto& old=*saved.native_font;auto& next=*style.native_font;if(!old.inherit_effect){next.effect=old.effect;next.inherit_effect=false;}if(!old.inherit_border){next.border_color=old.border_color;next.inherit_border=false;}if(!old.inherit_shadow){next.shadow_color=old.shadow_color;next.inherit_shadow=false;}}}rec.style_token=intern_text_style(style);}draw_text(id);}return HostResult{};}
        if(n=="SpriteMotionWait"){wait=Wait::Sprite;wait_id=a.n(0);wait_slot=a.n(1);return HostResult{true,{}};}
        if(n=="SpriteDelete"){auto it=sprites.find({a.n(0),a.n(1)});if(it!=sprites.end()){scene.erase(it->second.scene_id);sprites.erase(it);}return HostResult{};}
        if(n=="SpriteShow"||n=="SpriteHide"){auto it=sprites.find({a.n(0),a.n(1)});if(it!=sprites.end()){it->second.visible=n=="SpriteShow";position_sprite(it->second);}return HostResult{};}
        if(n=="DrawStop"){if(drawing)snapshot();drawing=false;return HostResult{};}
        if(n=="DrawStart"){drawing=true;return HostResult{};}
        if(n=="Transition"||n=="UserTransition") {
            bool user=n=="UserTransition";if((!user&&a.size()!=1&&a.size()!=2&&a.size()!=6)||(user&&a.size()!=2&&a.size()!=3&&a.size()!=6))return HostResult{};
            transition_mode=a.n(0);drawing=true;transition_end=now;transition_mask.clear();transition_rect.reset();if((!user&&(transition_mode<1||transition_mode>32))||(user&&(transition_mode<1||transition_mode>2)))return HostResult{};
            if(user)transition_mask=asset(a.s(1)).bytes;
            if(a.size()==6){if(user)throw std::runtime_error("original UserTransition six-argument RECT form reads a missing seventh argument");transition_rect=SDL_Rect{a.n(2),a.n(3),a.n(4)-a.n(2),a.n(5)-a.n(3)};}
            int duration=user?(a.size()==2?1000:a.n(2)):(a.size()==1?-1:a.n(1));int quality=settings.state().values.count("TransitionDrawMode")?settings.integer("TransitionDrawMode"):2;
            if(duration<0){if(user)duration=1000;else if(transition_mode==1||(transition_mode>=3&&transition_mode<=6)||(transition_mode>=11&&transition_mode<=19))duration=50;else if(transition_mode==20)duration=10;else duration=100;}
            if(quality<0||settings.integer("TransitionSkip")!=0)return HostResult{};if(!user&&quality==0)duration=std::max(1,duration/4);else if(!user&&quality==1)duration=std::max(1,duration/2);
            transition_sampling={duration,quality,transition_wait_type};transition_start=now;auto timing=scene.transition_timing(transition_mode,transition_sampling,user,transition_rect?&*transition_rect:nullptr);transition_end=now+timing.total_ms;return delay(timing.total_ms);
        }
        if(n=="Wait")return delay(std::max(0,a.n(0)));
        if(n=="FlagReset"){vm.clear_variables();return HostResult{};}
        if(n=="SystemDataWrite"){persist();return HostResult{false,int32_t(1)};}
        if(n=="HistoryReset"){history.clear();configuration.erase("HistoryEntries");return HostResult{};}
        if(n=="MouseSelect"||n=="MouseSelectL"||n=="Select"||n=="SelectTTE"||n=="SelectTTELoop"||n=="MouseSelectTTE"||n=="MouseSelectTTELoop") {configuration["SelectionHasArguments"]={int32_t(a.size()!=0)};clear_choices();regions.clear();hover=-1;collection=true;text_selection=n=="Select"||n=="SelectTTE"||n=="SelectTTELoop";timed_selection=n.find("TTE")!=std::string::npos;select_limit=0;selection_tte_loop=n=="SelectTTELoop"||n=="MouseSelectTTELoop";select_start=now;select_end=call.block_end.value_or(0);selection_script=key(call.location.script);selection_checkpoint=!call.subcall;wait=Wait::Selection;drawing=true;return HostResult{false,{},true};}
        if(n=="ScriptExit"){quit=true;return HostResult{true,{}};}
        if(collection&&call.block_begin){case_region(call);return HostResult{};}
        if(registration(n)){remember(call);return HostResult{};}
        return std::nullopt;
    }
    bool inside(const Region& r,int x,int y) const {
        Args a{r.args};size_t k=r.kind=="IndicateCase"?0:1;
        if(r.kind=="Case"){int px=layers.count(select_layer)?layers.at(select_layer).x:0,py=layers.count(select_layer)?layers.at(select_layer).y:0;if(a.size()>=7)return x>=px+a.n(3)&&y>=py+a.n(4)&&x<=px+a.n(5)&&y<=py+a.n(6);if(a.size()>=3)return x>=px+a.n(1)&&y>=py+a.n(2)&&x<=px+a.n(1)+(select_width>0?select_width:scene.width())&&y<=py+a.n(2)+select_height;return false;}
        return a.size()>=k+4 && (k==0||a.n(0)!=0) && x>=a.n(k)&&y>=a.n(k+1)&&x<=a.n(k+2)&&y<=a.n(k+3);
    }
    void hover_at(int x,int y) {
        update_selector(x,y);
        if(text_selection){int found=-1;for(size_t i=regions.size();i>0;--i)if(regions[i-1].kind=="Case"&&inside(regions[i-1],x,y)){found=int(i-1);break;}if(found!=hover){hover=found;draw_choices(found);}}
    }
    void click(int x,int y) {
        mouse_x=x;mouse_y=y;SelectionInput input;input.query=SelectionInput::MouseInput;input.x=x;input.y=y;input.message=0x201;auto candidate=selector.candidate(input);std::optional<size_t> item=candidate?std::optional<size_t>(candidate->item):selector.hit(x,y);
        if(item&&!selector.items()[*item].ignore_input){const auto& selected=selector.items()[*item];if(selected.select_sound>=0&&audio.loaded(selected.select_sound))audio.play(selected.select_sound);finish_selection(selected.branch);}
    }
    void cancel_selection() { for(auto it=regions.rbegin();it!=regions.rend();++it)if(it->kind=="MouseKeyCase"&&Args{it->args}.n(0)==2){finish_selection(it->branch);return;} }
public:
    Runtime(const std::filesystem::path& directory,const std::filesystem::path& state,uint32_t seed,bool accelerated,NativeFontOptions font_options={},std::filesystem::path original_executable={},std::filesystem::path patch={})
        :resources(directory),game_directory(directory),translation_patch(patch),vm([this](const std::string& n){return translation_patch.apply(parse_script(resources.read(n)),n);}),services(ServiceHooks{
            [this]{return uint32_t(now);},{},
            [this](std::string pattern){std::replace(pattern.begin(),pattern.end(),'\\','/');glob_t matches{};const auto path=(game_directory/pattern).string();bool found=false;if(glob(path.c_str(),0,nullptr,&matches)==0)for(size_t i=0;i<matches.gl_pathc;++i)if(std::filesystem::is_regular_file(matches.gl_pathv[i])){found=true;break;}globfree(&matches);return found;},
            [this](const std::string& name){return resources.contains(name);},
            [this](bool horizontal){return horizontal?scene.width():scene.height();},"CP932"}),state_dir(state),random(seed),fast(accelerated) {
        if(!patch.empty()&&font_options.fallback_font.empty()){
            for(const auto* candidate:{"/System/Library/Fonts/Hiragino Sans GB.ttc","/System/Library/Fonts/STHeiti Medium.ttc"})
                if(std::filesystem::is_regular_file(candidate)){font_options.fallback_font=candidate;break;}
        }
        scene.configure_native_fonts(std::move(font_options));
        std::optional<Bytes> ini;try{ini=resources.read("default.ini");}catch(const std::runtime_error&){}if(ini)engine_config=parse_engine_config(*ini);
        if(engine_config.sound_volume_type)audio.set_volume_type(*engine_config.sound_volume_type);if(engine_config.transition_wait_type)transition_wait_type=*engine_config.transition_wait_type;if(engine_config.skip_scroll_speed)settings.seed("SkipScrollSpeed",*engine_config.skip_scroll_speed);if(engine_config.mode_exclusive)settings.seed("ModeExclusive",int32_t(*engine_config.mode_exclusive));
        MacCdHooks cd_hooks;cd_hooks.now_ms=[this]{return uint32_t(now);};cd_player=std::make_unique<MacCdPlayer>(std::move(cd_hooks));cd_commands=std::make_unique<CdCommands>(cd_player->provider());cd_player->set_clock_sink([this](uint32_t track,uint32_t playback){cd_commands->set_clock_epochs(track,playback);});cd_player->set_silent_query([this]{return cd_commands->state().silent;});
        TextModelHooks text_hooks;
        text_hooks.dbcs_lead=[this](uint8_t c){if(original_encoding=="CP936"||original_encoding=="GBK")return c>=0x81&&c<=0xfe;return (c>=0x81&&c<=0x9f)||(c>=0xe0&&c<=0xfc);};
        text_hooks.measure=[this](int font,const std::string& encoded){auto style=fonts.count(font)?fonts.at(font):TextStyle{};auto p=scene.measure_text(decode(encoded,original_encoding.c_str()),style);return std::make_pair(p.x,p.y);};
        text_hooks.glyph_size=[this](int font,const std::string& encoded){auto style=current_text_style(text_layer,font);auto p=scene.measure_glyph(decode(encoded,original_encoding.c_str()),style);return std::make_pair(p.x,p.y);};
        text_hooks.register_original_font=[this](const std::string& path){asset(path);};text_hooks.register_graphic_font=text_hooks.register_original_font;
        text_hooks.graphic_font_size=[this](int image){auto f=font_image_frame(false,image);return std::make_pair(int(f.w),int(f.h));};text_hooks.original_font_size=[this](int image){auto f=font_image_frame(true,image);return std::make_pair(int(f.w),int(f.h));};
        native_text_box=std::make_unique<NativeTextBox>(scene.window(),scene.width(),scene.height(),[this]{return text_model&&text_model->fonts().count(0)?text_model->fonts().at(0).family:std::string("Hiragino Sans");});native_text_box->bind(text_hooks);
        text_model=std::make_unique<TextModel>(std::move(text_hooks),original_encoding);
        ModeVisualHooks visual_hooks;visual_hooks.layer_resource=[this](int id)->std::optional<std::string>{if(!layers.count(id))return {};return layers.at(id).asset;};visual_hooks.reload_image=[this](int id,const std::string& name,const std::string&){reload_layer_image(id,name);};visual_hooks.create_icon=[this](ModeIndicator mode,const ModeIconRecord& icon){if(!layers.count(icon.parent)||!layers.count(icon.source))return false;int slot=30000+int(mode);create_sprite(icon.parent,slot,icon.x,icon.y,icon.source,icon.group,true);auto& sprite=sprites.at({icon.parent,slot});sprite.mode=0;apply_frame(sprite);mode_icon_sprites[unsigned(mode)]=std::make_pair(icon.parent,slot);return true;};visual_hooks.remove_icon=[this](ModeIndicator mode,const ModeIconRecord&){auto& saved=mode_icon_sprites[unsigned(mode)];if(saved){auto it=sprites.find(*saved);if(it!=sprites.end()){scene.erase(it->second.scene_id);sprites.erase(it);}saved.reset();}};mode_visuals=std::make_unique<ModeVisuals>(std::move(visual_hooks));
        vm.set_host([this](const HostCall& c){return host(c);});scene.set_transition_random([this]{return native_random();});particles.set_random([this]{return native_random();});audio_commands.set_source_loader([this](const std::string& name){return open_resource(name);});audio_commands.set_midi_provider([this](const Bytes& bytes){midi.stop();midi.load(bytes);midi.play();},[this]{midi.stop();});restore();PlatformHooks hooks;hooks.read_resource=[this](const std::string& name){try{return resources.read(name);}catch(const std::runtime_error&){std::string file=name;std::replace(file.begin(),file.end(),'\\','/');return read_file(game_directory/file);}};hooks.default_cursor_names[0]=engine_config.normal_cursor.value_or("");hooks.cursor_image=[read=hooks.read_resource](const std::string& name){return decode_windows_cursor(read(name));};hooks.logical_size=[this]{return SDL_Point{scene.width(),scene.height()};};hooks.auto_region=[this](bool enabled){auto_window_region=enabled;if(!enabled)native_window_region(scene.window(),{});};hooks.extract_dialog=native_extract_dialog;hooks.execute=native_shell_open;
        // Resolve only the requested executable, or the conventional directory-name EXE.
        // Never choose an arbitrary EXE from an install that may include patches.
        if(original_executable.empty()){
            const auto directory_name=std::filesystem::canonical(game_directory);
            original_executable=directory_name/(directory_name.filename().string()+".exe");
        }
        hooks.icon_image=[original_executable](uint16_t id){
            if(!std::filesystem::is_regular_file(original_executable))
                throw std::runtime_error("WindowIconChange requires --original-exe PATH to the original engine executable");
            return decode_windows_cursor(pe_icon(read_file(original_executable),id));
        };
        hooks.cursor_auto_hide=[this](int32_t ms){cursor_auto_hide=ms;};platform=std::make_unique<PlatformServices>(scene.window(),scene.renderer(),game_directory,state_dir,std::move(hooks));if(engine_config.window_size_variable)platform->call("WindowSizeVariableSet",{int32_t(*engine_config.window_size_variable)});
    }
    int run(bool require_gpu,bool force_native,const std::string& encoding,int load_initial,const std::filesystem::path& action_file,bool stop_at_title,const std::filesystem::path& capture,uint64_t timeout,bool mute,bool trace,int hx,int hy,bool start_new,bool stop_at_text,int advances) {
        auto backend=scene.renderer_info();
        std::cout<<"RENDERER name="<<backend.name<<" accelerated="<<backend.accelerated<<" targets="<<backend.target_textures<<'\n';
        if(require_gpu&&!backend.accelerated)throw std::runtime_error("GPU renderer required, but only software is available");
        original_encoding=encoding;text_model->set_encoding(encoding);
        {auto path=state_dir/"Already.dat";if(std::filesystem::exists(path))vm.load_already(read_file(path),encoding.c_str());}
        if(!action_file.empty()){std::ifstream input(action_file);if(!input)throw std::runtime_error("cannot open actions file");std::string line;while(std::getline(input,line))if(!line.empty()&&line[0]!='#')actions.push_back(line);}
        until_title=stop_at_title;auto_new=start_new;until_text=stop_at_text;auto_advance=advances;screenshot=capture;max_ms=timeout;hover_test_x=hx;hover_test_y=hy;muted=mute;audio.set_muted(mute);if(cd_player)cd_player->set_muted(mute);audio.set_virtual_clock(fast);
        if(trace)vm.set_trace([](const VmLocation& l,const std::string& op){std::cerr<<l.script<<":word=0x"<<std::hex<<l.word<<std::dec<<"  "<<op<<'\n';});
        if(force_native)import_system(!std::filesystem::exists(native_slot_path(load_initial)));
        vm.start("start.yks");if(load_initial>=0){if(!force_native&&std::filesystem::exists(slot_path(load_initial)))load_slot(load_initial);else initial_native=load_initial;}const uint64_t start=SDL_GetTicks64();
        while(!quit) {
            const bool measure=frame_profile.active();const auto frame_start=std::chrono::steady_clock::now();const auto before_stats=measure?scene.render_stats():RenderStats{};profile_present_ms=0;double profile_vm_ms=0;const auto before_vm_steps=vm.steps();
            now=fast?now+16:SDL_GetTicks64()-start;
            if(max_ms && now>max_ms)throw std::runtime_error("runtime deadline before requested completion");
            tick(); SDL_Event e;
            while(SDL_PollEvent(&e)) {
                if(e.type==SDL_MOUSEMOTION){mouse_x=e.motion.x;mouse_y=e.motion.y;SDL_ShowCursor(SDL_ENABLE);if(cursor_auto_hide)cursor_hide_at=now+std::max<uint32_t>(10,uint32_t(cursor_auto_hide));}
                if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT)left_down=true;
                if(e.type==SDL_MOUSEBUTTONUP&&e.button.button==SDL_BUTTON_LEFT)left_down=false;
                if(e.type==SDL_QUIT){quit=true;break;}
                if(wait==Wait::CdTime&&((e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT)||(e.type==SDL_KEYDOWN&&(e.key.keysym.sym==SDLK_RETURN||e.key.keysym.sym==SDLK_SPACE)))){vm.resume();wait=Wait::None;continue;}
                if(music_game){if((e.type==SDL_KEYDOWN&&(e.key.keysym.sym==SDLK_RETURN||e.key.keysym.sym==SDLK_SPACE))||(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT))music_game->confirm();continue;}
                if(movie){if(movie->cancellable&&((e.type==SDL_KEYDOWN&&(e.key.keysym.sym==SDLK_ESCAPE||e.key.keysym.sym==SDLK_RETURN||e.key.keysym.sym==SDLK_SPACE))||(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT)))finish_movie();continue;}
                if(wait==Wait::TextHidden){if(e.type==SDL_MOUSEBUTTONDOWN||(e.type==SDL_KEYDOWN&&settings.key_matches("SystemKeySetWindowErase",windows_virtual_key(e.key.keysym.sym),e.key.repeat!=0)))finish_text_hidden();continue;}
                if(wait==Wait::Input){if(e.type==SDL_TEXTINPUT){auto proposed=input_value+e.text.text;size_t count=0;for(unsigned char c:proposed)if((c&0xc0)!=0x80)++count;if(count<=size_t(input_limit)){try{encode_original_text(proposed,original_encoding.c_str());input_value=std::move(proposed);show_input();}catch(const std::runtime_error&) {}}}else if(e.type==SDL_KEYDOWN){if(e.key.keysym.sym==SDLK_RETURN)finish_input(true);else if(e.key.keysym.sym==SDLK_ESCAPE)finish_input(false);else if(e.key.keysym.sym==SDLK_BACKSPACE&&!input_value.empty()){size_t at=input_value.size()-1;while(at>0&&(uint8_t(input_value[at])&0xc0)==0x80)--at;input_value.resize(at);show_input();}}continue;}
                if(history_mode){
                    if(e.type==SDL_MOUSEMOTION){if(history_drag&&left_down){history_view->drag_vertical(e.motion.y);draw_history();}else history_hover_at(e.motion.x,e.motion.y);}
                    else if(e.type==SDL_MOUSEBUTTONUP&&e.button.button==SDL_BUTTON_LEFT)history_drag=false;
                    else if(e.type==SDL_MOUSEWHEEL){if(e.wheel.y)history_action(e.wheel.y>0?4:5);}
                    else if(e.type==SDL_KEYDOWN&&!e.key.repeat){if(e.key.keysym.sym==SDLK_UP)history_action(2);else if(e.key.keysym.sym==SDLK_DOWN)history_action(3);else if(e.key.keysym.sym==SDLK_PAGEUP)history_action(8);else if(e.key.keysym.sym==SDLK_PAGEDOWN)history_action(9);else if(e.key.keysym.sym==SDLK_HOME)history_action(10);else if(e.key.keysym.sym==SDLK_END)history_action(11);else{for(int vk:virtual_keys(e.key.keysym)){if(settings.key_matches("SystemKeySetHistoryMode",vk,false)||settings.key_matches("SystemKeySetDecide",vk,false)||vk==27){history_action(7);break;}}}}
                    else if(e.type==SDL_MOUSEBUTTONDOWN){if(e.button.button==SDL_BUTTON_RIGHT)history_action(7);else if(e.button.button==SDL_BUTTON_LEFT)history_click(e.button.x,e.button.y);}continue;
                }
                if(wait==Wait::Reveal&&((e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT)||(e.type==SDL_KEYDOWN&&(e.key.keysym.sym==SDLK_RETURN||e.key.keysym.sym==SDLK_SPACE))))finish_reveal();
                if(system_interaction(e))continue;
                if(wait==Wait::Text){
                    if((e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT)||(e.type==SDL_MOUSEWHEEL&&e.wheel.y<0))advance();
                    if(e.type==SDL_KEYDOWN){bool decide=false;for(int vk:virtual_keys(e.key.keysym))decide|=settings.key_matches("SystemKeySetDecide",vk,e.key.repeat)||settings.key_matches("SystemKeySetKeyInSkip",vk,e.key.repeat);if(decide||(e.key.keysym.sym==SDLK_RETURN&&!settings.state().records.count("SystemKeySetDecide")))advance();}
                }
                if(wait==Wait::Selection&&!collection) {
                    if(selector_event(e))continue;
                    if(e.type==SDL_MOUSEMOTION)hover_at(e.motion.x,e.motion.y);
                    if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT)click(e.button.x,e.button.y);
                    if((e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_RIGHT)||(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_ESCAPE))cancel_selection();
                }
            }
            if((history_mode||wait==Wait::Input||wait==Wait::Text||wait==Wait::TextHidden||(wait==Wait::Selection&&!collection))&&!actions.empty()&&action_index<actions.size()&&!action_pause) {
                std::istringstream action(actions[action_index++]);std::string op;action>>op;
                if(op=="progress"){uint64_t page;action>>page;if(text_pages<page){if(wait==Wait::Selection){for(const auto& item:selector.items())if(item.kind==SelectionItem::Mouse&&!item.blocked&&!item.ignore_input){finish_selection(item.branch);break;}}else advance();--action_index;}}
                else if(op=="wait-ms"){uint64_t ms;action>>ms;if(!action_wait_until)action_wait_until=now+ms;if(now<action_wait_until)--action_index;else action_wait_until=0;}
                else if(op=="profile"){std::string label;size_t frames=0;double budget=0;action>>label>>frames;action>>budget;if(frame_profile.active())throw std::runtime_error("frame profile already active");frame_profile.begin(label,frames,budget,state_dir);}
                else if(op=="profile-wait"){if(frame_profile.active())--action_index;}
                else if(op=="wait-page"){uint64_t page;action>>page;if(text_pages<page)--action_index;}
                else if(op=="click"){int x,y;action>>x>>y;if(history_mode)history_click(x,y);else {SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.button=SDL_BUTTON_LEFT;event.button.state=SDL_PRESSED;event.button.x=x;event.button.y=y;if(!system_interaction(event)){if(wait==Wait::Selection)click(x,y);else {bool menu=false;for(const auto& item:configuration)if(item.first.rfind("AreaSetMouseLeftScript:",0)==0){Args b{item.second};if(b.size()>6&&x>=b.n(3)&&x<=b.n(5)&&y>=b.n(4)&&y<=b.n(6)){open_menu(b.s(1));menu=true;break;}}if(!menu)advance();}}}}
                else if(op=="key"){std::string name;std::getline(action,name);if(!name.empty()&&name[0]==' ')name.erase(name.begin());auto code=SDL_GetKeyFromName(name.c_str());if(code==SDLK_UNKNOWN)throw std::runtime_error("invalid action key name");SDL_Event event{};event.type=SDL_KEYDOWN;event.key.state=SDL_PRESSED;event.key.keysym.sym=code;event.key.keysym.scancode=SDL_GetScancodeFromKey(code);SDL_PushEvent(&event);event.type=SDL_KEYUP;event.key.state=SDL_RELEASED;SDL_PushEvent(&event);}
                else if(op=="input"){std::getline(action,input_value);if(!input_value.empty()&&input_value[0]==' ')input_value.erase(input_value.begin());if(wait!=Wait::Input)throw std::runtime_error("input action without dialog");finish_input(true);}
                else if(op=="history")show_history();else if(op=="history-close")close_history();else if(op=="history-action"){int label;action>>label;history_action(label);}else if(op=="hover"){int x,y;action>>x>>y;if(history_mode)history_hover_at(x,y);else hover_at(x,y);}else if(op=="advance"){if(history_mode)history_action(3);else advance();}else if(op=="cancel"){if(history_mode)history_action(7);else if(wait==Wait::Input)finish_input(false);else cancel_selection();}
                else if(op=="menu"){std::string name;action>>name;open_menu("yks\\system\\option\\m_"+name+".yks");}
                else if(op=="save"){int slot;action>>slot;save_slot(slot,"");}
                else if(op=="load"||op=="load-original"){int slot;action>>slot;load_slot(slot,op=="load-original");}
                else if(op=="capture"){std::string path;action>>path;scene.capture(path);}
                else if(op=="quit")quit=true;
                else throw std::runtime_error("invalid runtime action: "+op);
                action_pause=true;
            } else action_pause=false;
            if(wait==Wait::Text&&!history_mode) {
                if(auto_advance>0){--auto_advance;advance();}
                else if(until_text){if(!screenshot.empty())scene.capture(screenshot);quit=true;}
            }
            if(auto_new&&wait==Wait::Selection&&!collection&&selection_script=="yks\\title\\title_main.yks"){auto_new=0;click(200,370);}
            if(false&&wait==Wait::Selection&&!collection&&left_down){for(auto it=regions.rbegin();it!=regions.rend();++it)if(it->kind=="MouseKeyCase"&&Args{it->args}.n(0)==1&&inside(*it,mouse_x,mouse_y)){vm.choose(it->branch);wait=Wait::None;collection=false;break;}}
            if(pending_restart){auto entry=*pending_restart;pending_restart.reset();restart_engine(entry);}
            if(!pending_menu.empty()) {menu_vm=vm.snapshot();menu_runtime=runtime_state();menu_thumbnail=scene.thumbnail();menu_native_vm=vm.export_original();menu_native_scene=native_scene();auto name=pending_menu;pending_menu.clear();vm.call(name,true);in_menu=true;wait=Wait::None;}
            if(wait==Wait::Save){if(pending_subcall||vm.pending_subcall()){menu_vm=vm.snapshot_after_subcall();{auto old_wait=wait;wait=Wait::None;menu_runtime=runtime_state();wait=old_wait;}menu_thumbnail=scene.thumbnail();{Vm checkpoint([this](const std::string& name){return translation_patch.apply(parse_script(resources.read(name)),name);});checkpoint.restore(menu_vm);menu_native_vm=checkpoint.export_original();}menu_native_scene=native_scene();{const bool old_menu=in_menu;in_menu=true;save_slot(0,"","savedata.---");in_menu=old_menu;}pending_subcall=false;vm.resume();wait=Wait::None;}else{vm.resume(int32_t(1));wait=Wait::None;save_slot(pending_slot,pending_comment);}}
            if(wait==Wait::Load){load_slot(pending_slot,force_native);if(load_to_draw_stop){snapshot();drawing=false;}}
            bool completed=false;
            if(wait==Wait::Time||wait==Wait::CdTime)completed=now>=deadline;
            else if(wait==Wait::Audio)completed=!audio.playing(wait_id);
            else if(wait==Wait::Motion)completed=!motions.count({wait_id,true})&&!motions.count({wait_id,false})&&(!layer_motion.count(wait_id)||layer_motion.at(wait_id).scroll_mode==0);
            else if(wait==Wait::Sprite){auto it=sprites.find({wait_id,wait_slot});completed=it==sprites.end()||it->second.done;}
            if(completed){vm.resume();wait=Wait::None;}
            if(!history_mode&&(wait==Wait::None||collection)) {
                const auto vm_start=std::chrono::steady_clock::now();
                const auto status=vm.run(2000);
                profile_vm_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-vm_start).count();
                if(configuration.count("LoadHookParentWait")&&!vm.in_call()){auto parent_wait=Wait(number(configuration.at("LoadHookParentWait")[0]));if(parent_wait!=Wait::None)wait=parent_wait;configuration.erase("LoadHookParentWait");}
                if(selection_event&&status==VmStatus::Waiting&&wait==Wait::None){selection_event=false;wait=Wait::Selection;}
                if(in_menu&&!vm.in_call()&&status==VmStatus::Waiting){persist();in_menu=false;if(menu_resume_context){auto restore=std::move(menu_resume_context);menu_resume_context={};restore();}else{wait=Wait::Text;collection=false;regions.clear();hover=-1;}}
                if(status==VmStatus::Halted){quit=true;}
                if(status==VmStatus::Waiting&&wait==Wait::Selection){if(collection){if(text_selection)draw_choices();rebuild_selector();}collection=false;if(selection_checkpoint){selection_checkpoint=false;save_slot(0,"");}
                    if(!title&&selection_script=="yks\\title\\title_main.yks"){
                        title=true;
                        std::cout<<"TITLE_READY layers="<<layers.size()<<" sprites="<<sprites.size()<<" regions="<<regions.size()<<" steps="<<vm.steps()<<'\n';
                        if(hover_test_x>=0)hover_at(hover_test_x,hover_test_y);
                        if(!screenshot.empty())scene.capture(screenshot);
                        if(initial_native>=0){pending_slot=initial_native;initial_native=-1;wait=Wait::Load;}else if(until_title){render();quit=true;}
                    }
                }
            }
            const auto render_start=std::chrono::steady_clock::now();
            if(!fast||wait==Wait::Text||(wait==Wait::Selection&&!collection))render();
            const auto render_end=std::chrono::steady_clock::now();
            // Preserve the existing ~8ms polling cadence without adding a
            // second fixed delay to busy script/GPU frames (or vsync waits).
            const auto work_ms=std::chrono::duration<double,std::milli>(render_end-frame_start).count();
            SDL_Delay(fast?0:Uint32(std::max(0.0,8.0-work_ms)));
            const auto frame_end=std::chrono::steady_clock::now();
            if(measure){auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};auto after=scene.render_stats();profile_failed=frame_profile.add({ms(frame_start,frame_end),ms(frame_start,render_start),ms(render_start,render_end),profile_present_ms,ms(render_end,frame_end),after.gpu_operations-before_stats.gpu_operations,after.texture_updates-before_stats.texture_updates,after.image_decodes-before_stats.image_decodes,vm.steps()-before_vm_steps,profile_vm_ms},std::cout)||profile_failed;}

        }
        {std::vector<std::pair<std::string,HostTiming>> timings(host_timings.begin(),host_timings.end());std::sort(timings.begin(),timings.end(),[](const auto& a,const auto& b){return a.second.ms>b.second.ms;});for(const auto& [name,t]:timings)std::cout<<"HOST_PROFILE name="<<name<<" calls="<<t.calls<<" total_ms="<<t.ms<<" max_ms="<<t.max_ms<<'\n';}
        auto audio_metrics=audio.decode_stats();std::cout<<"AUDIO_STATS sources="<<audio_metrics.sources<<" pcm_capacity="<<audio_metrics.pcm_capacity_bytes<<" queued_frames="<<audio_metrics.queued_frames<<" decoded_frames="<<audio_metrics.decoded_frames<<" underruns="<<audio_metrics.underruns<<'\n';
        auto metrics=scene.render_stats();std::cout<<"RENDER_STATS uploads="<<metrics.uploads<<" updates="<<metrics.texture_updates<<" shared="<<metrics.shared_loads<<" snapshots="<<metrics.snapshots<<" readbacks="<<metrics.readbacks<<" decodes="<<metrics.image_decodes<<" cache_hits="<<metrics.resource_cache_hits<<" cache_bytes="<<metrics.resource_cache_bytes<<" gpu_operations="<<metrics.gpu_operations<<" compositions="<<metrics.compositions<<" composition_reuses="<<metrics.composition_reuses<<" text_layout_reuses="<<metrics.text_layout_reuses<<" text_appends="<<metrics.text_appends<<" readback_pixels="<<metrics.readback_pixels<<'\n';
        persist();
        if(frame_profile.active())throw std::runtime_error("frame profile interrupted");
        if(profile_failed)throw std::runtime_error("frame profile exceeded requested budget");
        if(until_title&&!title)throw std::runtime_error("window closed before reaching title");
        return 0;
    }
};
}
int play_game(int argc,char** argv) {
    bool require_gpu=false,force_native=false,until=false,fast=false,mute=false,trace=false,start_new=false,stop_text=false;int advances=0,load_initial=-1;uint64_t timeout=0;uint32_t seed=std::random_device{}();
    NativeFontOptions font_options;std::string encoding="CP932";bool encoding_explicit=false;std::filesystem::path patch,original_executable,action_file,screenshot,state=std::filesystem::current_path()/".yukari";int hx=-1,hy=-1;
    for(int i=3;i<argc;++i){std::string a=argv[i];
        if(a=="--font-directory"&&i+1<argc)font_options.font_directory=argv[++i];else if(a=="--gdi-cache"&&i+1<argc)font_options.gdi_cache_directory=argv[++i];else if(a=="--require-gdi-capture")font_options.require_gdi_capture=true;else if(a=="--require-gpu")require_gpu=true;else if(a=="--load-original-slot"&&i+1<argc){load_initial=std::stoi(argv[++i]);force_native=true;}else if(a=="--original-encoding"&&i+1<argc){encoding=argv[++i];encoding_explicit=true;if(encoding!="CP932"&&encoding!="CP936")throw std::runtime_error("original encoding must be CP932 or CP936");}else if(a=="--load-slot"&&i+1<argc)load_initial=std::stoi(argv[++i]);else if(a=="--actions"&&i+1<argc)action_file=argv[++i];else if(a=="--new-game")start_new=true;else if(a=="--until-text")stop_text=true;else if(a=="--advance"&&i+1<argc)advances=std::stoi(argv[++i]);else if(a=="--until-title")until=true;else if(a=="--fast")fast=true;else if(a=="--mute")mute=true;else if(a=="--trace")trace=true;
        else if(a=="--translation-patch"&&i+1<argc)patch=argv[++i];
        else if(a=="--original-exe"&&i+1<argc)original_executable=argv[++i];
        else if(a=="--screenshot"&&i+1<argc)screenshot=argv[++i];else if(a=="--state-dir"&&i+1<argc)state=argv[++i];
        else if(a=="--max-ms"&&i+1<argc)timeout=std::stoull(argv[++i]);else if(a=="--seed"&&i+1<argc)seed=uint32_t(std::stoul(argv[++i]));
        else if(a=="--hover"&&i+2<argc){hx=std::stoi(argv[++i]);hy=std::stoi(argv[++i]);}
        else throw std::runtime_error("invalid play argument: "+a);
    }
    const bool automatic_patch=patch.empty();
    patch=find_translation_patch(argv[2],patch);
    if(!patch.empty()){
        const TranslationPatch metadata(patch);
        if(!encoding_explicit&&metadata.encoding())encoding=*metadata.encoding();
        std::cout<<"TRANSLATION_PATCH path="<<patch.string()<<" automatic="<<(automatic_patch?1:0)<<" encoding="<<encoding<<"\n";
    }
    Runtime runtime(argv[2],state,seed,fast,font_options,original_executable,patch);
    return runtime.run(require_gpu,force_native,encoding,load_initial,action_file,until,screenshot,timeout,mute,trace,hx,hy,start_new,stop_text,advances);
}
}

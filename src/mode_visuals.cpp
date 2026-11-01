#include "yukari/mode_visuals.hpp"
#include <stdexcept>
#include <utility>
namespace yukari {
namespace {
unsigned index(ModeIndicator mode){const auto n=unsigned(mode);if(n>=4)throw std::runtime_error("invalid mode indicator");return n;}
unsigned index(ModeGraphic mode){const auto n=unsigned(mode);if(n>=2)throw std::runtime_error("invalid mode graphic");return n;}
}
ModeVisuals::ModeVisuals(ModeVisualHooks hooks):hooks_(std::move(hooks)){}
void ModeVisuals::set_icon(ModeIndicator mode,ModeIconRecord record){state_.icons[index(mode)]=record;}
void ModeVisuals::set_graphic(ModeGraphic mode,int layer,std::string replacement){auto& g=state_.graphics[index(mode)];g.enabled=true;g.layer=layer;g.replacement=std::move(replacement);}
void ModeVisuals::disable_graphic(ModeGraphic mode){state_.graphics[index(mode)].enabled=false;}
bool ModeVisuals::icon_present(ModeIndicator mode)const{const auto n=index(mode);return state_.icons[n].configured()&&state_.icon_instances[n];}
void ModeVisuals::update_icon(ModeIndicator mode,uint8_t requested){
    const auto n=index(mode);const bool present=icon_present(mode);
    // Native442880 compares the raw flag with the presence query, then acts
    // on current presence. Ordinary flags are0/1; do not normalize raw bytes.
    if(requested==uint8_t(present))return;
    if(present){
        state_.icon_instances[n]=false;
        if(!hooks_.remove_icon)throw std::runtime_error("mode indicator removal requires a hook");
        hooks_.remove_icon(mode,state_.icons[n]);
    }else if(!state_.icon_instances[n]&&state_.icons[n].configured()){
        if(!hooks_.create_icon)throw std::runtime_error("mode indicator creation requires a hook");
        state_.icon_instances[n]=hooks_.create_icon(mode,state_.icons[n]);
    }
}
void ModeVisuals::update_graphic(ModeGraphic mode,uint8_t active){
    auto& g=state_.graphics[index(mode)];if(!g.enabled)return;
    if(active==1){
        if(g.applied)return;
        if(!hooks_.layer_resource)throw std::runtime_error("mode image swap requires a layer-resource hook");
        auto current=hooks_.layer_resource(g.layer);if(!current)return;
        if(!hooks_.reload_image)throw std::runtime_error("mode image swap requires a reload hook");
        g.backup=*current;hooks_.reload_image(g.layer,g.replacement,{});g.applied=true;
    }else if(g.applied){
        if(!hooks_.layer_resource)throw std::runtime_error("mode image restore requires a layer-resource hook");
        if(!hooks_.layer_resource(g.layer))return;
        if(!hooks_.reload_image)throw std::runtime_error("mode image restore requires a reload hook");
        hooks_.reload_image(g.layer,g.backup,{});g.applied=false;
    }
}
void ModeVisuals::update(uint8_t automatic,uint8_t already_skip,uint8_t skip){
    update_icon(ModeIndicator::Auto,automatic);update_icon(ModeIndicator::AlreadySkip,already_skip);update_icon(ModeIndicator::Skip,skip);
    update_graphic(ModeGraphic::Auto,automatic);update_graphic(ModeGraphic::Skip,uint8_t(already_skip|skip));
}
void ModeVisuals::update_already(bool read){update_icon(ModeIndicator::Already,uint8_t(read));}
void ModeVisuals::restore(ModeVisualState state){state_=std::move(state);}
void ModeVisuals::clear_icon_instances(){state_.icon_instances={};}
std::pair<int32_t,int32_t> key_wait_icon_position(int32_t mode,int32_t dx,int32_t dy,const TextFlow& f,int32_t fallback_x,int32_t fallback_y){
    switch(mode){
    case 0:return {TextFlow::add(f.right,-30),TextFlow::add(f.bottom,-30)};
    case 1:return {dx,dy};
    case 2:return {TextFlow::add(f.left,dx),TextFlow::add(f.top,dy)};
    case 3:return {TextFlow::add(f.right,dx),TextFlow::add(f.bottom,dy)};
    case 4:return {TextFlow::add(f.x,dx),TextFlow::add(f.y,dy)};
    default:return {fallback_x,fallback_y};
    }
}
}

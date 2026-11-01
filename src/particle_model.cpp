#include "yukari/particle_model.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace yukari {
namespace {
int32_t signed_bits(uint32_t value){int32_t out;std::memcpy(&out,&value,4);return out;}
int32_t integer(double value){return std::isfinite(value)&&value>=INT32_MIN&&value<2147483648.0?int32_t(value):INT32_MIN;}
int32_t add(int32_t a,int32_t b){return signed_bits(uint32_t(a)+uint32_t(b));}
void dimensions(ParticleModel::Particle& p){const auto& f=p.frame();p.width=signed_bits(f.w);p.height=signed_bits(f.h);}
}
const Frame& ParticleModel::Particle::frame()const {
    const auto& group=sprite_motion_group(*groups,motion);
    if(motion.frame>=group.size())throw std::runtime_error("particle sprite frame out of bounds");
    return group[motion.frame];
}
void ParticleModel::set_random(std::function<uint32_t()> random){random_=std::move(random);}
uint32_t ParticleModel::random(){
    if(!random_)throw std::runtime_error("particle effects require original CRT random stream");
    const auto value=random_();if(value>32767)throw std::runtime_error("particle CRT random value exceeds RAND_MAX");return value;
}
int32_t ParticleModel::range(int32_t first,int32_t last){
    // 0x429e10: inclusive interval, two CRT calls, rejection below 0x7fff7fff.
    if(first>=last)return last;
    const int64_t span=int64_t(last)-first+1;
    if(span>=0x7fff7fff)throw std::runtime_error("native particle random range cannot terminate");
    uint32_t value;do{const auto high=random();value=high*65536+random();}while(uint64_t(value)>=uint64_t(0x7fff7fff)-span);
    return int32_t(int64_t(value%uint32_t(span))+first);
}
void ParticleModel::configure(Mode mode,std::string resource,int32_t period,Rect rect,const std::vector<int>& ids,SpriteGroups groups){
    if(mode!=Mode::Snow&&mode!=Mode::Sakura)throw std::runtime_error("invalid particle mode");
    if(period<=0||rect.right<=rect.left||rect.bottom<0||ids.empty()||ids.size()>10)
        throw std::runtime_error("invalid native particle configuration");
    if(mode==Mode::Sakura&&(rect.right>INT32_MAX/2||rect.right<0))throw std::runtime_error("Sakura random interval overflows");
    for(size_t slot=0;slot<ids.size();++slot){SpriteMotion motion;motion.group=int(slot);sprite_motion_reset(groups,motion,0);}
    mode_=mode;resource_=std::move(resource);period_=period;rect_=rect;countdown_=0;slots_=ids;
    groups_=std::make_shared<const SpriteGroups>(std::move(groups));
    for(int id:slots_)layers_.try_emplace(id);
    // Native configuration deliberately leaves DAT_4cdb8c (wind) unchanged.
}
void ParticleModel::stop_snow(){mode_=Mode::None;}
std::vector<int> ParticleModel::stop_sakura(){
    mode_=Mode::None;std::vector<int> removed;
    for(int id:slots_)if(id>=0){layers_.erase(id);removed.push_back(id);}
    slots_.clear();return removed;
}
void ParticleModel::erase_layer(int id){layers_.erase(id);}
void ParticleModel::clear(){mode_=Mode::None;resource_.clear();groups_.reset();period_=countdown_=wind_=0;slots_.clear();layers_.clear();next_serial_=1;rejected_spawns_=0;}
ParticleModel::Particle ParticleModel::spawn(size_t slot){
    Particle p;p.serial=next_serial_++;p.resource=resource_;p.groups=groups_;p.motion.group=int(slot);p.motion.mode=0;
    sprite_motion_reset(*p.groups,p.motion,0);dimensions(p);
    if(mode_==Mode::Snow){
        const auto width=int64_t(rect_.right)-rect_.left;
        p.x=int32_t(int64_t(random()%uint64_t(width))+rect_.left);p.y=signed_bits(0u-uint32_t(p.height));
        p.velocity_y=0.05/double(slot+1);
        const int divisor=int(random()%10)-5;
        // Native IEEE division by zero is retained; x87 conversion later yields INT_MIN.
        p.velocity_x=divisor?(p.velocity_y*0.5)/divisor:std::numeric_limits<double>::infinity();
    }else{
        p.x=range(-rect_.right,signed_bits(uint32_t(rect_.right)*2));
        p.y=signed_bits(0u-uint32_t(add(range(0,rect_.bottom),p.height)));
        p.velocity_y=800.0/(double(slot)+3.0)/1000.0;
        p.velocity_x=double(range(-50,100))/1000.0;
    }
    p.precise_x=p.x;p.precise_y=p.y;return p;
}
bool ParticleModel::tick(uint32_t delta,uint32_t accumulated,bool deferred,const Position& position){
    uint32_t spawn_count=0;
    if(mode_!=Mode::None&&delta){
        countdown_=signed_bits(uint32_t(countdown_)-delta);
        if(countdown_<=0){
            const auto count=uint64_t(-int64_t(countdown_))/uint32_t(period_)+1;
            spawn_count=uint32_t(count);countdown_=int32_t(int64_t(countdown_)+int64_t(count)*period_);
        }
    }
    bool changed=false;const uint32_t movement_delta=delta+accumulated;
    for(auto& [id,layer]:layers_){
        const auto found=std::find(slots_.begin(),slots_.end(),id);
        if(spawn_count&&found!=slots_.end()){
            const size_t slot=size_t(found-slots_.begin());layer.enabled=true;
            for(uint32_t n=0;n<spawn_count;++n){
                // Capacity4130b0 rejects >=1000, while Append412dc0 uses
                // count+1<1000: the 1000th allocated instance is discarded.
                if(layer.particles.size()>=1000)continue;
                auto p=spawn(slot);
                if(layer.particles.size()+1<1000)layer.particles.push_back(std::move(p));else ++rejected_spawns_;
            }
            if(mode_==Mode::Sakura){
                if(range(0,1000)<50)wind_=add(wind_,range(-1,2));
                if(uint32_t(wind_)+5>10||range(0,500)==0)wind_=range(0,1)?1:-1;
            }
        }
        if(!layer.enabled)continue;
        const double layer_y=position?position(id).second:0;
        for(size_t i=0;i<layer.particles.size();++i){
            auto& p=layer.particles[i];
            if(!deferred&&(p.velocity_x!=0||p.velocity_y!=0)){
                p.precise_x+=double(movement_delta)*p.velocity_x;p.precise_y+=double(movement_delta)*p.velocity_y;
                if(mode_==Mode::Sakura)p.velocity_x+=double(wind_)/5000.0;
                const int32_t x=integer(p.precise_x),y=integer(p.precise_y);
                if(x!=p.x||y!=p.y){
                    p.x=x;p.y=y;changed=true;
                    if(layer_y+p.y>rect_.bottom){
                        layer.particles.erase(layer.particles.begin()+i);
                        // Original Remove compacts, then the outer loop still
                        // increments its index. The new current slot receives
                        // animation handling but no position integration.
                        if(i<layer.particles.size()){
                            auto& next=layer.particles[i];next.animation_clock+=delta;
                            if(sprite_motion_tick(*next.groups,next.motion,next.animation_clock)){dimensions(next);changed=true;}
                        }
                        continue;
                    }
                }
            }
            p.animation_clock+=delta;
            if(sprite_motion_tick(*p.groups,p.motion,p.animation_clock)){dimensions(p);changed=true;}
        }
    }
    return changed;
}
}

namespace yukari {
Bytes ParticleModel::snapshot()const{
    Bytes out={'Y','U','K','A','P','A','R','T'};
    auto put=[&](uint64_t value,unsigned n=4){for(unsigned i=0;i<n;++i)out.push_back(uint8_t(value>>(8*i)));};
    auto string=[&](const std::string& value){put(value.size());out.insert(out.end(),value.begin(),value.end());};
    auto number=[&](double value){uint64_t bits;std::memcpy(&bits,&value,8);put(bits,8);};
    put(1);put(unsigned(mode_));string(resource_);put(bool(groups_));
    for(int32_t value:{period_,countdown_,wind_,rect_.left,rect_.top,rect_.right,rect_.bottom})put(uint32_t(value));
    put(next_serial_,8);put(rejected_spawns_,8);put(slots_.size());for(int id:slots_)put(uint32_t(id));
    put(layers_.size());for(const auto& [id,layer]:layers_){
        put(uint32_t(id));put(layer.enabled);put(layer.particles.size());
        for(const auto& p:layer.particles){
            put(p.serial,8);string(p.resource);put(uint32_t(p.motion.group));put(p.motion.frame,8);put(p.motion.next,8);put(p.motion.done);put(uint32_t(p.motion.mode));
            for(double value:{p.precise_x,p.precise_y,p.velocity_x,p.velocity_y})number(value);
            for(int32_t value:{p.x,p.y,p.width,p.height})put(uint32_t(value));put(p.animation_clock,8);
        }
    }
    return out;
}
void ParticleModel::restore(const Bytes& bytes,const ResourceLoader& loader){
    if(bytes.size()<12||std::memcmp(bytes.data(),"YUKAPART",8))throw std::runtime_error("invalid particle snapshot");
    size_t at=8;auto get=[&](unsigned n=4){if(n>bytes.size()-at)throw std::runtime_error("truncated particle snapshot");uint64_t value=0;for(unsigned i=0;i<n;++i)value|=uint64_t(bytes[at++])<<(8*i);return value;};
    auto string=[&](){const auto count=get();if(count>1024*1024||count>bytes.size()-at)throw std::runtime_error("invalid particle snapshot string");std::string value(bytes.begin()+at,bytes.begin()+at+count);at+=count;return value;};
    auto number=[&](){const auto bits=get(8);double value;std::memcpy(&value,&bits,8);return value;};
    auto boolean=[&](){const auto value=get();if(value>1)throw std::runtime_error("invalid particle snapshot boolean");return bool(value);};
    if(get()!=1)throw std::runtime_error("unsupported particle snapshot version");
    ParticleModel model;model.random_=random_;const auto mode=get();if(mode>2)throw std::runtime_error("invalid particle snapshot mode");model.mode_=Mode(mode);model.resource_=string();const bool has_groups=boolean();
    for(auto* value:{&model.period_,&model.countdown_,&model.wind_,&model.rect_.left,&model.rect_.top,&model.rect_.right,&model.rect_.bottom})*value=signed_bits(uint32_t(get()));
    model.next_serial_=get(8);model.rejected_spawns_=get(8);
    const auto slots=get();if(slots>10)throw std::runtime_error("invalid particle snapshot slots");for(size_t i=0;i<slots;++i)model.slots_.push_back(signed_bits(uint32_t(get())));
    std::map<std::string,std::shared_ptr<const SpriteGroups>> resources;
    auto resource=[&](const std::string& name){auto found=resources.find(name);if(found!=resources.end())return found->second;if(!loader)throw std::runtime_error("particle snapshot requires resource loader");auto groups=std::make_shared<const SpriteGroups>(loader(name));resources.emplace(name,groups);return groups;};
    if(has_groups)model.groups_=resource(model.resource_);
    if(model.mode_!=Mode::None&&(!model.groups_||model.period_<=0||model.rect_.right<=model.rect_.left||model.rect_.bottom<0))throw std::runtime_error("invalid particle snapshot configuration");
    const auto layers=get();if(layers>1000000)throw std::runtime_error("particle snapshot layer limit exceeded");
    for(size_t l=0;l<layers;++l){
        const int id=signed_bits(uint32_t(get()));if(model.layers_.count(id))throw std::runtime_error("duplicate particle snapshot layer");auto& layer=model.layers_[id];layer.enabled=boolean();const auto count=get();if(count>999)throw std::runtime_error("particle snapshot instance limit exceeded");
        for(size_t i=0;i<count;++i){
            Particle p;p.serial=get(8);p.resource=string();p.groups=resource(p.resource);p.motion.group=signed_bits(uint32_t(get()));p.motion.frame=size_t(get(8));p.motion.next=get(8);p.motion.done=boolean();p.motion.mode=signed_bits(uint32_t(get()));
            for(auto* value:{&p.precise_x,&p.precise_y,&p.velocity_x,&p.velocity_y})*value=number();
            for(auto* value:{&p.x,&p.y,&p.width,&p.height})*value=signed_bits(uint32_t(get()));p.animation_clock=get(8);
            const auto& f=p.frame();if(p.width!=signed_bits(f.w)||p.height!=signed_bits(f.h))throw std::runtime_error("particle snapshot resource dimensions changed");
            layer.particles.push_back(std::move(p));
        }
    }
    if(at!=bytes.size())throw std::runtime_error("trailing particle snapshot bytes");
    if(model.groups_)for(size_t i=0;i<model.slots_.size();++i){SpriteMotion motion;motion.group=int(i);sprite_motion_reset(*model.groups_,motion,0);}
    *this=std::move(model);
}
}

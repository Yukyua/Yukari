#pragma once
#include "yukari/formats.hpp"
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>

namespace yukari {
using SpriteGroups = std::vector<std::vector<Frame>>;
struct SpriteMotion {
    int group{};
    size_t frame{};
    uint64_t next{};
    bool done{};
    int mode{2};
};
inline SpriteGroups split_sprite_groups(const std::vector<Frame>& frames) {
    SpriteGroups groups;std::vector<Frame> group;
    for(const auto& f:frames) {
        if(!(f.x||f.y||f.w||f.h||f.duration||f.type||f.unknown||f.action)) {
            groups.push_back(std::move(group));group.clear();
        } else group.push_back(f); // Zero-size action records are executable.
    }
    if(!group.empty())groups.push_back(std::move(group));
    return groups;
}
inline const std::vector<Frame>& sprite_motion_group(const SpriteGroups& groups,const SpriteMotion& s) {
    if(s.group<0||size_t(s.group)>=groups.size()||groups[size_t(s.group)].empty())throw std::runtime_error("sprite motion group not found");
    return groups[size_t(s.group)];
}
inline int sprite_motion_mode(const SpriteMotion& s) {return s.mode>=102&&s.mode<=107?s.mode-100:s.mode;}
inline bool sprite_motion_advance(const SpriteGroups& groups,SpriteMotion& s) {
    const auto count=sprite_motion_group(groups,s).size();
    if(s.frame>=count)throw std::runtime_error("sprite frame out of bounds");
    const int mode=sprite_motion_mode(s);
    if(count<=1){s.done=true;return false;}
    if(mode==2||mode==6){s.frame=s.frame+1<count?s.frame+1:0;return true;}
    if(mode==3||mode==7){s.frame=s.frame?s.frame-1:count-1;return true;}
    if(mode==4&&s.frame+1<count){++s.frame;return true;}
    if(mode==5&&s.frame){--s.frame;return true;}
    s.done=true;return false;
}
// Native 0x40a390 applies actions before rectangle/mode/wait data. Action 1's
// x is a frame index; action 2's x is a group index. +100 modes carry elapsed
// time into the next frame, while ordinary modes replace their wait duration.
inline const Frame& sprite_motion_apply(const SpriteGroups& groups,SpriteMotion& s,uint64_t now) {
    std::set<std::tuple<int,size_t,int>> zero_chain;
    for(;;) {
        if(!zero_chain.emplace(s.group,s.frame,s.mode).second)throw std::runtime_error("sprite motion zero-wait/action cycle");
        const auto& group=sprite_motion_group(groups,s);
        if(s.frame>=group.size())throw std::runtime_error("sprite frame out of bounds");
        const auto& f=group[s.frame];
        if(f.action==1){s.frame=f.x;continue;}
        if(f.action==2){if(f.x>uint32_t(std::numeric_limits<int>::max()))throw std::runtime_error("sprite group jump out of range");s.group=int(f.x);s.frame=0;continue;}
        if(f.action!=0)throw std::runtime_error("unsupported sprite motion action");
        if(f.type){if(f.type>uint32_t(std::numeric_limits<int>::max()))throw std::runtime_error("sprite mode out of range");s.mode=int(f.type);}
        const auto base=s.mode>=102&&s.mode<=107?s.next:now;
        const uint64_t duration=group.size()==1?uint64_t(0x7fffffff):f.duration;
        if(duration>std::numeric_limits<uint64_t>::max()-base)throw std::runtime_error("sprite motion deadline overflow");
        s.next=base+duration;
        const int mode=sprite_motion_mode(s);
        s.done=group.size()==1||mode==1||(mode==4&&s.frame+1==group.size())||(mode==5&&s.frame==0);
        if(duration||s.done)return f;
        // Native reset immediately consumes zero-duration records. A terminal
        // single-shot frame holds even when its own wait duration is zero.
        if(!sprite_motion_advance(groups,s))return f;
    }
}
inline const Frame& sprite_motion_reset(const SpriteGroups& groups,SpriteMotion& s,uint64_t now) {
    const auto count=sprite_motion_group(groups,s).size();
    const auto mode=sprite_motion_mode(s);
    if(mode==4)s.frame=0;else if(mode==5)s.frame=count-1;
    s.done=false;s.next=now;
    return sprite_motion_apply(groups,s,now);
}
inline bool sprite_motion_tick(const SpriteGroups& groups,SpriteMotion& s,uint64_t now) {
    bool changed=false;size_t budget=100000;
    while(!s.done&&now>=s.next) {
        if(!budget--)throw std::runtime_error("sprite motion catch-up exceeds limit");
        if(!sprite_motion_advance(groups,s))break;
        sprite_motion_apply(groups,s,now);changed=true;
        // Ordinary modes set their next deadline relative to now. Accumulated
        // modes may need more than one advance after a slow rendered frame.
    }
    return changed;
}
}

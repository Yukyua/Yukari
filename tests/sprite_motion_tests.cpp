#include "yukari/sprite_motion.hpp"
#include <cassert>
#include <functional>
#include <iostream>
using namespace yukari;
namespace {
Frame image(uint32_t x,uint32_t duration,uint32_t mode=0){return {x,0,8,8,duration,mode,0,0};}
Frame jump(uint32_t target,uint32_t action){return {target,0,0,0,0,0,0,action};}
void rejects(const std::function<void()>& f){bool threw=false;try{f();}catch(const std::runtime_error&){threw=true;}assert(threw);}
}
int main(){
 auto groups=split_sprite_groups({jump(1,1),image(8,10,4),Frame{},image(16,0,1)});assert(groups.size()==2&&groups[0].size()==2&&groups[0][0].action==1);SpriteMotion s;auto f=sprite_motion_reset(groups,s,100);assert(s.frame==1&&f.x==8&&s.done);
 groups={{jump(2,1),image(8,20,2),image(16,15,4)}};s={};f=sprite_motion_reset(groups,s,0);assert(s.frame==2&&f.x==16&&s.done);
 groups={{jump(1,2)},{image(24,5,2),image(32,5)}};s={};f=sprite_motion_reset(groups,s,7);assert(s.group==1&&s.frame==0&&f.x==24&&s.next==12);assert(sprite_motion_tick(groups,s,12)&&s.frame==1);assert(sprite_motion_tick(groups,s,17)&&s.frame==0);
 groups={{image(0,0,2),image(8,0),image(16,20)}};s={};sprite_motion_reset(groups,s,100);assert(s.frame==2&&s.next==120);assert(sprite_motion_tick(groups,s,120)&&s.frame==2&&s.next==140);
 groups={{image(0,10,5),image(8,10,5),image(16,10,5)}};s={};s.mode=5;sprite_motion_reset(groups,s,100);assert(s.frame==2&&!s.done);assert(sprite_motion_tick(groups,s,110)&&s.frame==1);assert(sprite_motion_tick(groups,s,120)&&s.frame==0&&s.done);assert(!sprite_motion_tick(groups,s,999));
 // SpriteCreate explicitly starts in mode 2; a first record's mode 5 does
 // not retroactively change the reset index before that record is applied.
 s={};sprite_motion_reset(groups,s,100);assert(s.frame==0&&s.done);
 groups={{image(0,10,3),image(8,10,3),image(16,10,3)}};s={};sprite_motion_reset(groups,s,0);assert(s.frame==0);assert(sprite_motion_tick(groups,s,10)&&s.frame==2);
 groups={{image(0,10,102),image(8,10),image(16,10)}};s={};sprite_motion_reset(groups,s,0);assert(sprite_motion_tick(groups,s,25)&&s.frame==2&&s.next==30);assert(sprite_motion_tick(groups,s,30)&&s.frame==0&&s.next==40);
 groups={{image(0,10,2),image(8,10),image(16,10)}};s={};sprite_motion_reset(groups,s,0);assert(sprite_motion_tick(groups,s,25)&&s.frame==1&&s.next==35);
 groups={{image(0,0,2)}};s={};sprite_motion_reset(groups,s,0);assert(s.done&&s.next==0x7fffffff);assert(!sprite_motion_tick(groups,s,10000000000ULL));
 groups={{jump(0,1)}};s={};rejects([&]{sprite_motion_reset(groups,s,0);});groups={{jump(2,2)}};s={};rejects([&]{sprite_motion_reset(groups,s,0);});groups={{image(0,0,2),image(8,0)}};s={};rejects([&]{sprite_motion_reset(groups,s,0);});groups={{jump(5,1),image(0,10)}};s={};rejects([&]{sprite_motion_reset(groups,s,0);});
 std::cout<<"native sprite actions, zero waits, reverse reset and timer carry passed\n";
}

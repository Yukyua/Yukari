#include "yukari/particle_model.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using yukari::ParticleModel;
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
yukari::SpriteGroups groups(size_t n=1){return yukari::SpriteGroups(n,{{0,0,2,4,100,1,0,0}});}
int main(){try{
    ParticleModel p;uint64_t calls=0;p.set_random([&]{++calls;return 6u;});
    p.configure(ParticleModel::Mode::Snow,"snow.ykg",10,{20,30,120,100},{7,9},groups(2));
    p.tick(1);require(p.layers().at(7).particles.size()==1&&p.layers().at(9).particles.size()==1,"snow must spawn one instance in each selected layer");
    const auto& first=p.layers().at(7).particles.front();
    require(first.x==26&&first.y==-3&&first.precise_y==-3.95&&first.velocity_x==0.025&&first.velocity_y==0.05,"snow native position/velocity arithmetic differs");
    require(p.layers().at(9).particles.front().velocity_y==0.025,"snow depth slot does not affect velocity");
    require(p.countdown()==9&&calls==4,"global countdown or CRT call count differs");
    p.tick(9);require(p.countdown()==10&&p.layers().at(7).particles.size()==2,"countdown boundary failed to spawn");
    p.stop_snow();const auto old=p.layers().at(7).particles.front().precise_y;
    p.tick(10);require(p.layers().at(7).particles.size()==2&&p.layers().at(7).particles.front().precise_y>old,"SnowEffect() should stop spawning but retain falling particles");
    p.configure(ParticleModel::Mode::Snow,"next.ykg",100,{0,0,100,100},{7},groups());
    p.tick(1);require(p.layers().at(7).particles.size()==3&&p.layers().at(7).particles.back().resource=="next.ykg"&&p.layers().at(7).particles.front().resource=="snow.ykg","reconfigure discarded or rebound existing sprites");
    auto x=p.layers().at(7).particles.front().precise_x;p.stop_snow();p.tick(10,50,true);
    require(p.layers().at(7).particles.front().precise_x==x,"deferred motion integrated positions");
    p.tick(10,50,false);require(std::abs(p.layers().at(7).particles.front().precise_x-x-1.5)<1e-10,"sprite delta must include accumulated raw milliseconds");
    p.clear();calls=0;p.set_random([&]{++calls;return 5u;});p.configure(ParticleModel::Mode::Snow,"snow",1,{0,0,100,INT32_MAX},{1},groups());
    p.tick(1);require(std::isinf(p.layers().at(1).particles.front().velocity_x)&&p.layers().at(1).particles.front().x==INT32_MIN,"snow zero-divisor IEEE/x87 result was normalized");
    p.clear();calls=0;p.set_random([&]{++calls;return 6u;});p.configure(ParticleModel::Mode::Snow,"snow",1,{0,0,100,INT32_MAX},{1},groups());
    p.tick(1000,0,true);require(p.layers().at(1).particles.size()==999&&p.rejected_spawns()==2&&calls==2002,"native Append strict 1000 limit or rejected-allocation RNG differs");
    p.clear();calls=0;p.configure(ParticleModel::Mode::Snow,"snow",1,{0,0,100,0},{1},groups());p.tick(2,0,true);p.stop_snow();
    p.tick(200);require(p.layers().at(1).particles.size()==1&&p.layers().at(1).particles.front().precise_y==-4,"compaction must skip following particle motion for this tick");
    p.tick(200);require(p.layers().at(1).particles.empty(),"particle crossing bottom was not removed");
    p.clear();calls=0;p.set_random([&]{++calls;return 0u;});p.configure(ParticleModel::Mode::Sakura,"sakura",100,{0,0,100,100},{2},groups());
    p.tick(1);const auto& sakura=p.layers().at(2).particles.front();
    require(sakura.x==-100&&sakura.y==-3&&std::abs(sakura.velocity_y-800.0/3/1000)<1e-12,"Sakura spawn geometry/velocity differs");
    require(p.wind()==-1&&calls==14&&std::abs(sakura.velocity_x+0.0502)<1e-12,"inclusive two-call RNG/wind update differs");
    const auto ids=p.stop_sakura();require(ids==std::vector<int>{2}&&p.layers().empty()&&p.mode()==ParticleModel::Mode::None,"SakuraEffect() must delete configured layers");
    p.clear();uint64_t random_index=0;p.set_random([&]{return random_index++<2?32767u:0u;});
    p.configure(ParticleModel::Mode::Sakura,"sakura",100,{0,0,100,100},{2},groups());p.tick(1,0,true);
    require(random_index==16&&p.layers().at(2).particles.front().x==-100,"RandomRange rejection failed to consume another CRT pair");
    p.clear();p.set_random([]{return 6u;});auto animated=groups();animated[0]={{0,0,2,4,5,2,0,0},{2,0,3,5,7,2,0,0}};
    p.configure(ParticleModel::Mode::Snow,"anim",100,{0,0,100,100},{1},animated);p.tick(5,0,true);
    require(p.layers().at(1).particles.front().motion.frame==1&&p.layers().at(1).particles.front().width==3,"particle sprite animation did not advance while position deferred");
    p.tick(7,0,true);require(p.layers().at(1).particles.front().motion.frame==0,"particle sprite loop failed");
    p.configure(ParticleModel::Mode::Sakura,"new-resource",20,{0,0,100,100},{2},groups());p.tick(2);
    const auto saved=p.snapshot();ParticleModel restored;restored.set_random([]{return 6u;});unsigned loads=0;
    restored.restore(saved,[&](const std::string& name){++loads;return name=="anim"?animated:groups();});
    require(loads==2&&restored.snapshot()==saved,"particle restore lost configuration, resources or double/animation state");
    p.tick(25);restored.tick(25);require(p.snapshot()==restored.snapshot(),"restored particle simulation diverged");
    auto broken=saved;broken.pop_back();const auto before=restored.snapshot();bool bad_snapshot=false;
    try{restored.restore(broken,[&](const std::string& name){return name=="anim"?animated:groups();});}catch(const std::runtime_error&){bad_snapshot=true;}
    require(bad_snapshot&&restored.snapshot()==before,"invalid particle restore changed existing model");
    p.clear();auto removal_animation=groups();removal_animation[0]={{0,0,2,4,5,2,0,0},{2,0,3,4,1000,2,0,0}};
    p.configure(ParticleModel::Mode::Snow,"anim",1,{0,0,100,0},{1},removal_animation);p.tick(1,0,true);p.stop_snow();p.tick(200);
    require(p.layers().at(1).particles.size()==1&&p.layers().at(1).particles.front().precise_y==-4&&p.layers().at(1).particles.front().motion.frame==1,
            "compacted successor must animate despite skipped position integration");
    bool refused=false;try{p.configure(ParticleModel::Mode::Snow,"bad",0,{0,0,100,100},{1},groups());}catch(const std::runtime_error&){refused=true;}require(refused,"zero countdown period accepted");
    std::cout<<"particle native generation, random, motion and lifecycle tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

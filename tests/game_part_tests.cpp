#include "yukari/game_part.hpp"
#include <cassert>
#include <iostream>
using namespace yukari;
int main(){
 std::string source="BackGraphicType,3\r\nSoundFile,bgm\\intro.ogg\tbgm\\body.ogg\r\nSoundNameGraphic,title.png\nSuccessBorder,2\nStartDelay,500\n1,1000,1\n1,1500,0\n2,1000,0\n0,50,1\n5,0,1\n";
 auto chart=parse_music_chart(Bytes(source.begin(),source.end()));assert(chart.background==3&&chart.success_border==2&&chart.sound_file=="bgm\\intro.ogg\tbgm\\body.ogg"&&chart.sound_name_graphic=="title.png");assert(chart.total_time==7000&&chart.lanes[0].size()==2&&chart.lanes[0][0].time==1500&&chart.lanes[0][0].flag&&chart.lanes[4].empty());
 MusicGameOptions options;options.character_variants[1]=true;MusicGame game(chart,options);std::array<bool,5> keys{};
 auto p=game.position(0,0,0);assert(p.y==292&&p.x==432&&p.visible);
 keys[0]=true;assert(game.tick(1350,keys).empty()); // exact lower bound is not a hit
 assert(game.tick(1400,keys).empty()); // holding does not re-trigger
 keys[0]=false;game.tick(1401,keys);keys[0]=true;auto events=game.tick(1500,keys);assert(events.size()==1&&events[0].hit&&events[0].character_group==1&&game.hits()==1&&!game.position(0,0,1500).visible);
 keys[1]=true;assert(game.tick(1650,keys).empty()); // exact upper bound is neither hit nor miss
 keys[1]=false;events=game.tick(1651,keys);assert(events.size()==1&&!events[0].hit&&events[0].lane==1&&events[0].character_group==5&&game.misses()==1);
 keys[0]=false;game.tick(1900,keys);keys[0]=true;events=game.tick(2000,keys);assert(events.size()==1&&events[0].hit&&game.successful());
 game.tick(7000,keys);assert(!game.finished());game.tick(7001,keys);assert(game.finished()&&game.hits()==2);
 auto exact=parse_music_chart(Bytes{'M','u','s','i','c','T','o','t','a','l','T','i','m','e',',','1','0'});assert(exact.total_time==10);
 MusicChart pile;pile.total_time=10000;pile.lanes[0]={{1000,false},{1001,false},{1002,false}};MusicGame delayed(pile);assert(delayed.tick(4000,{}).size()==1);assert(delayed.misses()==1);assert(delayed.tick(4000,{}).size()==1);assert(delayed.misses()==2); // no draining on a late frame
 MusicGame aborted(chart);aborted.tick(0,{},true);assert(aborted.finished()&&aborted.hits()==0);
 auto nul=parse_music_chart(Bytes{'1',',','5','0','0',',','0',0,'2',',','7'});assert(nul.lanes[0].size()==1&&nul.lanes[1].empty());
 std::cout<<"native chart, strict hit window, key edges, miss cadence, score and geometry passed\n";
}

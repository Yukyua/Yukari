#pragma once
#include "yukari/formats.hpp"
#include <array>
namespace yukari {
struct MusicNote { uint32_t time=0; bool flag=false; };
struct MusicChart {
 int32_t background=0,speed=5,start_delay=0,success_border=0;
 uint32_t total_time=0;
 std::string sound_file,sound_name_graphic;
 std::array<std::vector<MusicNote>,5> lanes;
};
// Input remains in the game's encoding; resource names can be decoded by Loader.
MusicChart parse_music_chart(const Bytes& bytes);
struct MusicGameOptions {
 int32_t success_border=-1,speed=6,background=0;
 std::array<bool,5> character_variants{};
};
struct MusicGameEvent { unsigned lane; size_t note; bool hit; unsigned character_group; };
struct MusicNotePosition { int32_t x=0,y=0; bool visible=false; };
class MusicGame {
public:
 explicit MusicGame(MusicChart chart,const MusicGameOptions& options={});
 // One native Sleep(10) iteration; process at most one note per lane per tick.
 std::vector<MusicGameEvent> tick(uint32_t elapsed_ms,const std::array<bool,5>& keys,bool abort=false);
 const MusicChart& chart()const{return chart_;}
 uint32_t hits()const{return hits_;} uint32_t misses()const{return misses_;}
 bool finished()const{return finished_;} bool successful()const;
 MusicNotePosition position(unsigned lane,size_t note,uint32_t elapsed_ms)const;
 static constexpr std::array<char,5> keys={'Z','S','X','D','C'};
private:
 MusicChart chart_;MusicGameOptions options_;
 std::array<size_t,5> next_{};std::array<bool,5> previous_{};
 std::array<std::vector<bool>,5> hidden_;
 uint32_t hits_=0,misses_=0;bool finished_=false;
};
}

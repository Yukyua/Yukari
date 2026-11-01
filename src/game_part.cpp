#include "yukari/game_part.hpp"
#include <cstdio>
#include <limits>
#include <stdexcept>
namespace yukari {
MusicChart parse_music_chart(const Bytes& bytes){
 if(bytes.size()>16*1024*1024)throw std::runtime_error("music chart exceeds 16 MiB");
 MusicChart out;uint32_t maximum=0;std::string text(bytes.begin(),bytes.end());
 size_t at=0;while(at<text.size()&&text[at]){
  size_t end=text.find_first_of("\r\n",at);if(end==std::string::npos)end=text.size();size_t zero=text.find(char(0),at);if(zero!=std::string::npos&&zero<end)end=zero;
  std::string line=text.substr(at,end-at);if(line.size()>1023)throw std::runtime_error("music chart line exceeds native 1023-byte buffer");
  auto integer=[&](const char* prefix,int32_t& value){std::string p(prefix);if(line.compare(0,p.size(),p))return false;int n;if(std::sscanf(line.c_str()+p.size(),"%d",&n)==1)value=n;return true;};
  int32_t total=int32_t(out.total_time);
  if(integer("BackGraphicType,",out.background)||integer("SuccessBorder,",out.success_border)||integer("StartDelay,",out.start_delay)){}
  else if(integer("MusicTotalTime,",total))out.total_time=uint32_t(total);
  else if(line.compare(0,10,"SoundFile,")==0)out.sound_file=line.substr(10);
  else if(line.compare(0,17,"SoundNameGraphic,")==0)out.sound_name_graphic=line.substr(17);
  else {int lane=0,flag=0;unsigned time=0;if(std::sscanf(line.c_str(),"%d,%u,%d",&lane,&time,&flag)>=2&&lane>=1&&lane<=5&&time){auto& v=out.lanes[size_t(lane-1)];if(v.size()>=1000)throw std::runtime_error("music chart lane exceeds native 1000-note table");v.push_back({uint32_t(out.start_delay)+time,flag==1});if(time>maximum)maximum=time;}}
  at=end;while(at<text.size()&&(text[at]=='\r'||text[at]=='\n'))++at;
 }
 if(!out.total_time)out.total_time=uint32_t(out.start_delay)+5000+maximum;
 return out;
}
MusicGame::MusicGame(MusicChart chart,const MusicGameOptions& options):chart_(std::move(chart)),options_(options){
 chart_.speed=options.speed;if(!chart_.speed)throw std::runtime_error("music game speed divides by zero");
 if(options.success_border>=0)chart_.success_border=options.success_border;
 if(options.background>0)chart_.background=options.background;
 for(size_t lane=0;lane<5;++lane)hidden_[lane].resize(chart_.lanes[lane].size());
}
bool MusicGame::successful()const{return int32_t(hits_)>=chart_.success_border;}
std::vector<MusicGameEvent> MusicGame::tick(uint32_t elapsed,const std::array<bool,5>& keys,bool abort){
 std::vector<MusicGameEvent> events;if(finished_)return events;
 for(unsigned lane=0;lane<5;++lane){
  bool rising=keys[lane]&&!previous_[lane];previous_[lane]=keys[lane];
  size_t index=next_[lane];if(index==chart_.lanes[lane].size())continue;
  uint32_t time=chart_.lanes[lane][index].time;
  bool window=time-150<elapsed&&elapsed<time+150;
  bool missed=!window&&elapsed>time+150;
  if(missed||(window&&rising)){
   if(missed)++misses_;else{++hits_;hidden_[lane][index]=true;}
   events.push_back({lane,index,!missed,unsigned(options_.character_variants[lane]?3:0)+unsigned(missed?2:1)});++next_[lane];
  }
 }
 finished_=elapsed>chart_.total_time||abort;return events;
}
MusicNotePosition MusicGame::position(unsigned lane,size_t note,uint32_t elapsed)const{
 if(lane>=5||note>=chart_.lanes[lane].size())throw std::runtime_error("music note index outside chart");
 static constexpr int offset[5]={-34,-3,0,-3,-34};
 // Division in initial placement is signed; the scrolling parent is unsigned.
 int32_t numerator=int32_t(chart_.lanes[lane][note].time+uint32_t(offset[lane]));
 if(numerator==std::numeric_limits<int32_t>::min()&&chart_.speed==-1)throw std::runtime_error("music note position overflow");
 int64_t y=-int64_t(numerator/chart_.speed)+uint32_t(elapsed/uint32_t(chart_.speed)+536);
 MusicNotePosition p;p.y=int32_t(uint32_t(y));p.x=610;
 p.visible=p.y>=1&&p.y<=719&&!hidden_[lane][note];
 if(p.visible)switch(lane){case 0:p.x=367+p.y*152/676;break;case 1:p.x=487+p.y*81/650;break;case 2:p.x=607;break;case 3:p.x=731-p.y*81/650;break;case 4:p.x=848-p.y*152/676;break;}
 return p;
}
}

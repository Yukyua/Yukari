#include "yukari/mac_cd_player.hpp"
#include "yukari/cd_media.hpp"
#include <sys/mount.h>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
namespace yukari {
namespace {
constexpr int channel=0;
uint32_t clock_ms(){return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
std::string lower(std::string s){for(auto& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
std::string device_key(std::string s){const auto p=s.find_last_of('/');if(p!=std::string::npos)s.erase(0,p+1);if(s.compare(0,5,"rdisk")==0)s.erase(0,1);return s;}

}
std::vector<CdMount> MacCdPlayer::mounted_audio_discs(){
 struct statfs* table=nullptr;const auto count=getmntinfo(&table,MNT_NOWAIT);if(count<=0)return {};
 std::vector<CdMount> mounts;
 for(int i=0;i<count;++i)if(std::string(table[i].f_fstypename)=="cddafs"){
  std::filesystem::path path(table[i].f_mntonname);mounts.push_back({path.string(),path.filename().string(),table[i].f_mntfromname});
 }return mounts;
}
std::vector<CdTrackFile> MacCdPlayer::track_files(const CdMount& mount){
 std::vector<CdTrackFile> result;
 for(const auto& entry:std::filesystem::directory_iterator(mount.path)){
  if(!entry.is_regular_file())continue;const auto ext=lower(entry.path().extension().string());if(ext!=".aiff"&&ext!=".aif"&&ext!=".cdda")continue;
  const auto name=entry.path().filename().string();size_t i=0;while(i<name.size()&&name[i]==' ')++i;int track=0;const auto first=i;
  for(;i<name.size()&&name[i]>='0'&&name[i]<='9';++i){track=track*10+name[i]-'0';if(track>99)break;}
  if(i==first||track<1||track>99)continue;result.push_back({track,entry.path().string()});
 }
 std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.number<b.number;});
 for(size_t i=1;i<result.size();++i)if(result[i-1].number==result[i].number)throw std::runtime_error("ambiguous CD track number");
 // A readable matching TOC is authoritative about data tracks. Discovery
 // failure does not make an otherwise readable cddafs mount unusable.
 try{for(const auto& media:enumerate_cd_media())if(device_key(media.device)==device_key(mount.device)){
  result.erase(std::remove_if(result.begin(),result.end(),[&](const auto& file){auto t=std::find_if(media.tracks.begin(),media.tracks.end(),[&](const auto& track){return track.number==uint32_t(file.number);});return t==media.tracks.end()||!t->audio;}),result.end());break;
 }}catch(const std::exception&){}
 return result;
}
struct MacCdPlayer::Impl {
 MacCdHooks hooks;std::unique_ptr<Audio> audio;std::optional<CdMount> selected;
 std::function<void(uint32_t,uint32_t)> clock_sink;std::function<bool()> silent_query;std::string error;bool muted=false;
 explicit Impl(MacCdHooks h):hooks(std::move(h)){
  if(!hooks.mounts)hooks.mounts=MacCdPlayer::mounted_audio_discs;
  if(!hooks.tracks)hooks.tracks=MacCdPlayer::track_files;
  if(!hooks.make_audio)hooks.make_audio=[](){return std::make_unique<Audio>();};
  if(!hooks.now_ms)hooks.now_ms=clock_ms;
 }
 bool find(const std::string& prefix){
  try{const auto mounts=hooks.mounts();const auto wanted=lower(prefix.substr(0,prefix.find('\0')));
   for(const auto& mount:mounts)if(lower(mount.name).compare(0,wanted.size(),wanted)==0){selected=mount;error.clear();return true;}
   error="no matching mounted audio CD";
  }catch(const std::exception& e){error=e.what();}return false;
 }
 CdAcknowledgement play(int32_t track,bool loop){
  try{
   if(silent_query&&silent_query())throw std::runtime_error("CD playback is silenced");
   const auto mounts=hooks.mounts();std::optional<CdMount> mount;
   if(selected){for(const auto& candidate:mounts)if(candidate.path==selected->path&&candidate.device==selected->device){mount=candidate;break;}}
   else if(!mounts.empty())mount=mounts.front();
   if(!mount)throw std::runtime_error("no mounted audio CD");
   const auto tracks=hooks.tracks(*mount);const auto file=std::find_if(tracks.begin(),tracks.end(),[&](const auto& f){return f.number==track;});
   if(file==tracks.end())throw std::runtime_error("audio CD track not found");
   auto source=hooks.read_file?ResourceSource::memory(std::make_shared<const Bytes>(hooks.read_file(file->path))):ResourceSource::file(file->path);if(!source.size())throw std::runtime_error("empty CD source");
   // Validate decoding before creating a second SDL device. A corrupt track
   // must not stop the main game's audio or leave a claimed playing clock.
   Audio validation(false);validation.load_sources(channel,{source},true);validation.play(channel);if(!validation.duration_ms(channel))throw std::runtime_error("empty decoded CD track");
   if(!audio){auto next=hooks.make_audio();if(!next)throw std::runtime_error("CD audio factory returned null");audio=std::move(next);}
   audio->set_muted(muted);audio->load_sources(channel,{source},true);audio->play(channel,loop);
   if(!audio->playing(channel))throw std::runtime_error("CD audio did not start");
   if(clock_sink){const auto time=hooks.now_ms();clock_sink(time,time);}
   error.clear();return CdAcknowledgement::Signalled;
  }catch(const std::exception& e){error=e.what();if(audio)audio->stop(channel);return CdAcknowledgement::Failed;}
 }
 void stop(){if(audio)audio->stop(channel);}
};
MacCdPlayer::MacCdPlayer(MacCdHooks hooks):impl_(std::make_unique<Impl>(std::move(hooks))){}
MacCdPlayer::~MacCdPlayer()=default;
CdProvider MacCdPlayer::provider(){CdProvider result;auto* p=impl_.get();result.play=[p](int32_t track,bool loop){return p->play(track,loop);};result.stop=[p](bool){p->stop();};result.silence=[p]{p->stop();};result.set_loop=[p](bool loop){if(p->audio&&p->audio->loaded(channel))p->audio->set_loop(channel,loop);};result.find_volume=[p](const std::string& name){return p->find(name);};result.now_ms=[p]{return p->hooks.now_ms();};return result;}
void MacCdPlayer::set_clock_sink(std::function<void(uint32_t,uint32_t)> sink){impl_->clock_sink=std::move(sink);}
void MacCdPlayer::set_muted(bool muted){impl_->muted=muted;if(impl_->audio)impl_->audio->set_muted(muted);}
void MacCdPlayer::set_silent_query(std::function<bool()> query){impl_->silent_query=std::move(query);}
bool MacCdPlayer::audio_created()const{return bool(impl_->audio);}
const std::string& MacCdPlayer::last_error()const{return impl_->error;}
}

#include "yukari/mac_cd_player.hpp"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
using namespace yukari;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static HostCall call(std::string name,std::vector<Value> args={}){HostCall result;result.function=std::move(name);result.arguments=std::move(args);return result;}
static Bytes aiff(){
 Bytes out;auto tag=[&](const char* s){out.insert(out.end(),s,s+4);};auto u16=[&](uint16_t n){out.push_back(n>>8);out.push_back(n&255);};auto u32=[&](uint32_t n){out.push_back(n>>24);out.push_back((n>>16)&255);out.push_back((n>>8)&255);out.push_back(n&255);};
 constexpr int frames=4410;tag("FORM");u32(46+frames*2);tag("AIFF");tag("COMM");u32(18);u16(1);u32(frames);u16(16);out.insert(out.end(),{0x40,0x0e,0xac,0x44,0,0,0,0,0,0});tag("SSND");u32(8+frames*2);u32(0);u32(0);for(int i=0;i<frames;++i)u16(static_cast<uint16_t>(static_cast<int16_t>(std::sin(i*.1)*12000)));return out;
}
int main(){try{
 setenv("SDL_AUDIODRIVER","dummy",1);setenv("SDL_VIDEODRIVER","dummy",1);
 std::vector<CdMount> mounts;int factories=0,reads=0;uint32_t now=123;bool corrupt=false,read_error=false;Audio* audio=nullptr;const auto bytes=aiff();
 MacCdHooks hooks;hooks.mounts=[&]{return mounts;};hooks.tracks=[](const CdMount&){return std::vector<CdTrackFile>{{1,"track.aiff"},{2,"second.aif"}};};hooks.read_file=[&](const std::string&){++reads;if(read_error)throw std::runtime_error("removed disc");return corrupt?Bytes{1,2,3}:bytes;};hooks.make_audio=[&]{++factories;auto result=std::make_unique<Audio>(false);audio=result.get();return result;};hooks.now_ms=[&]{return now;};
 MacCdPlayer player(hooks);CdCommands cd(player.provider());player.set_clock_sink([&](uint32_t a,uint32_t b){cd.set_clock_epochs(a,b);});player.set_silent_query([&]{return cd.state().silent;});
 cd.handle(call("CD",{int32_t(1)}));check(!player.audio_created()&&!factories&&!reads&&!cd.state().playback_epoch,"no disc must not open device or seed clock");
 mounts={{"/fake/disc","GAME Audio","disk9"}};corrupt=true;cd.handle(call("CD",{int32_t(2)}));check(!factories&&cd.state().playback_epoch==0,"bad audio must fail before device creation");
 corrupt=false;cd.handle(call("CD",{int32_t(1),int32_t(1)}));check(factories==1&&audio&&audio->playing(0)&&audio->streamed(0)&&audio->loop(0),"AIFF must decode and play streamed");check(cd.state().playback_epoch==123,"success seeds clock");
 float pcm[960]{};audio->render(pcm,480);bool signal=false;for(float sample:pcm)signal|=std::abs(sample)>.01f;check(signal,"decoded AIFF must produce PCM");
 now=150;check(cd.elapsed()==27,"wall clock follows seeded epoch");cd.handle(call("CDSilentSet",{int32_t(1)}));check(!audio->playing(0),"silent stops playback");check(player.provider().play(2,false)==CdAcknowledgement::Failed&&!audio->playing(0),"new request cannot defeat silence");cd.handle(call("CDSilentSet",{int32_t(0)}));check(audio->playing(0)&&factories==1&&cd.elapsed()==0,"loop unmute restarts on same lazy device");
 cd.handle(call("CDPlayMode",{int32_t(0)}));check(!audio->loop(0),"live mode setter");cd.handle(call("CD",{int32_t(0)}));check(!audio->playing(0),"CD zero stops");
 read_error=true;now=180;cd.handle(call("CD",{int32_t(2)}));check(!audio->playing(0)&&cd.state().playback_epoch==150&&!player.last_error().empty(),"read failure neither throws nor seeds clock");read_error=false;
 auto provider=player.provider();check(provider.find_volume("game"),"volume prefix case matching");mounts.clear();check(provider.play(1,false)==CdAcknowledgement::Failed,"selected removed volume fails");
 char dir[]="/tmp/yukari-cd-tracks-XXXXXX";const auto path=mkdtemp(dir);check(path,"temporary directory");struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove_all(path,error);}}cleanup{path};
 for(const auto* file:{"01 Audio Track.aiff","2 Audio Track.AIF","3 Audio Track.cdda","notes.txt","notrack.aiff","999 Track.aif"})std::ofstream(std::filesystem::path(path)/file).put('x');
 const auto files=MacCdPlayer::track_files({path,"test","fake-device"});check(files.size()==3&&files[0].number==1&&files[2].number==3,"mounted track enumeration extension and numbering");
 const auto actual_file=(std::filesystem::path(path)/"01 Audio Track.aiff").string();{std::ofstream file(actual_file,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));}
 MacCdHooks disk_hooks;disk_hooks.mounts=[&]{return std::vector<CdMount>{{path,"TEST","fake-device"}};};disk_hooks.make_audio=[]{return std::make_unique<Audio>(false);};MacCdPlayer disk(disk_hooks);check(disk.provider().play(1,false)==CdAcknowledgement::Signalled&&disk.audio_created(),"default read-only file loader and track scanner must play AIFF");
 const auto real=MacCdPlayer::mounted_audio_discs();if(real.empty()){MacCdPlayer absent;auto p=absent.provider();check(p.play(1,false)==CdAcknowledgement::Failed&&!absent.audio_created(),"real no-disc route must stay lazy");std::cout<<"PASS actual macOS mount scan: no cddafs disc; no device opened\n";}else std::cout<<"INFO actual cddafs mounts="<<real.size()<<"; hardware playback not attempted\n";
 std::cout<<"PASS fake-mounted CDDA AIFF PCM, clock seeding, loop/silent/stop, absent/corrupt/removed source and lazy audio\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

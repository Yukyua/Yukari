#include "yukari/midi.hpp"
#include <AudioToolbox/AudioToolbox.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <stdexcept>
#include <string>
namespace yukari {
namespace {
void check(OSStatus status,const char* operation){if(status!=noErr)throw std::runtime_error(std::string(operation)+" failed (OSStatus "+std::to_string(status)+")");}
uint64_t milliseconds(double seconds){if(!std::isfinite(seconds)||seconds<0||seconds>double(std::numeric_limits<int64_t>::max()/1000))throw std::runtime_error("invalid MIDI timeline");return uint64_t(std::llround(seconds*1000));}
struct Sequence {
 MusicSequence sequence=nullptr;
 Sequence(){check(NewMusicSequence(&sequence),"NewMusicSequence");}
 ~Sequence(){if(sequence)DisposeMusicSequence(sequence);}
};
}
struct Midi::Impl {
 mutable std::mutex mutex;
 std::condition_variable changed;std::thread monitor;bool exiting=false,loop=true;std::string error;
 std::unique_ptr<Sequence> data;
 MusicPlayer player=nullptr;
 AudioUnit synth=nullptr;
 uint32_t track_count=0;
 double beats=0,seconds=0;
 ~Impl(){{std::lock_guard<std::mutex> guard(mutex);exiting=true;}changed.notify_all();if(monitor.joinable())monitor.join();shutdown();}
 void update(){if(!player)return;MusicTimeStamp now=0;check(MusicPlayerGetTime(player,&now),"MusicPlayerGetTime");if(now>=beats){if(loop){silence();check(MusicPlayerSetTime(player,0),"MusicPlayerSetTime(loop)");}else shutdown();}}
 void start_monitor(){if(monitor.joinable())return;monitor=std::thread([this]{std::unique_lock<std::mutex> lock(mutex);while(!exiting){if(!player){changed.wait(lock,[this]{return exiting||player;});continue;}changed.wait_for(lock,std::chrono::milliseconds(10));if(exiting)break;try{update();}catch(const std::exception& e){error=e.what();shutdown();}}});}
 void silence()noexcept{if(synth)for(unsigned channel=0;channel<16;++channel){MusicDeviceMIDIEvent(synth,0xb0|channel,120,0,0);MusicDeviceMIDIEvent(synth,0xb0|channel,123,0,0);}}
 void shutdown()noexcept{if(player){MusicPlayerStop(player);silence();DisposeMusicPlayer(player);player=nullptr;}synth=nullptr;}
 void find_synth(){AUGraph graph=nullptr;check(MusicSequenceGetAUGraph(data->sequence,&graph),"MusicSequenceGetAUGraph");if(!graph)throw std::runtime_error("MIDI sequence has no synthesizer graph");UInt32 count=0;check(AUGraphGetNodeCount(graph,&count),"AUGraphGetNodeCount");for(UInt32 i=0;i<count;++i){AUNode node;AudioComponentDescription description{};AudioUnit unit=nullptr;check(AUGraphGetIndNode(graph,i,&node),"AUGraphGetIndNode");check(AUGraphNodeInfo(graph,node,&description,&unit),"AUGraphNodeInfo");if(description.componentType==kAudioUnitType_MusicDevice){synth=unit;return;}}throw std::runtime_error("MIDI sequence graph has no music device");}
};
Midi::Midi():impl_(std::make_unique<Impl>()){}
Midi::~Midi()=default;
void Midi::load(const Bytes& bytes){
 if(bytes.size()<14||bytes.size()>16*1024*1024)throw std::runtime_error("invalid MIDI resource size");
 auto candidate=std::make_unique<Sequence>();
 CFDataRef buffer=CFDataCreate(kCFAllocatorDefault,bytes.data(),CFIndex(bytes.size()));if(!buffer)throw std::runtime_error("cannot allocate MIDI file data");
 const auto status=MusicSequenceFileLoadData(candidate->sequence,buffer,kMusicSequenceFile_MIDIType,0);CFRelease(buffer);check(status,"MusicSequenceFileLoadData");
 UInt32 tracks=0;check(MusicSequenceGetTrackCount(candidate->sequence,&tracks),"MusicSequenceGetTrackCount");if(tracks>4096)throw std::runtime_error("MIDI track limit exceeded");
 double length=0;for(UInt32 i=0;i<tracks;++i){MusicTrack track=nullptr;check(MusicSequenceGetIndTrack(candidate->sequence,i,&track),"MusicSequenceGetIndTrack");MusicTimeStamp end=0;UInt32 size=sizeof(end);check(MusicTrackGetProperty(track,kSequenceTrackProperty_TrackLength,&end,&size),"MusicTrackGetProperty(track length)");if(!std::isfinite(end)||end<0)throw std::runtime_error("invalid MIDI track length");length=std::max(length,end);}
 Float64 seconds=0;check(MusicSequenceGetSecondsForBeats(candidate->sequence,length,&seconds),"MusicSequenceGetSecondsForBeats");(void)milliseconds(seconds);
 std::lock_guard<std::mutex> guard(impl_->mutex);impl_->shutdown();impl_->error.clear();impl_->data=std::move(candidate);impl_->track_count=tracks;impl_->beats=length;impl_->seconds=seconds;
}
void Midi::play(){
 std::lock_guard<std::mutex> guard(impl_->mutex);if(!impl_->data)throw std::runtime_error("no MIDI sequence loaded");impl_->shutdown();impl_->error.clear();if(impl_->beats==0)return;
 try{check(NewMusicPlayer(&impl_->player),"NewMusicPlayer");check(MusicPlayerSetSequence(impl_->player,impl_->data->sequence),"MusicPlayerSetSequence");check(MusicPlayerSetTime(impl_->player,0),"MusicPlayerSetTime");check(MusicPlayerPreroll(impl_->player),"MusicPlayerPreroll");impl_->find_synth();check(MusicPlayerStart(impl_->player),"MusicPlayerStart");impl_->start_monitor();impl_->changed.notify_all();}catch(...){impl_->shutdown();throw;}
}
void Midi::stop(){std::lock_guard<std::mutex> guard(impl_->mutex);impl_->shutdown();impl_->data.reset();impl_->track_count=0;impl_->beats=impl_->seconds=0;impl_->error.clear();}
void Midi::set_loop(bool loop){std::lock_guard<std::mutex> guard(impl_->mutex);impl_->loop=loop;}
bool Midi::loaded()const{std::lock_guard<std::mutex> guard(impl_->mutex);return bool(impl_->data);}
bool Midi::playing()const{std::lock_guard<std::mutex> guard(impl_->mutex);if(!impl_->error.empty())throw std::runtime_error(impl_->error);impl_->update();if(!impl_->player)return false;Boolean active=false;check(MusicPlayerIsPlaying(impl_->player,&active),"MusicPlayerIsPlaying");return active;}
uint32_t Midi::tracks()const{std::lock_guard<std::mutex> guard(impl_->mutex);return impl_->track_count;}
uint64_t Midi::duration_ms()const{std::lock_guard<std::mutex> guard(impl_->mutex);return milliseconds(impl_->seconds);}
uint64_t Midi::position_ms()const{std::lock_guard<std::mutex> guard(impl_->mutex);if(!impl_->player)return 0;MusicTimeStamp beats=0;check(MusicPlayerGetTime(impl_->player,&beats),"MusicPlayerGetTime");Float64 seconds=0;check(MusicSequenceGetSecondsForBeats(impl_->data->sequence,std::max(0.0,std::min(beats,impl_->beats)),&seconds),"MusicSequenceGetSecondsForBeats");return milliseconds(seconds);}
}

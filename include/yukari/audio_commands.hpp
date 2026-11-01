#pragma once
#include "yukari/audio.hpp"
#include "yukari/vm.hpp"
#include <functional>
#include <algorithm>
#include <map>
#include <stdexcept>
namespace yukari {
struct AudioPolicy {
 bool skip_auto_groups=false; // Caller resolves original skip/input/frame+c4 conditions.
 bool suppress_nonloop=false; // Original DAT_0056c912.
 bool instant_stop=false; // Original FUN_004338f0 skip predicate.
};
struct AudioCommand {
 HostResult result;
 std::optional<uint32_t> wait_ms;
 std::optional<int> wait_sound;
};
// Runtime may delegate its Sound*/CD*/MIDI* services here, then translate the
// wait descriptor to its event-loop wait. Record references retain save paths.
class AudioCommands {
public:
 using Records=std::map<int,std::pair<std::string,bool>>;
 using Loader=std::function<Bytes(const std::string&)>;
 AudioCommands(Audio& audio,Loader loader,Records& records):audio_(audio),loader_(std::move(loader)),records_(records){}
 void set_source_loader(std::function<ResourceSource(const std::string&)> loader){source_loader_=std::move(loader);}
 void set_midi_provider(std::function<void(const Bytes&)> play,std::function<void()> stop){midi_play_=std::move(play);midi_stop_=std::move(stop);}
 std::optional<AudioCommand> handle(const HostCall& call,const AudioPolicy& policy={}) {
  const auto& n=call.function;const auto& args=call.arguments;
  auto num=[&](size_t i)->int{if(i>=args.size()||!std::holds_alternative<int32_t>(args[i]))throw std::runtime_error(n+": integer argument missing");return std::get<int32_t>(args[i]);};
  auto text=[&](size_t i)->std::string{if(i>=args.size()||!std::holds_alternative<std::string>(args[i]))throw std::runtime_error(n+": resource argument missing");return std::get<std::string>(args[i]);};
  auto duration=[&](size_t i){return uint32_t(i<args.size()?std::max(0,num(i)):1000);};
  AudioCommand out;
  auto value=[&](int v){out.result.value=int32_t(v);};
  auto load=[&](int id,const std::string& name,bool stream=false,int32_t loop_start=0){
   std::vector<ResourceSource> parts;size_t begin=0;do{auto end=name.find('\t',begin);auto part=name.substr(begin,end==std::string::npos?end:end-begin);if(part.empty())throw std::runtime_error("empty audio source fragment");parts.push_back(source_loader_?source_loader_(part):ResourceSource::memory(std::make_shared<const Bytes>(loader_(part))));if(end==std::string::npos||end+1==name.size())break;begin=end+1;}while(begin<=name.size());
   audio_.load_sources(id,parts,stream,loop_start);records_[id]={name,false};
  };
  if(n=="SoundLoad"||n=="SoundLoadStream"){load(num(0),text(1),n=="SoundLoadStream",args.size()>2?num(2):0);return out;}
  if(n=="SoundDelete"){if(!args.empty()){audio_.erase(num(0));records_.erase(num(0));}return out;}
  if(n=="SoundPlay"||n=="SoundPlayLoop"||n=="SoundPlayOneOnly"){
   const int id=num(0);if(args.size()==2)load(id,text(1));
   if(n=="SoundPlayLoop")audio_.set_loop(id,true);
   const bool loop=audio_.loop(id);
   if(audio_.loaded(id)&&(!(policy.skip_auto_groups&&audio_.auto_skip(id))||n=="SoundPlayLoop")&&(!policy.suppress_nonloop||loop||n=="SoundPlayLoop"))audio_.play(id);
   if(n=="SoundPlayOneOnly")audio_.set_one_only(id);
   if(records_.count(id))records_[id].second=loop;return out;
  }
  if(n=="SoundLoopSet"){if(args.size()>1){audio_.set_loop(num(0),num(1)!=0);if(records_.count(num(0)))records_[num(0)].second=num(1)!=0;}return out;}
  if(n=="SoundLoopGet"){value(audio_.loop(num(0)));return out;}
  if(n=="SoundPlayCheck"){value(audio_.status(num(0)));return out;}
  if(n=="SoundPlayWait"){if(audio_.status(num(0))==1&&!policy.instant_stop){out.result.waiting=true;out.wait_sound=num(0);}return out;}
  if(n=="SoundStop"||n=="SoundStopToFade"||n=="SoundDeleteToFade"){
   if(args.empty())return out;const auto ms=n=="SoundStop"&&policy.instant_stop?0:duration(1);const int id=num(0);
   const bool loaded=audio_.loaded(id);audio_.fade_stop(id,ms,n=="SoundDeleteToFade");
   if(n=="SoundStop"&&loaded&&ms){out.result.waiting=true;out.wait_ms=ms;}return out;
  }
  if(n=="SoundStopALL"){for(const auto& item:records_)audio_.fade_stop(item.first,500);return out;}
  if(n=="SoundVolume"){if(args.size()==2){if(audio_.loaded(num(0)))audio_.set_volume(num(0),num(1));value(audio_.loaded(num(0))?audio_.volume(num(0)):0);}else value(0);return out;}
  if(n=="SoundVolumeToFade"){if(args.size()>1)audio_.fade_volume(num(0),num(1),duration(2));return out;}
  if(n=="SoundSilentSet"){if(args.size()==2)audio_.set_silent(num(0),num(1)==1);return out;}
  if(n=="SoundSilentGet"){value(args.size()==1&&audio_.silent(num(0)));return out;}
  if(n=="SoundGroupSet"){if(args.size()==3&&num(0)>=0)audio_.set_group(num(0),num(1),num(2));return out;}
  if(n=="SoundGroupVolumeSet"||n=="SoundGroupVolumeGet"){
   if(n=="SoundGroupVolumeSet"&&args.size()==2)audio_.set_group_volume(num(0),num(1));value(args.empty()?0:audio_.group_volume(num(0)));return out;
  }
  if(n=="SoundGroupVolumeSetToFade"||n=="SoundGroupSilentSetToFade"){
   if(args.size()==2||args.size()==3)audio_.fade_group_volume(num(0),n=="SoundGroupSilentSetToFade"?int(num(1)==1):num(1),duration(2));return out;
  }
  if(n=="SoundGroupSilentSet"||n=="SoundGroupSilentGet"){
   if(n=="SoundGroupSilentSet"&&args.size()==2)audio_.set_group_silent(num(0),num(1)==1);value(args.empty()?0:int(audio_.group_silent(num(0))));return out;
  }
  if(n=="SoundGroupAutomodeSkipSet"||n=="SoundGroupAutomodeSkipGet"){
   if(n=="SoundGroupAutomodeSkipSet"&&args.size()==2)audio_.set_group_auto_skip(num(0),num(1));value(args.empty()?0:audio_.group_auto_skip(num(0)));return out;
  }
  if(n=="SoundGroupRestrainTarget"){if(!args.empty())audio_.set_restrain_level(num(0));if(args.size()>1){std::vector<int> groups;for(size_t i=1;i<args.size();++i)groups.push_back(num(i));audio_.set_restrain_groups(groups);}return out;}
  if(n=="HistorySoundGroupSet"){if(!args.empty()){std::vector<int> groups;for(size_t i=0;i<args.size();++i)groups.push_back(num(i)&255);audio_.add_history_groups(groups);}return out;}
  if(n=="CDPlayTime"){return out;} // Native handler itself returns null.
  if(n=="CDNameSet"){cd_name_=text(0);return out;}
  if(n=="CDPlayMode"){cd_loop_=num(0)==1;return out;}
  if(n=="CDSilentSet"){if(!args.empty())cd_silent_=num(0)!=0;return out;}
  if(n=="CDSilentGet"){value(cd_silent_);return out;}
  if(n=="CD"){if(num(0)!=0)throw std::runtime_error("CD audio playback requires an unavailable CD device");return out;}
  if(n=="CDWait"){if(num(0)>0)throw std::runtime_error("CDWait requires an unavailable CD playback clock");return out;}
  if(n=="MIDIPlay"){if(!midi_play_)throw std::runtime_error("MIDI playback provider is not installed");midi_play_(loader_(text(0)));return out;}
  if(n=="MIDIStop"){if(midi_stop_)midi_stop_();return out;}
  return std::nullopt;
 }
private:
 Audio& audio_;Loader loader_;Records& records_;std::function<ResourceSource(const std::string&)> source_loader_;
 std::string cd_name_;bool cd_loop_=false,cd_silent_=false;
 std::function<void(const Bytes&)> midi_play_;std::function<void()> midi_stop_;
};
}

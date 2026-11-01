#include "yukari/cd_commands.hpp"
#include <chrono>
#include <stdexcept>
#include <utility>
namespace yukari {
CdCommands::CdCommands(CdProvider provider):provider_(std::move(provider)){}
uint32_t CdCommands::now()const {
 if(provider_.now_ms)return provider_.now_ms();
 return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}
void CdCommands::set_clock_epochs(uint32_t track,uint32_t playback){state_.track_epoch=track;state_.playback_epoch=playback;}
uint32_t CdCommands::elapsed(int clock)const {
 const auto epoch=clock==1?state_.track_epoch:clock==2?state_.playback_epoch:0;
 return epoch?now()-epoch:0;
}
void CdCommands::play(int32_t track,bool loop){
 if(state_.device_track==track)return; // Same track suppresses even loop update.
 state_.device_track=track;state_.device_loop=loop;
 const auto outcome=provider_.play?provider_.play(track,loop):CdAcknowledgement::Failed;
 if(outcome==CdAcknowledgement::Timeout)state_.track_epoch=state_.playback_epoch=now();
}
void CdCommands::stop(bool immediate){
 state_.device_track=0;
 if(provider_.stop)provider_.stop(immediate);
 // 401f70 does not reset either clock epoch.
}
std::optional<CdCommand> CdCommands::handle(const HostCall& call,const CdPolicy& policy){
 const auto& name=call.function; const auto& args=call.arguments;
 if(name!="CD"&&name!="CDNameSet"&&name!="CDPlayMode"&&name!="CDPlayTime"&&name!="CDSilentSet"&&name!="CDSilentGet"&&name!="CDWait")return std::nullopt;
 auto number=[&](size_t i){if(i>=args.size()||!std::holds_alternative<int32_t>(args[i]))throw std::runtime_error(name+": integer argument missing");return std::get<int32_t>(args[i]);};
 CdCommand out;
 if(name=="CDPlayTime")return out; // Original 4376e0 really returns null.
 if(name=="CDSilentGet"){out.result.value=int32_t(state_.silent);return out;}
 if(name=="CDNameSet"){
  if(!state_.name_attempted){state_.name_attempted=true;if(policy.name_discovery_allowed){
   if(args.empty()||!std::holds_alternative<std::string>(args[0]))throw std::runtime_error(name+": string argument missing");
   if(provider_.find_volume)(void)provider_.find_volume(std::get<std::string>(args[0]));
  }}return out;
 }
 if(name=="CDPlayMode"){state_.script_loop=state_.device_loop=number(0)==1;if(provider_.set_loop)provider_.set_loop(state_.device_loop);return out;}
 if(name=="CDWait"){
  const int32_t target=number(0);
  if(target>0){const auto current=elapsed(2);if(current<uint32_t(target)){out.wait_ms=uint32_t(target)-current;out.click_interruptible=true;out.result.waiting=true;}}
  return out;
 }
 if(name=="CDSilentSet"){
  if(args.empty())return out;
  state_.silent=number(0)!=0;
  if(state_.silent){if(provider_.silence)provider_.silence();}
  else if(state_.device_track>0&&state_.device_loop){const auto track=state_.device_track;state_.device_track=0;play(track,true);}
  else stop(false);
  return out;
 }
 if(args.size()!=1&&args.size()!=2)return out;
 state_.script_track=number(0);
 if(args.size()==2)state_.script_loop=number(1)==1;
 if(state_.script_track){if(policy.immediate_stop)stop(true);play(state_.script_track,state_.script_loop);}
 else stop(policy.immediate_stop);
 return out;
}
}

#include "yukari/text_schedule.hpp"
#include <algorithm>
#include <stdexcept>
namespace yukari {
namespace {size_t length(const std::string& text){auto end=text.find(char(0));return end==std::string::npos?text.size():end;}}
TextSchedule::TextSchedule(std::vector<TextScheduleTarget> targets){TextScheduleState state;state.targets=std::move(targets);state.complete=false;restore(std::move(state));}
void TextSchedule::restore(TextScheduleState state){
 if(state.targets.size()>65536||state.next_target>state.targets.size()||(state.targets.empty()&&state.next_target))throw std::runtime_error("invalid text schedule target count/cursor");
 size_t total=0;for(auto& target:state.targets){if(target.encoded.size()>128*1024*1024-total||target.offset>length(target.encoded))throw std::runtime_error("invalid text schedule string/offset");total+=target.encoded.size();target.encoded.resize(length(target.encoded));if(state.complete&&target.offset!=target.encoded.size())throw std::runtime_error("completed text schedule has pending target");}
 if(state.complete&&state.next_target)throw std::runtime_error("completed text schedule has mid-round cursor");state_=std::move(state);
}
TextScheduleCycle TextSchedule::cycle(const Emit& emit){
 if(state_.complete)return {};
 if(!emit)throw std::runtime_error("text schedule requires emitter");
 TextScheduleCycle result;
 while(state_.next_target<state_.targets.size()){
  auto& target=state_.targets[state_.next_target];const auto end=target.encoded.size();
  if(target.offset<end){
   const auto consumed=emit(target.layer,std::string_view(target.encoded).substr(target.offset,end-target.offset));
   if(!consumed){result.status=TextScheduleCycle::PageBlocked;return result;}
   if(consumed>end-target.offset)throw std::runtime_error("text emitter consumed beyond source terminator");target.offset+=consumed;
   if(std::find(result.changed_layers.begin(),result.changed_layers.end(),target.layer)==result.changed_layers.end())result.changed_layers.push_back(target.layer);
  }
  ++state_.next_target;
 }
 state_.next_target=0;state_.complete=true;
 for(const auto& target:state_.targets)if(target.offset<target.encoded.size()){state_.complete=false;break;}
 result.status=TextScheduleCycle::RoundComplete;result.final_round=state_.complete;return result;
}
uint32_t text_schedule_delay(const TextSchedulePolicy& policy){return !policy.skip_now&&policy.text_speed!=0&&!policy.fast_latched&&!policy.single_layer_no_wait?uint32_t(policy.text_speed):0;}
bool text_schedule_latch(bool deferred,bool skip,int32_t speed,bool single,bool already,bool already_fast){return !deferred&&(skip||speed==0||single||(already&&already_fast));}
}

#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
namespace yukari {
struct TextScheduleTarget {int32_t layer=0;std::string encoded;size_t offset=0;};
struct TextScheduleState {std::vector<TextScheduleTarget> targets;size_t next_target=0;bool complete=false;};
struct TextScheduleCycle {
 enum Status { RoundComplete,PageBlocked,Done } status=Done;
 bool final_round=false;
 std::vector<int32_t> changed_layers;
};
// Native445ed0 calls4455f0 once per target in declaration order, then sleeps
// once per full round. Emitter owns encoding, control codes and page geometry.
class TextSchedule {
public:
 using Emit=std::function<size_t(int32_t,std::string_view)>;
 explicit TextSchedule(std::vector<TextScheduleTarget> targets={});
 TextScheduleCycle cycle(const Emit&);
 const TextScheduleState& state()const{return state_;}
 void restore(TextScheduleState state);
private:
 TextScheduleState state_;
};
struct TextSchedulePolicy {
 int32_t text_speed=0;
 bool skip_now=false,fast_latched=false,single_layer_no_wait=false;
};
// Sleep is performed after RoundComplete, even when final_round==true. No
// delay belongs to PageBlocked or Done. Negative speed preserves DWORD cast.
uint32_t text_schedule_delay(const TextSchedulePolicy&);
// At the beginning of StrOut when the first target's native offset is zero,
// reset db10 then possibly acquire drawing deferral. If already deferred this
// instruction does not own it and must not release it at completion.
bool text_schedule_latch(bool drawing_already_deferred,bool skip_now,int32_t text_speed,
                         bool single_layer_no_wait,bool current_instruction_already,bool already_text_fast);
}

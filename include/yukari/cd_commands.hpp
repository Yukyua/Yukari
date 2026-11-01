#pragma once
#include "yukari/vm.hpp"
#include <functional>

namespace yukari {
// Native ResetEvent/SetEvent/WaitForSingleObject outcome. Device/MCI errors
// are backend observations, not script errors. Timeout alone seeds both clocks.
enum class CdAcknowledgement { Signalled, Timeout, Failed };
struct CdProvider {
 std::function<CdAcknowledgement(int32_t track,bool loop)> play;
 std::function<void(bool immediate)> stop;
 std::function<void()> silence;
 std::function<void(bool)> set_loop;
 // Match an optical-volume display-name prefix, case-insensitively (402050).
 std::function<bool(const std::string&)> find_volume;
 std::function<uint32_t()> now_ms;
};
struct CdState {
 int32_t script_track=0, device_track=0;
 bool script_loop=false, device_loop=false, silent=false, name_attempted=false;
 uint32_t track_epoch=0, playback_epoch=0;
};
struct CdPolicy {
 bool immediate_stop=false; // Original 4338f0 predicate.
 bool name_discovery_allowed=true; // Original DAT_56c358 == 0.
};
struct CdCommand {
 HostResult result;
 std::optional<uint32_t> wait_ms;
 bool click_interruptible=false;
};
class CdCommands {
public:
 explicit CdCommands(CdProvider provider={});
 std::optional<CdCommand> handle(const HostCall&,const CdPolicy& policy={});
 uint32_t elapsed(int clock=2) const;
 // A device worker may report the original globals; elapsed() never substitutes
 // a hardware-position query for timeGetTime subtraction.
 void set_clock_epochs(uint32_t track,uint32_t playback);
 const CdState& state()const{return state_;}
 void restore(const CdState& state){state_=state;}
private:
 void play(int32_t track,bool loop);
 void stop(bool immediate);
 uint32_t now()const;
 CdProvider provider_; CdState state_;
};
}

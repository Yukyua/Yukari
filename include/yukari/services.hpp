#pragma once
#include "yukari/vm.hpp"
#include <array>

namespace yukari {
// Evidence-backed scalar services. The runtime supplies platform queries rather
// than receiving invented successful answers when a provider is unavailable.
struct ServiceCalendar {
    int32_t year{}, month{}, day{}, hour{}, minute{}, second{}, millisecond{}, week{};
};
struct ServiceResult {
    Value value{};
    // TimerWait yields the remaining duration; the caller must pump events and
    // honor original skip/cancel policies while waiting, then resume the VM.
    std::optional<uint32_t> delay_ms;
};
struct ServiceState {
    std::array<uint32_t,16> timers{};
    uint32_t play_seconds{}, play_started{};
};
struct ServiceHooks {
    std::function<uint32_t()> ticks;
    std::function<std::optional<ServiceCalendar>()> calendar;
    // FileExistCheck uses FindFirstFile-style patterns, ignoring directories.
    std::function<bool(const std::string&)> file_exists_pattern;
    std::function<bool(const std::string&)> archive_contains;
    std::function<int32_t(bool horizontal)> window_size;
    std::string text_encoding="CP932";
};
class ScalarServices {
public:
    explicit ScalarServices(ServiceHooks hooks={});
    // Unknown name => nullopt; absent required platform provider => exception.
    // Monostate value is an observed native null result, never a default handler.
    std::optional<ServiceResult> call(const std::string& name,const std::vector<Value>& arguments);
    const ServiceState& state() const { return state_; }
    void restore(const ServiceState& state) { state_=state; }
private:
    ServiceHooks hooks_;
    ServiceState state_;
};
}

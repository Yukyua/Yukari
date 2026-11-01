#include "yukari/services.hpp"
#include "yukari/original_scene.hpp"
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <limits>
#include <stdexcept>

namespace yukari {
namespace {
int32_t integer(const Value& value) {
    auto p=std::get_if<int32_t>(&value);
    if(!p)throw std::runtime_error("scalar service requires integer argument");
    return *p;
}
const std::string& string(const Value& value) {
    auto p=std::get_if<std::string>(&value);
    if(!p)throw std::runtime_error("scalar service requires string argument");
    return *p;
}
int32_t signed_bits(uint32_t n) { int32_t r;std::memcpy(&r,&n,4);return r; }
uint32_t ticks() {
    return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}
std::optional<ServiceCalendar> calendar() {
    auto now=std::chrono::system_clock::now();
    auto seconds=std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto stamp=std::chrono::system_clock::to_time_t(seconds);
    std::tm tm{};
    if(!localtime_r(&stamp,&tm))return std::nullopt;
    return ServiceCalendar{tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec,
        int32_t(std::chrono::duration_cast<std::chrono::milliseconds>(now-seconds).count()),tm.tm_wday};
}
int32_t trunc_x86(long double value) {
    // 0x464ca0 SSE path uses cvttsd2si; indefinite integer on overflow.
    value=std::trunc(static_cast<double>(value));
    if(!std::isfinite(value)||value<std::numeric_limits<int32_t>::min()||value>std::numeric_limits<int32_t>::max())
        return std::numeric_limits<int32_t>::min();
    return int32_t(value);
}
}
ScalarServices::ScalarServices(ServiceHooks hooks):hooks_(std::move(hooks)) {
    if(!hooks_.ticks)hooks_.ticks=ticks;
    if(!hooks_.calendar)hooks_.calendar=calendar;
    state_.play_started=hooks_.ticks();
}
std::optional<ServiceResult> ScalarServices::call(const std::string& name,const std::vector<Value>& args) {
    ServiceResult result;
    if(name=="StringLength") {
        int32_t length=0;
        if(!args.empty()&&!std::holds_alternative<std::monostate>(args[0])) {
            const auto& utf8=string(args[0]);
            auto bytes=encode_original_text(utf8.substr(0,utf8.find('\0')),hooks_.text_encoding.c_str());
            if(bytes.size()>size_t(std::numeric_limits<int32_t>::max()))throw std::runtime_error("StringLength overflow");
            length=int32_t(bytes.size());
        }
        result.value=length;return result;
    }
    if(name=="NumberToString") {
        if(!args.empty())result.value=std::to_string(integer(args[0]));
        return result;
    }
    if(name=="sin"||name=="cos"||name=="tan") {
        if(args.size()!=1)return result;
        // Constants read from original VA 0x4a3950/6598/34f8/6590:
        // pi approximation 3.141592, 180, scale 1000 and bias +0.4.
        long double angle=static_cast<long double>(integer(args[0]))*double(3.141592)/double(180);
        long double value=name=="sin"?std::sin(angle):name=="cos"?std::cos(angle):std::tan(angle);
        result.value=trunc_x86(value*double(1000)+double(0.4));return result;
    }
    if(name=="TimerStart"||name=="TimerGet"||name=="TimerWait") {
        if(name=="TimerGet")result.value=int32_t(0);
        const size_t count=name=="TimerWait"?2:1;
        if(args.size()!=count)return result;
        int32_t id=integer(args[0]);
        if(id<0||id>=16)return result;
        if(name=="TimerStart"){state_.timers[id]=hooks_.ticks();return result;}
        uint32_t start=state_.timers[id];if(!start)return result;
        uint32_t elapsed=hooks_.ticks()-start;
        if(name=="TimerGet")result.value=signed_bits(elapsed);
        else {uint32_t duration=uint32_t(integer(args[1]));if(elapsed<duration)result.delay_ms=duration-elapsed;}
        return result;
    }
    if(name=="PlayTimeReset") {
        state_.play_seconds=0;state_.play_started=hooks_.ticks();return result;
    }
    if(name=="PlayTime") {
        result.value=signed_bits(state_.play_seconds+(hooks_.ticks()-state_.play_started)/1000);return result;
    }
    if(name=="Year"||name=="Month"||name=="Day"||name=="Hour"||name=="Minute"||name=="Second"||name=="Millisecond"||name=="Week") {
        auto t=hooks_.calendar();int32_t value=0;
        if(t){if(name=="Year")value=t->year;else if(name=="Month")value=t->month;else if(name=="Day")value=t->day;
            else if(name=="Hour")value=t->hour;else if(name=="Minute")value=t->minute;else if(name=="Second")value=t->second;
            else if(name=="Millisecond")value=t->millisecond;else value=t->week;}
        result.value=value;return result;
    }
    if(name=="FileExistCheck"||name=="FileExistCheckFromYKC") {
        result.value=int32_t(0);if(args.empty())return result;
        auto& provider=name=="FileExistCheck"?hooks_.file_exists_pattern:hooks_.archive_contains;
        if(!provider)throw std::runtime_error(name+" requires a configured query provider");
        result.value=int32_t(provider(string(args[0])));return result;
    }
    if(name=="WindowSizeXGet"||name=="WindowSizeYGet") {
        if(!hooks_.window_size)throw std::runtime_error(name+" requires a configured logical-window size provider");
        result.value=hooks_.window_size(name=="WindowSizeXGet");return result;
    }
    // Original 0x43c600 unconditionally constructs integer zero.
    if(name=="DebugModeCheck"){result.value=int32_t(0);return result;}
    if(name=="ExclusiveCheck") {
        // 0x43c630 compares an all-zero GUID to a default nonzero GUID, or to
        // exactly eleven integer components. 0x432600 is GUID equality.
        bool zero=false;
        if(args.size()==11){
            zero=uint32_t(integer(args[0]))==0;
            for(size_t i=1;i<args.size();++i){
                uint32_t mask=i<=2?0xffff:0xff;
                zero=(uint32_t(integer(args[i]))&mask)==0 && zero;
            }
        }
        result.value=int32_t(zero);return result;
    }
    return std::nullopt;
}
}

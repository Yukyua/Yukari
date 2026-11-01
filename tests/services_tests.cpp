#include "yukari/services.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace yukari;
namespace {
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
ServiceResult call(ScalarServices& s,const std::string& n,std::vector<Value> a={}){
    auto r=s.call(n,a);check(bool(r),"missing service "+n);return *r;
}
int32_t num(ScalarServices& s,const std::string& n,std::vector<Value> a={}){
    auto r=call(s,n,a);auto p=std::get_if<int32_t>(&r.value);check(p!=nullptr,"expected integer: "+n);return *p;
}
template<class F>void fails(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("expected error");}
void strings(){
    ScalarServices s;
    check(num(s,"StringLength")==0,"length missing arg");
    check(num(s,"StringLength",{Value{}})==0,"length null pointer");
    check(num(s,"StringLength",{std::string("abc")})==3,"ASCII bytes");
    check(num(s,"StringLength",{std::string("\xe3\x81\x82")})==2,"CP932 hiragana bytes rather than UTF8 bytes");
    check(num(s,"StringLength",{std::string("\xef\xbd\xb1")})==1,"CP932 halfwidth bytes");
    check(num(s,"StringLength",{std::string("a\0b",3)})==1,"NUL terminates native string");
    fails([&]{call(s,"StringLength",{std::string("\xf0\x9f\x98\x80")});});
    check(std::get<std::string>(call(s,"NumberToString",{int32_t(-2147483647-1)}).value)=="-2147483648","signed decimal");
    check(std::holds_alternative<std::monostate>(call(s,"NumberToString").value),"missing number native null");
    fails([&]{call(s,"NumberToString",{std::string("12")});});
    check(!s.call("UnprovenFunction",{}),"unknown must not become a successful empty call");
}
void trig(){
    ScalarServices s;
    check(num(s,"sin",{int32_t(0)})==0,"sin zero");
    check(num(s,"cos",{int32_t(0)})==1000,"cos scale");
    check(num(s,"tan",{int32_t(0)})==0,"tan zero");
    check(num(s,"sin",{int32_t(90)})==1000,"positive quarter turn");
    check(num(s,"sin",{int32_t(-90)})==-999,"native positive bias with negative truncation");
    check(num(s,"cos",{int32_t(180)})==-999,"native positive bias at negative cosine");
    check(num(s,"tan",{int32_t(90)})==std::numeric_limits<int32_t>::min(),"x86 integer indefinite overflow");
    check(std::holds_alternative<std::monostate>(call(s,"sin").value),"invalid trig argc native null");
}
void timers(){
    uint32_t tick=1000;ServiceHooks hooks;hooks.ticks=[&]{return tick;};ScalarServices s(hooks);
    check(num(s,"TimerGet",{int32_t(0)})==0,"unstarted timer");
    call(s,"TimerStart",{int32_t(0)});tick=1033;
    check(num(s,"TimerGet",{int32_t(0)})==33,"elapsed timer");
    check(call(s,"TimerWait",{int32_t(0),int32_t(80)}).delay_ms==47,"remaining timer wait");
    tick=1080;check(!call(s,"TimerWait",{int32_t(0),int32_t(80)}).delay_ms,"expired timer wait");
    call(s,"TimerStart",{int32_t(-1)});call(s,"TimerStart",{int32_t(16)});
    check(num(s,"TimerGet",{int32_t(16)})==0,"invalid timer index");
    tick=0;call(s,"TimerStart",{int32_t(1)});tick=100;
    check(num(s,"TimerGet",{int32_t(1)})==0,"native zero start sentinel");
    tick=0xfffffff0;call(s,"TimerStart",{int32_t(2)});auto state=s.state();tick=0x10;
    check(num(s,"TimerGet",{int32_t(2)})==32,"unsigned tick wrap");
    ScalarServices restored(hooks);restored.restore(state);
    check(num(restored,"TimerGet",{int32_t(2)})==32,"restore preserves timers");
    check(call(s,"TimerWait",{int32_t(2),int32_t(-1)}).delay_ms==uint32_t(0xffffffff-32),"unsigned native duration");
    tick=3500;check(num(s,"PlayTime")==2,"play clock seconds");
    call(s,"PlayTimeReset");tick=4499;check(num(s,"PlayTime")==0,"play reset subsecond");
    tick=4500;check(num(s,"PlayTime")==1,"play reset second");
    state=s.state();state.play_seconds=42;state.play_started=0xfffffff0;s.restore(state);tick=0x10;
    check(num(s,"PlayTime")==42,"play base and wrap");
}
void queries(){
    bool calendar_ok=true;ServiceHooks hooks;
    hooks.calendar=[&]()->std::optional<ServiceCalendar>{if(!calendar_ok)return std::nullopt;return ServiceCalendar{2026,10,31,8,9,10,321,6};};
    hooks.file_exists_pattern=[](const std::string& n){return n=="data*.ykc";};
    hooks.archive_contains=[](const std::string& n){return n=="start.yks";};
    hooks.window_size=[](bool x){return x?1024:576;};ScalarServices s(hooks);
    const char* fields[]={"Year","Month","Day","Hour","Minute","Second","Millisecond","Week"};
    int expected[]={2026,10,31,8,9,10,321,6};
    for(size_t i=0;i<8;++i)check(num(s,fields[i])==expected[i],"calendar field");
    calendar_ok=false;check(num(s,"Year")==0,"calendar failure native zero");
    check(num(s,"FileExistCheck",{std::string("data*.ykc")})==1,"file pattern query");
    check(num(s,"FileExistCheck",{std::string("missing")})==0,"missing file query");
    check(num(s,"FileExistCheck")==0,"missing filename native zero");
    check(num(s,"FileExistCheckFromYKC",{std::string("start.yks")})==1,"archive query");
    check(num(s,"WindowSizeXGet")==1024&&num(s,"WindowSizeYGet")==576,"logical window size");
    check(num(s,"DebugModeCheck")==0,"original debug constant");
    ScalarServices absent;
    fails([&]{call(absent,"FileExistCheck",{std::string("path")});});
    fails([&]{call(absent,"FileExistCheckFromYKC",{std::string("path")});});
    fails([&]{call(absent,"WindowSizeXGet");});
}
void exclusive(){
    ScalarServices s;check(num(s,"ExclusiveCheck")==0,"default nonzero GUID");
    std::vector<Value> parts(11,int32_t(0));
    check(num(s,"ExclusiveCheck",parts)==1,"zero GUID equality");
    parts[1]=int32_t(65536);parts[3]=int32_t(256);
    check(num(s,"ExclusiveCheck",parts)==1,"native WORD/BYTE component truncation");
    parts[10]=int32_t(1);check(num(s,"ExclusiveCheck",parts)==0,"nonzero byte differs");
    parts.assign(12,int32_t(0));check(num(s,"ExclusiveCheck",parts)==0,"non-eleven argc uses default GUID");
}
}
int main(){try{strings();trig();timers();queries();exclusive();std::cout<<"services: 5 groups passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

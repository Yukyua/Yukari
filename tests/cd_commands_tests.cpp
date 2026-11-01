#include "yukari/cd_commands.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace yukari;
static void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
static HostCall command(std::string name,std::vector<Value> args={}){HostCall call;call.function=std::move(name);call.arguments=std::move(args);return call;}
int main(){try{
 CdCommands absent;
 check(!absent.handle(command("unrecognized")),"unknown must not be accepted");
 check(!absent.handle(command("CD",{int32_t(1)}))->result.waiting,"absent drive must not throw or wait for play");
 auto wait=absent.handle(command("CDWait",{int32_t(1500)}));
 check(wait->result.waiting&&wait->wait_ms==1500&&wait->click_interruptible,"unseeded clock must cause interruptible target wait");
 check(std::holds_alternative<std::monostate>(absent.handle(command("CDPlayTime"))->result.value),"CDPlayTime must return null");
 uint32_t time=100;int plays=0,stops=0,silences=0,names=0,loop_sets=0;bool last_loop=false,immediate=false;CdAcknowledgement outcome=CdAcknowledgement::Timeout;
 CdProvider provider;
 provider.now_ms=[&]{return time;};provider.play=[&](int32_t track,bool loop){check(track!=0,"zero track must stop");++plays;last_loop=loop;return outcome;};
 provider.stop=[&](bool flag){++stops;immediate=flag;};provider.silence=[&]{++silences;};provider.set_loop=[&](bool loop){++loop_sets;last_loop=loop;};
 provider.find_volume=[&](const std::string& prefix){++names;check(prefix=="GAME","name prefix");return false;};
 CdCommands cd(provider);
 cd.handle(command("CD",{int32_t(2),int32_t(1)}));check(plays==1&&last_loop&&cd.elapsed()==0,"timeout must set epochs");
 time=130;check(cd.elapsed(1)==30&&cd.elapsed(2)==30&&cd.elapsed(3)==0,"clock selection");
 cd.handle(command("CD",{int32_t(2),int32_t(0)}));
 check(plays==1&&cd.state().device_loop&&!cd.state().script_loop,"same-track request must not change worker loop");
 cd.handle(command("CDPlayMode",{int32_t(2)}));check(loop_sets==1&&!last_loop,"PlayMode exact one");
 cd.handle(command("CDSilentSet",{int32_t(-1)}));check(silences==1&&std::get<int32_t>(cd.handle(command("CDSilentGet"))->result.value)==1,"silent bool");
 cd.handle(command("CDSilentSet",{int32_t(0)}));check(stops==1&&cd.state().device_track==0&&cd.elapsed()==30,"unmute nonloop stops, retains epochs");
 cd.handle(command("CD",{int32_t(3),int32_t(1)}));time=160;
 cd.handle(command("CDSilentSet",{int32_t(1)}));cd.handle(command("CDSilentSet",{int32_t(0)}));check(plays==3&&last_loop&&cd.elapsed()==0,"unmute loop restarts even same track");
 outcome=CdAcknowledgement::Failed;time=200;
 cd.handle(command("CD",{int32_t(4)}));check(cd.elapsed()==40,"backend failure must not reseed epochs or throw");
 cd.handle(command("CD",{int32_t(4)}),CdPolicy{true,true});check(immediate&&plays==5,"immediate restart must stop before play");
 cd.set_clock_epochs(std::numeric_limits<uint32_t>::max()-9,std::numeric_limits<uint32_t>::max()-9);time=5;check(cd.elapsed()==15,"32-bit timer wraps");
 check(cd.handle(command("CDWait",{int32_t(20)}))->wait_ms==5,"wait remaining duration");
 check(!cd.handle(command("CDWait",{int32_t(15)}))->result.waiting,"reached target no wait");
 check(!cd.handle(command("CDWait",{int32_t(-1)}))->result.waiting,"negative target no wait");
 cd.handle(command("CDNameSet",{std::string("GAME")}));cd.handle(command("CDNameSet",{std::string("other")}));check(names==1,"name discovery one shot even failure");
 CdCommands disabled(provider);disabled.handle(command("CDNameSet",{std::string("GAME")}),CdPolicy{false,false});disabled.handle(command("CDNameSet",{std::string("GAME")}));check(names==1,"disabled discovery consumes one shot");
 const auto snapshot=cd.state();CdCommands restored(provider);restored.restore(snapshot);check(restored.elapsed()==cd.elapsed()&&restored.state().name_attempted,"state import");
 bool failed=false;try{cd.handle(command("CDWait",{std::string("x")}));}catch(const std::runtime_error&){failed=true;}check(failed,"argument type failure");
 std::cout<<"PASS CD absent device, clocks, mode, silent restart, one-shot volume discovery and wait contracts\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

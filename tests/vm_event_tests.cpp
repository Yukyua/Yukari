#include "yukari/vm.hpp"
#include "yukari/save.hpp"
#include <cassert>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace yukari;
struct Program {
 Script s;uint32_t pc=0;
 uint32_t add(uint32_t type,std::string text={},int32_t value=0){uint32_t i=s.elements.size();s.elements.push_back({type,0,0,0,std::move(text),value});return i;}
 uint32_t emit(uint32_t e,std::vector<uint32_t> args={}){uint32_t p=pc;pc+=s.elements[e].type==0?2+args.size():1;s.instructions.push_back({p,e,args});return p;}
 uint32_t call(std::string name,std::vector<uint32_t> args={}){return emit(add(0,name),args);}
};
void subcalls(){
 Program root,child,grand;
 root.call("ScriptSubCall",{root.add(5,"child")});root.call("Observe");root.call("Return");
 child.call("Observe");child.call("ScriptCall",{child.add(5,"grand")});child.call("Return");grand.call("Observe");grand.call("Return");
 std::map<std::string,Script> scripts{{"root",root.s},{"child",child.s},{"grand",grand.s}};
 for(bool async:{false,true}){
  Vm vm([&](const std::string& n){return scripts.at(n);});int checkpoint=0,observed=0;
  vm.set_host([&](const HostCall& c)->std::optional<HostResult>{if(c.function=="ScriptSubCall"){++checkpoint;assert(!c.subcall);return HostResult{async};}if(c.function=="Observe"){assert(vm.current_frame_skip_mode()==(observed==1?2u:3u));assert(vm.submenu_enabled()==(observed!=1));if(observed==0){vm.set_current_frame_skip_mode(2);vm.set_submenu_enabled(false);}assert(c.subcall==(observed<2));assert(vm.in_subcall()==c.subcall);++observed;return HostResult{};}return std::nullopt;});
  vm.start("root");assert(vm.current_frame_skip_mode()==0&&!vm.submenu_enabled());vm.set_current_frame_skip_mode(3);vm.set_submenu_enabled(true);if(async){assert(vm.run()==VmStatus::Waiting);assert(vm.pending_subcall());auto data=vm.snapshot();vm.restore(data);assert(vm.pending_subcall());vm.resume();assert(vm.in_subcall()&&!vm.pending_subcall());}
  assert(vm.run(1)==VmStatus::StepLimit);auto data=vm.snapshot();vm.restore(data);assert(vm.run()==VmStatus::Halted);assert(checkpoint==1&&observed==3&&!vm.in_subcall());
 }
 Vm bad([&](const std::string& n){return scripts.at(n);});bad.start("root");bool rejected=false;try{bad.run();}catch(const std::runtime_error&){rejected=true;}assert(rejected);
}
void interrupts(){
 Program p;p.s.max_locals=1;auto local=p.add(10);
 auto open=p.add(1,"{"),close=p.add(1,"}"),eventopen=p.add(1,"{"),eventclose=p.add(1,"}");
 p.call("Select");auto begin=p.emit(open);p.call("TimeInterrupt");auto event=p.emit(eventopen);p.emit(local);p.call("Update");p.call("Wait");p.emit(local);p.call("Update");auto eventend=p.emit(eventclose);auto selectionend=p.emit(close);p.call("After");p.call("Return",{local});
 p.s.elements[open].value=selectionend;p.s.elements[close].value=begin;p.s.elements[eventopen].value=eventend;p.s.elements[eventclose].value=event;
 Vm vm([&](const std::string&){return p.s;});int updates=0,after=0;
 vm.set_host([&](const HostCall& c)->std::optional<HostResult>{if(c.function=="Select")return HostResult{false,{},true};if(c.function=="TimeInterrupt")return HostResult{false,{},false,true};if(c.function=="Update"){++updates;vm.set_variable("Flag",1,int32_t(updates));return HostResult{false,int32_t(updates)};}if(c.function=="Wait")return HostResult{true};if(c.function=="After"){++after;return HostResult{};}return std::nullopt;});
 vm.start("select");assert(vm.run()==VmStatus::Waiting);
 for(int i=0;i<2;++i){vm.interrupt_selection(event,eventend);assert(vm.run()==VmStatus::Waiting);assert(updates==2*i+1);auto data=vm.snapshot();vm.restore(data);vm.resume();assert(vm.run()==VmStatus::Waiting);assert(updates==2*i+2&&after==0);assert(std::get<int32_t>(vm.variable("Flag",1))==updates);}
 vm.choose(selectionend);assert(vm.run()==VmStatus::Halted&&after==1);assert(std::get<int32_t>(vm.last_result())==4);
}
void reset_vm(){
 Program p;p.call("Wait");p.call("Return");Vm vm([&](const std::string& n){if(n=="bad")throw std::runtime_error("missing");return p.s;});int hosts=0,traces=0;
 vm.set_host([&](const HostCall&)->std::optional<HostResult>{++hosts;return HostResult{true};});vm.set_trace([&](const VmLocation&,const std::string&){++traces;});vm.start("root");assert(vm.run()==VmStatus::Waiting);
 vm.set_current_frame_skip_mode(3);vm.set_submenu_enabled(true);auto native=vm.export_original();assert(native.frames.back().mode==3&&native.frames.back().header[0x3c]==1);Vm restored([&](const std::string&){return p.s;});restored.import_original(native);assert(restored.current_frame_skip_mode()==3&&restored.submenu_enabled());
 vm.set_variable("Flag",1,int32_t(5));vm.set_variable("GlobalFlag",1,int32_t(7));vm.set_variable("GlobalString",2,std::string("kept"));
 bool rejected=false;try{vm.reset("bad");}catch(const std::runtime_error&){rejected=true;}assert(rejected);assert(vm.run()==VmStatus::Waiting);
 vm.reset("root");assert(vm.current_frame_skip_mode()==0&&!vm.submenu_enabled());assert(vm.steps()==0&&vm.is_already("root",0));assert(std::get<int32_t>(vm.variable("Flag",1))==0);assert(std::get<int32_t>(vm.variable("GlobalFlag",1))==7);assert(std::get<std::string>(vm.variable("GlobalString",2))=="kept");assert(vm.run()==VmStatus::Waiting&&vm.current_already());assert(hosts==2&&traces==2);
}
void menu_injection(){
 Program root,menu;root.call("Wait");root.call("Return");menu.call("Observe");menu.call("Return");
 Vm vm([&](const std::string& n){return n=="root"?root.s:menu.s;});int observed=0;
 vm.set_host([&](const HostCall& c)->std::optional<HostResult>{if(c.function=="Wait")return HostResult{true};assert(c.function=="Observe"&&c.subcall&&vm.in_subcall());assert(vm.current_frame_skip_mode()==3&&!vm.submenu_enabled());++observed;return HostResult{};});
 vm.start("root");vm.set_current_frame_skip_mode(3);assert(vm.run()==VmStatus::Waiting&&!vm.pending_subcall());vm.call("menu",true);assert(vm.run(1)==VmStatus::StepLimit);auto state=vm.snapshot();vm.restore(state);assert(vm.in_subcall());assert(vm.run()==VmStatus::Waiting&&observed==1&&!vm.in_subcall()&&!vm.in_call());vm.resume();assert(vm.run()==VmStatus::Halted);
}
int main(){subcalls();interrupts();reset_vm();menu_injection();std::cout<<"SubCall checkpoint/inheritance and selection event restoration passed\n";}

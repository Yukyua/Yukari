#include "yukari/vm.hpp"
#include "yukari/save.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace yukari;
struct Program{Script s;uint32_t pc=0;uint32_t add(uint32_t type,std::string text={},int32_t value=0){uint32_t id=s.elements.size();s.elements.push_back({type,0,0,0,text,value});return id;}void emit(uint32_t e,std::vector<uint32_t> args={}){s.instructions.push_back({pc,e,args});pc+=s.elements[e].type==0?2+args.size():1;}void call(std::string name,std::vector<uint32_t> args={}){emit(add(0,name),args);}};
int main(){
 Program root,child;root.s.max_locals=1;auto local=root.add(10);root.emit(local);root.call("SelectID");root.call("Observe",{local});root.call("SelectID",{root.add(4,{},37)});root.call("ScriptCall",{root.add(5,"child")});root.emit(local);root.call("SelectID");root.call("Observe",{local});root.call("Return",{local});
 child.call("Observe",{});child.call("SelectID",{child.add(4,{},99)});child.call("Pause");child.call("Return");
 auto loader=[&](const std::string& n){return n=="root"?root.s:child.s;};Vm vm(loader);int observes=0;
 vm.set_host([&](const HostCall& call)->std::optional<HostResult>{if(call.function=="Pause")return HostResult{true};if(call.function=="Observe"){if(observes==0){assert(!vm.current_frame_select_id());assert(std::get<int32_t>(call.arguments.at(0))==-1);}else if(observes==1)assert(!vm.current_frame_select_id());else{assert(vm.current_frame_select_id()==37);assert(std::get<int32_t>(call.arguments.at(0))==37);}++observes;return HostResult{};}throw std::runtime_error("SelectID unexpectedly escaped to host");});
 vm.start("root");assert(vm.run()==VmStatus::Waiting&&vm.current_frame_select_id()==99);auto native=vm.export_original();assert(native.frames.size()==2&&u32(native.frames[0].header,0x48)==1&&u32(native.frames[0].header,0x4c)==37&&u32(native.frames[1].header,0x4c)==99);
 vm.set_current_frame_select_id(-1);auto negative=vm.export_original();assert(u32(negative.frames.back().header,0x48)==1&&u32(negative.frames.back().header,0x4c)==0xffffffff);vm.set_current_frame_select_id(99);
 Vm imported(loader);imported.import_original(native);assert(imported.current_frame_select_id()==99);auto inactive=native;for(unsigned i=0;i<4;++i)inactive.frames.back().header[0x48+i]=0;inactive.frames.back().header[0x4c]=0x78;inactive.frames.back().header[0x4d]=0x56;inactive.frames.back().header[0x4e]=0x34;inactive.frames.back().header[0x4f]=0x12;imported.import_original(inactive);assert(!imported.current_frame_select_id()&&u32(imported.export_original().frames.back().header,0x4c)==0x12345678);auto preserved=imported.snapshot();imported.restore(preserved);assert(!imported.current_frame_select_id()&&u32(imported.export_original().frames.back().header,0x4c)==0x12345678);auto saved=vm.snapshot();vm.restore(saved);assert(vm.current_frame_select_id()==99);vm.resume();assert(vm.run()==VmStatus::Halted&&observes==3&&std::get<int32_t>(vm.last_result())==37);
 std::cout<<"SelectID unset/set, per-frame isolation, native fields and snapshot restore passed\n";
}

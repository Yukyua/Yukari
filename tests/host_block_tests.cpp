#include "yukari/vm.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace yukari;
namespace {
Script guarded(){
 Script s;
 s.elements={{0,0,0,0,"Guard",0},{1,0,0,0,"{",9},{0,0,0,0,"DeadCode",0},{1,0,0,0,"{",8},{0,0,0,0,"NestedDeadCode",0},{1,0,0,0,"}",5},{1,0,0,0,"}",2},{0,0,0,0,"Reached",0},{0,0,0,0,"Return",0}};
 s.instructions={{0,0,{}},{2,1,{}},{3,2,{}},{5,3,{}},{6,4,{}},{8,5,{}},{9,6,{}},{10,7,{}},{12,8,{}}};return s;
}
}
int main(){
 for(bool waiting:{false,true}){
  Vm vm([](const std::string&){return guarded();});int reached=0;
  vm.set_host([&](const HostCall& c)->std::optional<HostResult>{if(c.function=="Guard"){assert(c.block_begin==2&&c.block_end==9);return HostResult{waiting,{},false,true};}if(c.function=="Reached"){++reached;return HostResult{};}throw std::runtime_error("skipped body was executed");});
  vm.start("block.yks");if(waiting){assert(vm.run()==VmStatus::Waiting);auto state=vm.snapshot();vm.restore(state);vm.resume();}assert(vm.run()==VmStatus::Halted&&reached==1);
 }
 Script s;s.elements={{0,0,0,0,"Guard",0},{0,0,0,0,"Return",0}};s.instructions={{0,0,{}},{2,1,{}}};Vm vm([&](const std::string&){return s;});vm.set_host([](const HostCall&)->std::optional<HostResult>{return HostResult{false,{},false,true};});vm.start("no-block.yks");assert(vm.run()==VmStatus::Halted);
 Vm bad([](const std::string&){return guarded();});bad.set_host([](const HostCall&)->std::optional<HostResult>{return HostResult{false,{},true,true};});bad.start("bad.yks");bool rejected=false;try{bad.run();}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 std::cout<<"host conditional block bypass, nested body and async resume passed\n";
}

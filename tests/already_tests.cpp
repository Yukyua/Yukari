#include "yukari/already.hpp"
#include "yukari/vm.hpp"
#include "yukari/save.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace yukari;
namespace {
void put(Bytes& b,size_t p,uint32_t v){for(unsigned i=0;i<4;++i)b.at(p+i)=uint8_t(v>>(i*8));}
void rejects(const std::function<void()>& f){bool threw=false;try{f();}catch(const std::runtime_error&){threw=true;}assert(threw);}
Script program(){Script s;s.elements={{0,0,0,0,"Observe",0},{4,0,0,0,"",5},{0,0,0,0,"Return",0}};s.instructions={{0,0,{1}},{3,2,{}}};return s;}
}
int main(){
 AlreadyData a;a.bind("日本語.yks",8);assert(!a.read("日本語.yks",0));assert(!a.mark("日本語.yks",0));assert(a.mark("日本語.yks",0));assert(!a.read("日本語.yks",7));rejects([&]{a.mark("日本語.yks",8);});auto bytes=encode_already(a);auto b=decode_already(bytes);assert(b.entries()==a.entries());a.bind("日本語.yks",9);assert(!a.read("日本語.yks",0));a.bind("日本語.YKS",8);assert(a.read("日本語.yks",0));assert(a.entries().size()==2);assert(decode_already(encode_already(a)).entries()==a.entries());
 // The hot visitor must survive bank insertion, count/case rebind and copying.
 AlreadyData hot;assert(!hot.mark("HOT.YKS",1,4));assert(hot.mark("HOT.YKS",1,4));
 auto copied=hot;assert(!copied.mark("HOT.YKS",2,4));assert(!hot.read("hot.yks",2));
 AlreadyData assigned;assigned=hot;assert(!assigned.mark("HOT.YKS",3,4));assert(!hot.read("hot.yks",3));
 auto moved=std::move(copied);assert(moved.mark("HOT.YKS",2,4));assert(!copied.mark("other",0,2));
 assigned=std::move(moved);assert(assigned.mark("HOT.YKS",2,4));
 hot.bind("hot.yks",5);assert(!hot.mark("HOT.YKS",1,5));assert(hot.mark("HOT.YKS",1,4));
 rejects([&]{hot.mark("HOT.YKS",4,4);});
 auto corrupt=bytes;corrupt[0]='!';rejects([&]{decode_already(corrupt);});corrupt=bytes;put(corrupt,12,0xffffffff);rejects([&]{decode_already(corrupt);});corrupt=bytes;put(corrupt,u32(corrupt,16)+12,0xfffffff0);rejects([&]{decode_already(corrupt);});corrupt=bytes;put(corrupt,u32(corrupt,16)+4,1);rejects([&]{decode_already(corrupt);});
 Vm vm([](const std::string&){return program();});bool old=true;unsigned called=0;vm.set_host([&](const HostCall& c)->std::optional<HostResult>{assert(c.function=="Observe");old=vm.current_already();assert(vm.is_already(c.location.script,c.location.word));++called;return HostResult{true,{}};});
 vm.start("read.yks");auto initial=vm.snapshot();auto native=vm.export_original();assert(vm.run()==VmStatus::Waiting&&!old);assert(!vm.is_already("read.yks",1)&&!vm.is_already("read.yks",2));auto waiting_data=vm.save_already();assert(vm.run()==VmStatus::Waiting&&called==1&&!vm.current_already());vm.resume();assert(vm.run()==VmStatus::Halted);vm.start("read.yks");assert(vm.run()==VmStatus::Waiting&&old);vm.resume();assert(vm.run()==VmStatus::Halted);
 vm.restore(initial);assert(vm.is_already("read.yks",0)&&vm.is_already("read.yks",3));assert(vm.run()==VmStatus::Waiting&&old);vm.resume();assert(vm.run()==VmStatus::Halted);vm.import_original(native);assert(vm.is_already("read.yks",0));assert(vm.run()==VmStatus::Waiting&&old);
 Vm second([](const std::string&){return program();});second.load_already(waiting_data);second.set_host([&](const HostCall&)->std::optional<HostResult>{assert(second.current_already());return HostResult{true,{}};});second.start("read.yks");assert(second.run()==VmStatus::Waiting);
 auto prior=second.save_already();rejects([&]{second.load_already(Bytes{});});assert(second.save_already()==prior);
 std::cout<<"native already codec, pre-execution read order, waiting and snapshot preservation passed\n";
}

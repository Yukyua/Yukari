#include "yukari/text_schedule.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace yukari;
int main(){
 TextSchedule schedule({{1,"ABC",0},{2,"xy",0},{1,"!",0}});std::string observed;auto emit=[&](int id,std::string_view text){observed+=char('0'+id);observed+=text[0];return size_t(1);};
 auto round=schedule.cycle(emit);assert(round.status==TextScheduleCycle::RoundComplete&&!round.final_round&&observed=="1A2x1!"&&(round.changed_layers==std::vector<int32_t>{1,2}));assert(text_schedule_delay({25,false,false,false})==25);
 round=schedule.cycle(emit);assert(!round.final_round&&observed=="1A2x1!1B2y");round=schedule.cycle(emit);assert(round.final_round&&observed=="1A2x1!1B2y1C");assert(text_schedule_delay({25,false,false,false})==25);assert(schedule.cycle(emit).status==TextScheduleCycle::Done);
 TextSchedule blocked({{1,"AB",0},{2,"xy",0},{3,"Z",0}});bool page=true;observed.clear();auto page_emit=[&](int id,std::string_view text){if(id==2&&page)return size_t(0);return emit(id,text);};round=blocked.cycle(page_emit);assert(round.status==TextScheduleCycle::PageBlocked&&observed=="1A"&&blocked.state().next_target==1&&blocked.state().targets[0].offset==1);auto saved=blocked.state();TextSchedule restored;restored.restore(saved);page=false;round=restored.cycle(page_emit);assert(round.status==TextScheduleCycle::RoundComplete&&!round.final_round&&observed=="1A2x3Z");round=restored.cycle(page_emit);assert(round.final_round&&observed=="1A2x3Z1B2y");
 TextSchedule controls({{1,"@gf(7)A",0},{2,std::string("\x82\xa0Z",3),0}});std::vector<std::string> tokens;auto token_emit=[&](int id,std::string_view text){size_t n=text.rfind("@gf(",0)==0?6:(uint8_t(text[0])==0x82?2:1);tokens.push_back(std::to_string(id)+":"+std::string(text.substr(0,n)));return n;};round=controls.cycle(token_emit);assert(!round.final_round&&tokens[0]=="1:@gf(7)"&&tokens[1]==std::string("2:\x82\xa0",4));assert(controls.cycle(token_emit).final_round);
 assert(text_schedule_delay({25,true,false,false})==0&&text_schedule_delay({25,false,true,false})==0&&text_schedule_delay({25,false,false,true})==0&&text_schedule_delay({0,false,false,false})==0&&text_schedule_delay({-1,false,false,false})==0xffffffff);
 assert(text_schedule_latch(false,true,25,false,false,false));assert(!text_schedule_latch(true,true,25,false,false,false));assert(text_schedule_latch(false,false,25,false,true,true));assert(!text_schedule_latch(false,false,25,false,true,false));
 TextSchedule empty({{1,"",0},{2,std::string("X\0hidden",8),0}});assert(empty.cycle(emit).final_round&&empty.state().targets[1].offset==1);
 TextSchedule no_arguments;assert(no_arguments.cycle(emit).final_round&&no_arguments.cycle(emit).status==TextScheduleCycle::Done);
 bool failed=false;try{auto bad=saved;bad.next_target=100;restored.restore(bad);}catch(const std::runtime_error&){failed=true;}assert(failed&&restored.state().complete);
 std::cout<<"StrOut round-robin, per-round/final delay, blocked-page resume, control tokens, duplicate layers and fast latch passed\n";
}

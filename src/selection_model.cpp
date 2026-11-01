#include "yukari/selection_model.hpp"
#include <algorithm>
#include <stdexcept>
namespace yukari {
bool SelectionRect::contains(int32_t x,int32_t y)const{return left<=x&&x<=right&&top<=y&&y<=bottom;}
SelectionRegistrationResult decode_selection_case(const HostCall& c,SelectionItem& out,const SelectionRegistration& options){
 const auto& a=c.arguments;auto get=[&](size_t n,int32_t fallback=0){if(n>=a.size())return fallback;auto v=std::get_if<int32_t>(&a[n]);if(!v)throw std::runtime_error(c.function+" requires integer argument "+std::to_string(n));return *v;};
 auto rect=[&](size_t n){return SelectionRect{get(n),get(n+1),get(n+2),get(n+3)};};
 auto sprite=[&](size_t at,unsigned slot,size_t group){out.sprites[slot]={get(at),get(at+1),get(at+2),get(at+3),get(group,-1),0};};
 out={};out.branch=c.block_begin.value_or(0);out.end=c.block_end.value_or(0);
 if(c.function=="KeyCase")return SelectionRegistrationResult::NativeUnrecognized;
 if(c.function=="Case"||c.function=="Case2"){
  const bool conditional=c.function=="Case2";const size_t offset=conditional?1:0;
  if(conditional&&!get(0))return SelectionRegistrationResult::ConditionFalse;
  if(a.size()<=offset||!std::holds_alternative<std::string>(a[offset]))throw std::runtime_error(c.function+" requires display text");out.text=std::get<std::string>(a[offset]);out.clickable_cursor=true;
  if(a.size()>=offset+7)out.rect=rect(offset+3);else if(options.text_rect)out.rect=*options.text_rect;else throw std::runtime_error(c.function+" requires host text rectangle when explicit bounds are absent");
  out.enter_sound=get(offset+7,-1);out.select_sound=get(offset+8,-1);
 }
 else if(c.function=="MouseCase"||c.function=="MouseCase2"){
  const bool conditional=c.function=="MouseCase2";const size_t offset=conditional?1:0;
  if(a.size()<offset+4)throw std::runtime_error(c.function+" requires rectangle");out.rect=rect(offset);out.blocked=conditional&&!get(0);out.clickable_cursor=!out.blocked;
  if(a.size()>=offset+(conditional?11:10)){sprite(offset+4,0,offset+8);sprite(offset+4,1,offset+9);if(conditional)sprite(offset+4,2,offset+10);}
  out.enter_sound=get(offset+(conditional?11:10),-1);out.select_sound=get(offset+(conditional?12:11),-1);
  const int32_t flags=get(offset+(conditional?13:12),-1);if(flags>=0){if(flags&1)out.clickable_cursor=false;out.native_c9=flags&2;out.ignore_input=flags&4;out.navigation_excluded=flags&8;out.stop_hit_scan=!conditional&&(flags&0x40);out.loop_enter_sound=flags&0x80;}
 }
 else if(c.function=="IndicateCase"){
  if(a.size()<4)throw std::runtime_error("IndicateCase requires rectangle");out.rect=rect(0);out.kind=SelectionItem::Indicate;out.blocked=out.navigation_excluded=true;if(a.size()>=10){sprite(4,0,8);sprite(4,1,9);out.enter_sound=get(10,-1);}
 }
 else if(c.function=="SelectingKey"){
  if(a.size()<2||!get(1))return SelectionRegistrationResult::ConditionFalse;if(a.size()>17)throw std::runtime_error("SelectingKey exceeds native 16-key table");out.kind=SelectionItem::Selecting;out.relative_target=get(0);
  for(size_t i=a.size();i>1;--i)if(get(i-1))out.keys.push_back(get(i-1));
 }
 else if(c.function=="KeyOverPrev"||c.function=="KeyOverNext"){
  out.rect={0,0,0,0};out.key_over_prev=c.function=="KeyOverPrev";out.key_over_next=c.function=="KeyOverNext";
 }
 else if(c.function=="CursorMoveTypeIndex"||c.function=="CursorMoveTypeAuto"){
  out.kind=c.function=="CursorMoveTypeIndex"?SelectionItem::MoveIndex:SelectionItem::MoveAuto;out.rect={0,0,0,0};
 }
 else if(c.function=="MouseCase2Already"){
  if(a.size()<15)throw std::runtime_error("MouseCase2Already requires 15 arguments");out.blocked=get(0)==0;out.clickable_cursor=!out.blocked;out.rect=rect(1);
  sprite(5,0,options.already?12:9);sprite(5,1,options.already?13:10);sprite(5,2,options.already?14:11);out.enter_sound=get(15,-1);out.select_sound=get(16,-1);
 }
 else if(c.function=="MouseKeyCase"){
  if(a.empty()||get(0)<1||get(0)>5)return SelectionRegistrationResult::ConditionFalse;
  out.kind=SelectionItem::MouseKey;out.mouse_key=get(0);
  if(a.size()>1){if(a.size()<5)throw std::runtime_error("MouseKeyCase rectangle requires four coordinates");out.rect=rect(1);}
  if(a.size()>10){sprite(5,0,9);sprite(5,1,10);out.enter_sound=get(11,-1);out.select_sound=get(12,-1);}
  if(get(13,-1)>=0)out.allow_repeat=(get(13)&0x20)!=0;
 }
 else if(c.function=="KeyTableCase"){
  if(a.empty()||!get(0))return SelectionRegistrationResult::ConditionFalse;if(a.size()>16)throw std::runtime_error("KeyTableCase exceeds native 16-key table");out.kind=SelectionItem::KeyHistory;
  for(auto it=a.rbegin();it!=a.rend();++it){const auto v=std::get_if<int32_t>(&*it);if(!v)throw std::runtime_error("KeyTableCase requires integers");if(*v)out.keys.push_back(*v);}
 }
 else if(c.function=="WindowMessageCase"){if(a.empty())throw std::runtime_error("WindowMessageCase requires message");out.kind=SelectionItem::Message;out.message=get(0);}
 else if(c.function=="IndicateUsualCase"||c.function=="IndicateUsualTimeCase"){
  out.kind=SelectionItem::Indicate;out.blocked=out.navigation_excluded=true;const bool timed=c.function=="IndicateUsualTimeCase";
  if(a.size()>=size_t(timed?6:5)){const size_t base=timed?1:0;sprite(base,1,base+4);if(timed)out.sprites[1].delay_ms=get(0);out.enter_sound=get(base+5,-1);}
 }
 else if(c.function=="TogetherIndicateCase"){
  out.kind=SelectionItem::Together;out.blocked=out.navigation_excluded=true;if(a.size()>=6){sprite(0,0,4);sprite(0,1,5);}
 }
 else return SelectionRegistrationResult::Unhandled;
 return SelectionRegistrationResult::Added;
}
size_t SelectionModel::add(SelectionItem item){if(item.keys.size()>16)throw std::runtime_error("selection key table exceeds 16 entries");items_.push_back(std::move(item));visibility_.push_back({false,false,false});states_.push_back(0);return items_.size()-1;}
std::optional<size_t> SelectionModel::hit(int32_t x,int32_t y)const{
 std::optional<size_t> found;for(size_t i=0;i<items_.size();++i){const auto& item=items_[i];if(item.kind==SelectionItem::Mouse&&!item.blocked&&item.rect.contains(x,y)){found=i;if(item.stop_hit_scan)break;}}return found;
}
std::optional<SelectionCandidate> SelectionModel::candidate(const SelectionInput& in)const{
 auto result=[&](size_t i){return SelectionCandidate{i,items_[i].branch,items_[i].end};};
 if(in.query==SelectionInput::KeyDown){for(size_t i=0;i<items_.size();++i){const auto& item=items_[i];if(item.kind==SelectionItem::Key&&!item.blocked&&(item.allow_repeat||!(in.lparam&0x40000000))&&std::find(item.keys.begin(),item.keys.end(),int32_t(in.wparam))!=item.keys.end())return result(i);}}
 if(in.query==SelectionInput::KeyHistoryChanged)for(size_t i=0;i<items_.size();++i){const auto& item=items_[i];if(item.kind==SelectionItem::KeyHistory&&!item.blocked&&!item.keys.empty()&&std::equal(item.keys.begin(),item.keys.end(),in.key_history.begin()))return result(i);}
 if(in.query==SelectionInput::MouseInput)for(size_t i=0;i<items_.size();++i){const auto& item=items_[i];if(item.kind!=SelectionItem::MouseKey||item.blocked)continue;const auto& r=item.rect;if(!(r.left==0&&r.top==0&&r.right==0&&r.bottom==0)&&!r.contains(in.x,in.y))continue;const int k=item.mouse_key;bool match=false;if(k>=1&&k<=3)match=item.allow_repeat?in.mouse_held[size_t(k-1)]:in.message==uint32_t(0x201+(k-1)*3);else if(k==4||k==5){const int16_t delta=int16_t(in.wparam>>16);match=in.message==0x20a&&(k==4?delta>0:delta<0);}if(match)return result(i);}
 if(in.query==SelectionInput::SelectingKey&&in.focused)for(size_t i=0;i<items_.size();++i){const auto& item=items_[i];if((item.kind!=SelectionItem::Selecting&&item.kind!=SelectionItem::SelectingAlternate)||item.blocked||item.keys.empty())continue;const int64_t target=int64_t(i)+item.relative_target;if(target<0||target>=int64_t(items_.size())||size_t(target)!=*in.focused||item.keys[0]!=int32_t(in.wparam))continue;bool match=true;for(size_t k=1;k<item.keys.size();++k){const auto vk=item.keys[k];if(vk<0||vk>=256||!(in.key_state[size_t(vk)]&0x80))match=false;}if(match)return result(i);}
 std::optional<SelectionCandidate> message;if(in.query==SelectionInput::WindowMessage)for(size_t i=0;i<items_.size();++i)if(items_[i].kind==SelectionItem::Message&&uint32_t(items_[i].message)==in.message)message=result(i);return message;
}
SelectionNavigationResult SelectionModel::navigate(int32_t vk,std::optional<size_t> focused,const SelectionNavigationOptions& options)const{
 if(focused&&*focused>=items_.size())throw std::runtime_error("selection navigation focus outside item list");
 SelectionNavigationResult out{focused,false};auto eligible=[&](size_t i){return items_[i].kind==SelectionItem::Mouse&&!items_[i].blocked&&!items_[i].navigation_excluded;};auto has=[&](const std::vector<int32_t>& keys){return std::find(keys.begin(),keys.end(),vk)!=keys.end();};
 int32_t mode=options.mode;std::optional<size_t> marker;
 if(focused)for(size_t n=*focused+1;n;--n){const size_t i=n-1;if(items_[i].kind==SelectionItem::MoveIndex||items_[i].kind==SelectionItem::MoveAuto){marker=i;mode=items_[i].kind==SelectionItem::MoveIndex?1:0;break;}}
 if(options.tab_groups&&vk==9){
  std::optional<size_t> group;const size_t start=focused?*focused+1:0;
  for(size_t i=start;i<items_.size();++i)if(eligible(i)&&items_[i].key_over_prev){group=i;break;}
  if(!group)for(size_t i=0;i<items_.size();++i)if(eligible(i)&&items_[i].key_over_prev){group=i;break;}
  if(group)for(size_t i=*group+1;i<items_.size();++i)if(eligible(i)){if(items_[i].key_over_prev)continue;if(!items_[i].key_over_next){out.focused=i;return out;}break;}
 }
 if(mode==2){
  int direction=-1;if(vk==0x26||vk==0x68)direction=0;else if(vk==0x28||vk==0x62)direction=1;else if(vk==0x25||vk==0x64)direction=2;else if(vk==0x27||vk==0x66)direction=3;if(direction<0)return out;
  auto current=focused;if(!current)for(size_t i=0;i<items_.size();++i)if(eligible(i)){current=i;break;}
  for(size_t steps=0;current&&steps<items_.size();++steps){current=items_[*current].neighbors[size_t(direction)];if(current&&*current>=items_.size())throw std::runtime_error("selection neighbor outside item list");if(current&&eligible(*current)){out.focused=current;return out;}}return out;
 }
 if(mode==0){
  if(!focused)return out;int direction=0;if(has(options.up))direction=1;if(has(options.down))direction=2;if(has(options.left))direction=3;if(has(options.right))direction=4;if(!direction)return out;
  auto center=[](const SelectionRect& r){return std::pair<float,float>{float((int64_t(r.right)-r.left)/2+r.left),float((int64_t(r.bottom)-r.top)/2+r.top)};};
  const auto origin=center(items_[*focused].rect);float best=0;const size_t start=marker?*marker+1:0;
  for(size_t i=start;i<items_.size();++i){if(items_[i].kind==SelectionItem::MoveIndex||items_[i].kind==SelectionItem::MoveAuto)break;if(i==*focused||!eligible(i))continue;const auto point=center(items_[i].rect);const float dx=point.first-origin.first,dy=point.second-origin.second;
   if((direction==1&&dy>=0)||(direction==2&&dy<=0)||(direction==3&&dx>=0)||(direction==4&&dx<=0))continue;
   const float distance=direction<3?float(double(dy)*dy*.5+double(dx)*dx):float(double(dx)*dx*.5+double(dy)*dy);if(distance!=0&&(best==0||distance<best)){best=distance;out.focused=i;}
  }return out;
 }
 int step=has(options.previous)?-1:0;if(has(options.next))step=1;if(!step)return out;
 int64_t at=focused?int64_t(*focused)+step:(step>0?0:int64_t(items_.size())-1);
 for(;at>=0&&at<int64_t(items_.size());at+=step)if(eligible(size_t(at))){out.focused=size_t(at);out.activate=step<0?items_[size_t(at)].key_over_prev:items_[size_t(at)].key_over_next;break;}return out;
}
SelectionUpdate SelectionModel::update(const SelectionInput& in,std::optional<size_t> focused){
 if(focused&&*focused>=items_.size())throw std::runtime_error("selection focus outside item list");SelectionUpdate out;out.candidate=candidate(in);
 for(size_t i=0;i<items_.size();++i){const auto& item=items_[i];const bool indicate=item.kind==SelectionItem::Indicate;
  const bool on=indicate?item.rect.contains(in.x,in.y):(focused&&(*focused==i||(item.kind==SelectionItem::Together&&i&&*focused==i-1)));
  const int state=on?2:((!indicate&&item.blocked)?4:1);if(indicate&&states_[i]==state)continue;
  const std::array<bool,3> desired={state==1,state==2,state==4};
  for(unsigned slot=0;slot<3;++slot)if(item.sprites[slot].present()&&visibility_[i][slot]!=desired[slot]){out.sprites.push_back({i,slot,desired[slot],desired[slot]&&slot!=2});visibility_[i][slot]=desired[slot];}
  if(on&&item.enter_sound>=0&&(indicate||states_[i]!=state))out.play_sounds.push_back(item.enter_sound);
  else if(!on&&item.loop_enter_sound&&item.enter_sound>=0)out.stop_sounds.push_back(item.enter_sound);
  states_[i]=state;
 }
 return out;
}
}

namespace yukari {
SelectionRegistrationResult SelectionDeclarations::consume(const HostCall& c,const SelectionRegistration& options){
 auto get=[&](size_t at,int32_t fallback=0){if(at>=c.arguments.size())return fallback;const auto value=std::get_if<int32_t>(&c.arguments[at]);if(!value)throw std::runtime_error(c.function+" requires integer argument "+std::to_string(at));return *value;};
 if(c.function=="SelectDisable"){disable_navigation=!c.arguments.empty()&&get(0)!=0;return SelectionRegistrationResult::Added;}
 if(c.function=="TabGrurpCheng"){navigation.tab_groups=true;return SelectionRegistrationResult::Added;}
 if(c.function=="TimeInterrupt"||c.function=="IntervalInterrupt"){
  if(c.arguments.empty())return SelectionRegistrationResult::ConditionFalse;
  const int32_t delay=get(0);timers.push_back({delay,c.function=="IntervalInterrupt"?delay:0,c.block_begin.value_or(0),c.block_end.value_or(0),c.function=="IntervalInterrupt"});return SelectionRegistrationResult::Added;
 }
 if(c.function=="IndicateString"||c.function=="IndicateStringC"){
  if(options.text_render_mode!=0)return SelectionRegistrationResult::ConditionFalse;
  if(c.arguments.size()<3||!std::holds_alternative<std::string>(c.arguments[0]))throw std::runtime_error(c.function+" requires text,x,y");
  decorations.push_back({{},std::get<std::string>(c.arguments[0]),get(1),get(2),c.function=="IndicateStringC"});return SelectionRegistrationResult::Added;
 }
 if(c.function=="AlreadyIndicatePreceding"){
  if(c.arguments.size()!=4&&c.arguments.size()!=6)return SelectionRegistrationResult::ConditionFalse;
  const bool explicit_position=c.arguments.size()==6;const int32_t x=explicit_position?get(2):preceding_text_position_.first,y=explicit_position?get(3):preceding_text_position_.second;
  const size_t groups=explicit_position?4:2;const int32_t group=get(groups+(preceding_already_?1:0));if(group>=0)decorations.push_back({SelectionSprite{get(0),get(1),x,y,group,0},{},x,y,false});return SelectionRegistrationResult::Added;
 }
 SelectionItem item;auto result=decode_selection_case(c,item,options);if(result!=SelectionRegistrationResult::Added)return result;
 if((c.function=="MouseCase"||c.function=="MouseCase2")&&disable_navigation)item.navigation_excluded=true;
 if(c.function=="Case"||c.function=="Case2"){
  preceding_already_=options.already;const size_t start=c.function=="Case2"?2:1;
  if(c.arguments.size()>=start+2&&get(start)>=0&&get(start+1)>=0)preceding_text_position_={get(start),get(start+1)};
  else if(options.text_position)preceding_text_position_=*options.text_position;
 }
 else if(c.function=="MouseCase"||c.function=="MouseCase2"||c.function=="MouseCase2Already"||c.function=="MouseKeyCase"||c.function=="KeyTableCase")preceding_already_=options.already;
 model.add(std::move(item));return result;
}
}

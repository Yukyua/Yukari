#include "yukari/history_view.hpp"
#include <cstring>
#include <algorithm>
#include <stdexcept>
namespace yukari {
namespace {
int32_t signed_word(uint32_t value){int32_t result;std::memcpy(&result,&value,4);return result;}
}
std::vector<HistoryPage> decode_history_pages(const Bytes& bytes,const char* encoding){
 std::vector<HistoryPage> pages;size_t at=0,total_records=0;
 while(at<bytes.size()){
  if(pages.size()>=1000)throw std::runtime_error("history exceeds native 1000-page ring");
  require_range(bytes.size(),at,4,"history record count");const auto count=u32(bytes,at);at+=4;
  if(count>100000||count>1000000-total_records)throw std::runtime_error("history record limit exceeded");total_records+=count;require_range(bytes.size(),at,size_t(count)*28,"history glyph records");
  HistoryPage page;page.glyphs.reserve(count);
  for(uint32_t i=0;i<count;++i){const auto p=at+size_t(i)*28;page.glyphs.push_back({signed_word(u32(bytes,p)),signed_word(u32(bytes,p+4)),signed_word(u32(bytes,p+8)),signed_word(u32(bytes,p+12)),u32(bytes,p+16),signed_word(u32(bytes,p+20)),u32(bytes,p+24)});}
  at+=size_t(count)*28;require_range(bytes.size(),at,4,"history voice size");const auto length=u32(bytes,at);at+=4;require_range(bytes.size(),at,length,"history voice string");
  if(length){if(bytes[at+length-1])throw std::runtime_error("history voice lacks terminating NUL");size_t n=0;while(n<length&&bytes[at+n])++n;page.voice=decode(std::string(bytes.begin()+at,bytes.begin()+at+n),encoding);}
  at+=length;pages.push_back(std::move(page));
 }return pages;
}
}

namespace yukari {
namespace {
int32_t setting(const SettingsState& state,const std::string& key,int32_t fallback=0){const auto it=state.values.find(key);if(it==state.values.end())return fallback;if(const auto* n=std::get_if<int32_t>(&it->second))return *n;throw std::runtime_error("history setting is not integer: "+key);}
int32_t integer(const Value& value){if(auto p=std::get_if<int32_t>(&value))return *p;throw std::runtime_error("history configuration requires integer");}
int32_t difference(int32_t a,int32_t b){const auto n=int64_t(a)-b;if(n<INT32_MIN||n>INT32_MAX)throw std::runtime_error("history coordinate overflow");return int32_t(n);}
int32_t sum(int32_t a,int32_t b){const auto n=int64_t(a)+b;if(n<INT32_MIN||n>INT32_MAX)throw std::runtime_error("history coordinate overflow");return int32_t(n);}
bool contains(const AreaBounds& r,int32_t x,int32_t y){return x>=r.left&&x<=r.right&&y>=r.top&&y<=r.bottom;}
void unite(AreaBounds& a,const AreaBounds& b){if(b.right<=b.left||b.bottom<=b.top)return;if(a.right<=a.left||a.bottom<=a.top){a=b;return;}a.left=std::min(a.left,b.left);a.top=std::min(a.top,b.top);a.right=std::max(a.right,b.right);a.bottom=std::max(a.bottom,b.bottom);}
}
HistoryOptions HistoryOptions::from_settings(const SettingsState& state,int32_t default_layer){
 HistoryOptions out;out.default_layer=default_layer;out.window_enabled=setting(state,"HistoryWindowEnabled")!=0;out.window_layer=setting(state,"HistoryWindow");out.text_color_enabled=setting(state,"HistoryTextColorEnabled")!=0;out.text_color=uint32_t(setting(state,"HistoryTextColor",-1));out.voiced_text_color=uint32_t(setting(state,"HistoryVoicedTextColor",setting(state,"HistoryTextBorderColor",-1)));out.voice_continue=setting(state,"HistoryToVoiceContinue")!=0;out.exit_to_next=setting(state,"HistoryExitToNext")!=0;
 if(setting(state,"HistorySoundNumberEnabled"))out.sound_number=setting(state,"HistorySoundNumber");
 if(auto it=state.records.find("HistoryWindowRect");it!=state.records.end()&&it->second.size()==4)out.window_rect=AreaBounds{integer(it->second[0]),integer(it->second[1]),integer(it->second[2]),integer(it->second[3])};
 if(auto it=state.records.find("HistorySoundIconSet");it!=state.records.end()&&it->second.size()==3)out.sound_icon=std::array<int32_t,3>{integer(it->second[0]),integer(it->second[1]),integer(it->second[2])};
 return out;
}
HistoryView::HistoryView(HistoryHooks hooks):hooks_(std::move(hooks)){}
HistoryView::PageLayout HistoryView::prepare(size_t page_index){
 PageLayout out;out.voice_layer=options_.window_enabled?options_.window_layer:options_.default_layer;
 int32_t layer=out.voice_layer;auto target=hooks_.layer(layer);if(!target)throw std::runtime_error("history target layer is missing");if(options_.window_enabled&&options_.window_rect)target->text_rect=*options_.window_rect;
 int32_t previous=pages_[page_index].glyphs.empty()?0:!pages_[page_index].glyphs.front().layer;
 int32_t group_y=0,anchor_x=0,first_end=0,last_end=0;bool anchor=options_.window_enabled;
 for(const auto& record:pages_[page_index].glyphs){
  if(record.kind==7){anchor=true;continue;}if(record.kind<3||record.kind>5)continue;
  if(previous!=record.layer){
   if(options_.window_enabled){group_y=sum(out.bounds.bottom,options_.line_margin);anchor=true;}
   else if(auto next=hooks_.layer(record.layer)){target=next;layer=record.layer;out.voice_layer=layer;}
   previous=record.layer;
  }
  if(anchor)anchor_x=record.x;anchor=false;
  if(record.kind==3&&!(record.character&255))continue;
  auto extent=hooks_.glyph_extent(record);if(extent.width<0||extent.height<0)throw std::runtime_error("negative history glyph extent");
  const auto relative_y=sum(record.y,group_y);const auto x=sum(difference(target->text_rect.left,anchor_x),record.x),y=sum(target->text_rect.top,relative_y);
  HistoryDrawGlyph draw;draw.page=page_index;draw.glyph=record;draw.layer=layer;draw.x=sum(target->x,x);draw.y=sum(target->y,y);draw.width=extent.width;draw.height=extent.height;draw.color=record.color;
  if(options_.text_color_enabled&&record.kind!=5){const auto color=pages_[page_index].voice.empty()?options_.text_color:options_.voiced_text_color;if(color!=0xffffffffU)draw.color=color;}
  if(record.kind==3){std::string encoded(1,char(record.character&255));if(record.character&0xff00)encoded+=char((record.character>>8)&255);draw.utf8=decode(encoded,options_.encoding.c_str());}
  out.glyphs.push_back(std::move(draw));unite(out.bounds,{record.x,relative_y,sum(record.x,extent.width),sum(relative_y,extent.height)});last_end=sum(x,extent.width);if(group_y==0&&record.y==0)first_end=std::max(first_end,last_end);
 }
 out.bounds.bottom=sum(out.bounds.bottom,options_.rows_margin);
 if(!pages_[page_index].voice.empty()&&options_.sound_icon){
  if(!hooks_.icon_extent)throw std::runtime_error("history voice icons require actual group extent provider");
  const auto source=(*options_.sound_icon)[0],normal=(*options_.sound_icon)[1],hover=(*options_.sound_icon)[2];
  HistoryDrawIcon icon;icon.page=page_index;icon.source=source;icon.normal=normal;icon.hover=hover;
  auto frame=[&](int32_t group)->std::optional<HistoryIconFrame>{
   if(group<0)return {};auto extent=hooks_.icon_extent(source,group);if(extent.width<0||extent.height<0)throw std::runtime_error("negative history icon extent");int32_t x=0,y=0;const auto bottom=difference(out.bounds.bottom,options_.rows_margin);
   switch(options_.icon_position_type){case 1:case 4:case 9:y=difference(bottom,out.bounds.top)/2-extent.height/2;break;case 2:y=sum(difference(bottom,extent.height),5);break;case 5:case 7:case 10:y=difference(bottom,extent.height);break;default:break;}
   switch(options_.icon_position_type){case 3:case 4:case 5:x=out.bounds.right;break;case 6:x=first_end;break;case 7:x=last_end;break;case 8:case 9:case 10:x=-extent.width;break;default:break;}
   x=sum(x,options_.icon_x);y=sum(y,options_.icon_y);unite(out.bounds,{x,y,sum(x,extent.width),sum(y,extent.height)});
   return HistoryIconFrame{group,sum(sum(x,target->text_rect.left),target->x),sum(sum(y,target->text_rect.top),target->y),extent.width,extent.height};
  };
  icon.normal_frame=frame(normal);icon.hover_frame=frame(hover);const auto selected=icon.normal_frame?icon.normal_frame:icon.hover_frame;
  if(selected){icon.x=selected->x;icon.y=selected->y;icon.width=selected->width;icon.height=selected->height;out.icon=std::move(icon);}
 }
 out.height=std::max(0,out.bounds.bottom);return out;
}
void HistoryView::open(std::vector<HistoryPage> pages,HistoryOptions options,const AreaServices* areas){
 if(!hooks_.layer||!hooks_.glyph_extent)throw std::runtime_error("history requires original layer and glyph providers");if(pages.size()>1000)throw std::runtime_error("history exceeds native ring size");
 close();pages_=std::move(pages);options_=std::move(options);areas_=areas;prepared_.clear();layout_={};advance_=false;
 if(pages_.empty()&&!options_.window_enabled)return;
 size_t count=0;for(const auto& page:pages_){if(page.glyphs.size()>100000||page.glyphs.size()>1000000-count)throw std::runtime_error("history glyph limit exceeded");count+=page.glyphs.size();}
 for(size_t i=0;i<pages_.size();++i)prepared_.push_back(prepare(i));layout_.open=true;
 if(!pages_.empty()){layout_.last_start=pages_.size()-1;if(options_.window_enabled){auto layer=hooks_.layer(options_.window_layer);if(!layer)throw std::runtime_error("history window layer missing");const auto rect=options_.window_rect.value_or(layer->text_rect);const auto height=int64_t(rect.bottom)-rect.top;int64_t used=0;for(size_t i=pages_.size();i-->0;){if(used+prepared_[i].height>=height)break;used+=prepared_[i].height;layout_.last_start=i;}}layout_.first=layout_.last_start;}
 rebuild();
}
void HistoryView::rebuild(){
 layout_.glyphs.clear();layout_.icons.clear();layout_.regions.clear();layout_.pages.clear();layout_.right_exit_sound.reset();if(!layout_.open)return;
 int32_t offset=0;for(size_t i=layout_.first;i<prepared_.size();++i){
  if(!options_.window_enabled&&i!=layout_.first)break;auto target=hooks_.layer(options_.window_enabled?options_.window_layer:prepared_[i].voice_layer);if(!target)continue;const auto rect=options_.window_enabled?options_.window_rect.value_or(target->text_rect):target->text_rect;
  if(options_.window_enabled&&int64_t(offset)>int64_t(rect.bottom)-rect.top)break;layout_.pages.push_back(i);bool any_clipped=false;
  auto start=layout_.glyphs.size();for(auto glyph:prepared_[i].glyphs){glyph.y=sum(glyph.y,offset);if(options_.window_enabled&&int64_t(glyph.y)+glyph.height>int64_t(target->y)+rect.bottom){glyph.visible=false;any_clipped=true;}layout_.glyphs.push_back(std::move(glyph));}
  if(any_clipped&&options_.all_or_nothing_clip)for(size_t j=start;j<layout_.glyphs.size();++j)layout_.glyphs[j].visible=false;
  auto bounds=prepared_[i].bounds;bounds.left=sum(bounds.left,sum(target->x,rect.left));bounds.right=sum(bounds.right,sum(target->x,rect.left));bounds.top=sum(bounds.top,sum(sum(target->y,rect.top),offset));bounds.bottom=sum(difference(bounds.bottom,options_.rows_margin),sum(sum(target->y,rect.top),offset));
  if(!options_.window_enabled)bounds={sum(target->x,rect.left),sum(target->y,rect.top),sum(target->x,rect.right),sum(target->y,rect.bottom)};
  if(auto icon=prepared_[i].icon){icon->y=sum(icon->y,offset);if(icon->normal_frame)icon->normal_frame->y=sum(icon->normal_frame->y,offset);if(icon->hover_frame)icon->hover_frame->y=sum(icon->hover_frame->y,offset);if(!options_.window_enabled||int64_t(icon->y)+icon->height<=int64_t(target->y)+rect.bottom)layout_.icons.push_back(*icon);else bounds.bottom=std::min(bounds.bottom,sum(target->y,rect.bottom));}
  if(!pages_[i].voice.empty())layout_.regions.push_back({int32_t(10000+i),bounds,false,i,{}});
  offset=sum(offset,prepared_[i].height);
 }
 if(options_.screen_rect&&(!areas_||!areas_->definition("AreaSetHistoryExit")||!areas_->definition("AreaSetHistoryExit")->native[0]))layout_.regions.insert(layout_.regions.begin(),{0,*options_.screen_rect,false,{},{}});
 if(areas_){if(const auto* right=areas_->definition("AreaSetHistoryMouseR"))if(right->native[14]>=0)layout_.right_exit_sound=right->native[14];for(const auto& item:std::vector<std::pair<const char*,int32_t>>{{"AreaSetHistoryPrev",2},{"AreaSetHistoryNext",3},{"AreaSetHistoryPagePrev",8},{"AreaSetHistoryPageNext",9},{"AreaSetHistoryHead",10},{"AreaSetHistoryLast",11},{"AreaSetHistoryExit",6},{"AreaSetHistoryScrollBarH",12},{"AreaSetHistoryScrollBarV",13}}){const auto* area=areas_->definition(item.first);if(!area)continue;auto bounds=areas_->bounds(*area);if(!bounds)continue;bool disabled=(item.second==2||item.second==8||item.second==10)?layout_.first==0:(item.second==3||item.second==9||item.second==11)?layout_.first>=layout_.last_start:false;layout_.regions.push_back({item.second,*bounds,disabled,{},areas_->sprites(*area)});}
  int32_t label=14;for(const auto& area:areas_->state().scripts){if(area.history_indication){if(auto bounds=areas_->bounds(area))layout_.regions.push_back({label,*bounds,false,{},areas_->sprites(area)});}++label;}
 }
 for(auto& region:layout_.regions)if(region.action==13&&region.sprites&&hooks_.icon_extent){auto& sprite=*region.sprites;const auto group=sprite.normal_frame>=0?sprite.normal_frame:sprite.hover_frame;if(sprite.source>=0&&group>=0){const auto height=hooks_.icon_extent(sprite.source,group).height;const auto half=std::max(0,height)/2;const int64_t lo=int64_t(region.bounds.top)+half,hi=int64_t(region.bounds.bottom)-half;const auto center=hi>lo?lo+(layout_.last_start?int64_t(double(hi-lo)*double(layout_.first)/double(layout_.last_start)):0):lo;const auto parent=hooks_.layer(sprite.parent);sprite.y=int32_t(center-half-(parent?parent->y:0));}}
}
void HistoryView::move(int64_t amount){
 if(!layout_.open)return;const auto next=int64_t(layout_.first)+amount;
 if(next>int64_t(layout_.last_start)&&options_.exit_to_next){advance_=true;close();return;}
 layout_.first=static_cast<size_t>(std::max<int64_t>(0,std::min<int64_t>(int64_t(layout_.last_start),next)));rebuild();
}
bool HistoryView::action(int32_t label){
 if(!layout_.open)return false;
 if(label==0||label==6||label==7){close();return true;}
 if(label==2||label==4){move(-1);return true;}if(label==3||label==5){move(1);return true;}
 if(label==8||label==9){const int64_t step=options_.window_enabled?(areas_?areas_->state().history_page_size:2):1;move(label==8?-step:step);return true;}
 if(label==10){move(-int64_t(layout_.first));return true;}if(label==11){move(int64_t(layout_.last_start)-int64_t(layout_.first));return true;}if(label==12){rebuild();return true;}
 if(label>=10000&&size_t(label-10000)<pages_.size()){
  const auto page=size_t(label-10000);if(pages_[page].voice.empty())return true;if(voice_active_&&hooks_.stop_voice)hooks_.stop_voice(100);voice_active_=false;
  if(hooks_.stop_other_voices)hooks_.stop_other_voices(100);if(!hooks_.play_voice)throw std::runtime_error("history voice playback provider missing");voice_active_=hooks_.play_voice(pages_[page].voice,options_.sound_number);return true;
 }return false;
}
bool HistoryView::click(int32_t x,int32_t y){for(auto it=layout_.regions.rbegin();it!=layout_.regions.rend();++it)if(!it->disabled&&contains(it->bounds,x,y))return it->action==13?drag_vertical(y):action(it->action);return false;}
bool HistoryView::drag_vertical(int32_t y){
 if(!layout_.open)return false;auto it=std::find_if(layout_.regions.begin(),layout_.regions.end(),[](const auto& region){return region.action==13;});if(it==layout_.regions.end())return false;
 int32_t thumb=0;if(it->sprites&&hooks_.icon_extent){const auto& s=*it->sprites;const auto group=s.normal_frame>=0?s.normal_frame:s.hover_frame;if(s.source>=0&&group>=0)thumb=std::max(0,hooks_.icon_extent(s.source,group).height);}
 const auto half=thumb/2;const auto lo=int64_t(it->bounds.top)+half,hi=int64_t(it->bounds.bottom)-half;if(hi<=lo)return true;const auto clamped=std::max(lo,std::min(hi,int64_t(y)));const auto index=int64_t(double(clamped-lo)/double(hi-lo)*double(layout_.last_start));layout_.first=size_t(std::max<int64_t>(0,std::min<int64_t>(layout_.last_start,index)));rebuild();return true;
}
void HistoryView::close(){if(voice_active_&&!options_.voice_continue&&hooks_.stop_voice)hooks_.stop_voice(100);voice_active_=false;layout_.open=false;layout_.glyphs.clear();layout_.icons.clear();layout_.regions.clear();layout_.pages.clear();}
bool HistoryView::take_advance_request(){const auto value=advance_;advance_=false;return value;}
}

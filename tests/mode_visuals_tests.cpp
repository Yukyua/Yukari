#include "yukari/mode_visuals.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
using namespace yukari;
void require(bool v,const char* text){if(!v)throw std::runtime_error(text);}
int main(){try{
 std::map<int,std::string> images={{7,"normal"},{8,"other"}};std::vector<std::string> reloads;unsigned created=0,removed=0;std::array<bool,4> live{};
 ModeVisualHooks hooks;hooks.layer_resource=[&](int id)->std::optional<std::string>{auto found=images.find(id);if(found==images.end())return std::nullopt;return found->second;};
 hooks.reload_image=[&](int id,const std::string& image,const std::string& mask){require(mask.empty(),"swap did not clear external mask");images.at(id)=image;reloads.push_back(image);};
 hooks.create_icon=[&](ModeIndicator mode,const ModeIconRecord& record){++created;require(record.parent==120&&record.source==121,"mode icon did not preserve source/parent mapping");live[unsigned(mode)]=true;return true;};
 hooks.remove_icon=[&](ModeIndicator mode,const ModeIconRecord&){++removed;live[unsigned(mode)]=false;};
 ModeVisuals model(hooks);model.set_graphic(ModeGraphic::Auto,7,"automatic");model.update(1,0,0);require(images[7]=="automatic"&&model.state().graphics[0].backup=="normal"&&reloads.size()==1,"auto swap did not capture and replace original resource");
 model.update(1,0,0);require(reloads.size()==1,"auto image reloaded repeatedly despite latch");
 images[7]="changed while active";model.update(0,0,0);require(images[7]=="normal"&&!model.state().graphics[0].applied,"auto stop did not restore captured original resource");
 model.update(1,0,0);model.disable_graphic(ModeGraphic::Auto);model.update(0,0,0);require(images[7]=="automatic"&&model.state().graphics[0].applied,"disabled registration incorrectly restored an applied swap");
 model.set_graphic(ModeGraphic::Auto,8,"next");model.update(0,0,0);require(images[8]=="normal","re-registration did not retain native previous backup/latch");
 model.set_graphic(ModeGraphic::Skip,99,"skip");model.update(0,1,0);require(!model.state().graphics[1].applied,"missing layer falsely latched a swap");images[99]="late";model.update(0,0,1);require(images[99]=="skip"&&model.state().graphics[1].backup=="late","combined skip state did not activate on full skip");images.erase(99);model.update(0,0,0);require(model.state().graphics[1].applied,"missing restore destination cleared its pending latch");images[99]="returned";model.update(0,0,0);require(images[99]=="late","deferred layer restore lost its original backup");
 for(unsigned n=0;n<4;++n)model.set_icon(ModeIndicator(n),{120,846,0,121,int(n)});
 model.update(1,1,0);model.update_already(true);require(created==3&&model.icon_present(ModeIndicator::Auto)&&model.icon_present(ModeIndicator::AlreadySkip)&&model.icon_present(ModeIndicator::Already),"mode/read icon creation policy differs");
 model.update(1,1,0);model.update_already(true);require(created==3,"existing indicator was duplicated");model.update(0,0,1);model.update_already(false);require(created==4&&removed==3&&model.icon_present(ModeIndicator::Skip),"mode/read icon removal policy differs");
 model.update(0,0,0);require(removed==4,"skip indicator was retained after stop");
 // Preserve raw native comparison, not implicit boolean conversion.
 model.update(2,0,0);require(model.icon_present(ModeIndicator::Auto),"raw mode flag did not create missing icon");model.update(2,0,0);require(!model.icon_present(ModeIndicator::Auto),"raw mode flag comparison was normalized");
 ModeVisuals nested(hooks);images[7]="base";nested.set_graphic(ModeGraphic::Auto,7,"auto");nested.set_graphic(ModeGraphic::Skip,7,"skip");nested.update(1,1,0);require(images[7]=="skip"&&nested.state().graphics[1].backup=="auto","same-layer swaps did not execute auto before skip");nested.update(0,0,0);require(images[7]=="auto","same-layer native restore order was flattened into one base image");
 const auto saved=model.state();ModeVisuals restored(hooks);restored.restore(saved);require(restored.state().graphics[0].backup==saved.graphics[0].backup&&restored.state().icons[2].group==2,"mode model state restore lost registration fields");restored.clear_icon_instances();for(unsigned n=0;n<4;++n)require(!restored.icon_present(ModeIndicator(n)),"scene teardown retained stale icon pointer state");
 TextFlow flow;flow.left=100;flow.top=200;flow.right=900;flow.bottom=600;flow.x=350;flow.y=450;
 require(key_wait_icon_position(0,5,6,flow)==std::make_pair(870,570),"default icon corner differs");require(key_wait_icon_position(1,5,6,flow)==std::make_pair(5,6),"absolute icon offset differs");require(key_wait_icon_position(2,5,6,flow)==std::make_pair(105,206),"top-left icon offset differs");require(key_wait_icon_position(3,5,6,flow)==std::make_pair(905,606),"bottom-right icon offset differs");require(key_wait_icon_position(4,5,6,flow)==std::make_pair(355,456),"cursor-relative icon offset differs");require(key_wait_icon_position(9,5,6,flow,77,88)==std::make_pair(77,88),"out-of-range icon mode lost native fallback");
 std::cout<<"native mode swaps, icon consumers and KeyWait geometry tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#include "yukari/save_preview.hpp"
#include <iostream>
#include <stdexcept>
using namespace yukari;
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(){try{
 SavePreviewMetadata metadata;metadata.exists=true;metadata.thumbnail={1,2,3};metadata.comment1="first";metadata.comment2="second";metadata.date={2024,12,31,7,9,8,true};
 auto preview=save_preview_layout({},metadata,20,30);require(preview.success&&preview.parent_x==20&&preview.parent_y==30&&preview.thumbnail&&preview.thumbnail->x==5&&preview.thumbnail->y==5&&preview.thumbnail->width==80&&preview.thumbnail->height==60,"native default thumbnail layout differs");
 require(preview.text.size()==3&&preview.text[0].text=="2024/12/31 07:09"&&preview.text[0].font==9&&preview.text[0].effect==2&&preview.text[0].color==0x303030&&preview.text[1].y==25&&preview.text[2].y==45,"native default text layout/date differs");
 SettingsServices settings;auto call=[&](const std::string& name,std::vector<Value> args){require(bool(settings.call(name,args)),"unhandled save-preview setting");};
 call("SaveDataDisplaySetToFont",{int32_t(8),int32_t(2),int32_t(0xffffff)});
 call("SaveDataDisplaySetToGraphic",{int32_t(9),int32_t(9),int32_t(126),int32_t(70)});
 call("SaveDataDisplaySetToDate",{int32_t(175),int32_t(8),std::string("%4.4d/%2.2d/%2.2d %2.2d:%2.2d"),int32_t(1),int32_t(3),int32_t(4),int32_t(12),int32_t(13)});
 call("SaveDataDisplaySetToComment1",{int32_t(175),int32_t(46)});call("SaveDataDisplaySetToComment2",{int32_t(175),int32_t(66)});
 preview=save_preview_layout(settings.state(),metadata,200,300);
 require(preview.thumbnail->x==9&&preview.thumbnail->width==126&&preview.thumbnail->height==70&&preview.text[0].x==175&&preview.text[0].y==8&&preview.text[1].y==46&&preview.text[2].y==66,"game OptionSystem coordinates were not consumed");
 require(preview.text[0].font==8&&preview.text[0].effect==2&&preview.text[0].color==0xffffff,"font index/effect/color confused with font size");
 call("SaveDataDisplaySetToComment1Font",{int32_t(3),int32_t(-1),int32_t(-1)});call("SaveDataDisplaySetToComment2Font",{int32_t(-1),int32_t(4),int32_t(0x00ff00)});
 call("SaveDataDisplaySetToComment2",{int32_t(10),int32_t(20),int32_t(180),int32_t(80)});preview=save_preview_layout(settings.state(),metadata,200,300);
 require(preview.text[1].font==3&&preview.text[1].effect==2&&preview.text[1].color==0xffffff&&preview.text[2].font==8&&preview.text[2].effect==4&&preview.text[2].color==0xff00,"per-comment font inheritance differs");
 require(preview.text[2].rect&&preview.text[2].rect->left==10&&preview.text[2].rect->bottom==80,"comment2 bounded native-flow rectangle was discarded");
 call("SaveDataDisplaySetToDate",{int32_t(1),int32_t(2),std::string("%02d %02d:%02d:%02d %%"),int32_t(2),int32_t(11),int32_t(13),int32_t(14)});metadata.date.hour=12;
 preview=save_preview_layout(settings.state(),metadata,0,0);require(preview.text[0].text=="24 00:09:08 %","native integer date fields/12-hour modulo differ");
 call("SaveDataDisplaySetToGraphic",{});call("SaveDataDisplaySetToDate",{});call("SaveDataDisplaySetToComment2",{});metadata.comment1="abc日本";
 preview=save_preview_layout(settings.state(),metadata,0,0,{3,"CP932"});require(!preview.thumbnail&&preview.text.size()==1&&preview.text[0].text=="日本","disabled elements or CP932 byte-prefix handling differ");
 unsigned draws=0;SavePreviewDrawHooks hooks;hooks.text=[&](const SavePreviewText& text,int x,int y){require(text.text=="日本"&&x==0&&y==0,"draw hook lost resolved layout");++draws;};draw_save_preview(preview,hooks);require(draws==1,"save preview did not dispatch text draw");
 bool rejected=false;try{draw_save_preview(preview,{});}catch(const std::runtime_error&){rejected=true;}require(rejected,"missing draw hook silently claimed preview rendering");
 call("SaveDataDisplaySetToDate",{int32_t(0),int32_t(0),std::string("%n"),int32_t(1)});rejected=false;try{save_preview_layout(settings.state(),metadata,0,0);}catch(const std::runtime_error&){rejected=true;}require(rejected,"non-integer native date format accepted");
 metadata.exists=false;require(!save_preview_layout(settings.state(),metadata,0,0).success,"absent slot preview reported success");
 metadata.exists=true;metadata.thumbnail.clear();metadata.date.valid=false;auto no_thumbnail=save_preview_layout({},metadata,0,0);require(no_thumbnail.success&&!no_thumbnail.thumbnail&&no_thumbnail.text.size()==2,"empty native DIB should preserve valid comments");
 std::cout<<"save preview native defaults, settings, dates and comment layout tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

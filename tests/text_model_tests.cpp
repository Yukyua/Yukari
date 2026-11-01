#include "yukari/text_model.hpp"
#include <iostream>
#include <stdexcept>
using namespace yukari;
void check(bool c,const char* s){if(!c)throw std::runtime_error(s);}
template<class F>void reject(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("expected explicit rejection");}
int main(){try{
 TextModel m;auto& l=m.layer(2);l.flow.left=10;l.flow.top=20;l.flow.right=110;l.flow.bottom=100;
 m.call("DefaultFontSet",{int32_t(0),std::string("font"),int32_t(20),int32_t(400),int32_t(20),int32_t(30)});
 m.reset_cursor(2);m.glyph(2,"A",false,9,20);check(l.flow.x==20&&l.flow.anchor_x==20,"half-cell advance");
 m.glyph(2,std::string("\x82\xa0",2),true,20,20);check(l.flow.x==40,"DBCS advance");
 m.line_feed(2);check(l.flow.y==50&&l.records.back().kind==7,"native LF marker");
 m.glyph(2,"B",false,8,20);check(m.str_in(2)==std::string("A\xe3\x81\x82" "B"),"StrIn excludes LF markers");
 l.records.push_back({4,0,0,1,1,0,12,{}});l.records.push_back({5,0,0,1,1,0,3,{}});
 check(m.str_in(2).find("@o(12)@g(3)")!=std::string::npos,"StrIn image references");
 l.flow.overflow=true;m.reset_cursor(2);check(l.flow.overflow,"cursor reset must preserve overflow");m.clear(2);check(!l.flow.overflow&&l.records.empty(),"PF clear records");
 m.call("NameWindowSet",{int32_t(2),int32_t(1),int32_t(2),int32_t(81),int32_t(92)});check(m.name_window()==2&&l.flow.x==1&&l.flow.y==2,"name window bounds/reset");
 check(std::get<int32_t>(*m.call("RubyFontSet",{int32_t(3)}))==3,"RubyFontSet result");
 reject([&]{m.call("TextBoxSet",{int32_t(0),int32_t(0),int32_t(10),int32_t(10)});});
 reject([&]{m.call("OriginalFontSet",{std::string("missing")});});
 unsigned creates=0,deletes=0;std::string edited;TextModelHooks hooks;
 hooks.create_box=[&](const TextBoxSpec& s){check(s.value_24==10&&s.style==7,"edit-control native fields");++creates;};hooks.delete_box=[&]{++deletes;};hooks.get_box=[&]{return edited;};hooks.set_box=[&](const std::string& s){edited=s;};
 TextModel ui(hooks);for(int i=0;i<2;++i)ui.call("TextBoxSet",{int32_t(1),int32_t(2),int32_t(10),int32_t(20),int32_t(7)});
 ui.call("TextBoxStringSet",{std::string("edited")});check(std::get<std::string>(*ui.call("TextBoxStringGet",{}))=="edited","actual edit-control hook roundtrip");
 ui.call("TextBoxDelete",{});ui.call("TextBoxDelete",{});check(creates==2&&deletes==2,"control replacement/delete lifecycle");check(!ui.call("unknown",{}),"unknown route must remain unknown");
 TextModelHooks layout_hooks;layout_hooks.dbcs_lead=[](uint8_t b){return b>=0x81&&b<=0x9f;};
 layout_hooks.measure=[](int32_t font,const std::string& text){return std::make_pair(int32_t(text.size()*(font==1?4:10)),font==1?8:20);};
 layout_hooks.original_font_size=[](int32_t){return std::make_pair(12,12);};
 layout_hooks.graphic_font_size=[](int32_t){return std::make_pair(30,40);};
 TextModel layout(layout_hooks);auto& ll=layout.layer(1);ll.flow.right=200;ll.flow.bottom=200;
 layout.call("DefaultFontSet",{int32_t(0),std::string("main"),int32_t(20),int32_t(400)});
 layout.call("DefaultFontSet",{int32_t(1),std::string("ruby"),int32_t(8),int32_t(400)});layout.call("RubyFontSet",{int32_t(1)});
 check(layout.output_step(1,"Arest")==1&&ll.flow.x==10,"output one encoded glyph");
 const auto x=ll.flow.x,y=ll.flow.y,anchor=ll.flow.anchor_x;
 check(layout.output_step(1,"@r(1,abc)rest")==9,"ruby consumed bytes");
 check(ll.flow.x==x&&ll.flow.y==y&&ll.flow.anchor_x==anchor,"ruby restores cursor/anchors");
 check(ll.records[1].font==1&&ll.records[1].x==-6&&ll.records[1].y==-8,"ruby center placement");
 check(layout.output_step(1,"@gf(0x12)rest")==9&&ll.flow.x==40&&ll.records.back().kind==5,"graphic-font max-cell advance");
 ll.flow.right=60;check(layout.output_step(1,"@m(3)abc")==5&&ll.flow.y==20&&ll.flow.x==0,"keep-together lookahead LF");
 reject([&]{layout.output_step(1,"@r(bad)");});
 check(layout.measure(0,"@r(1,abc)A").first==20,"native measure retains closing parenthesis");
 ll.flow.right=200;ll.flow.bottom=200;layout.clear(1);layout.call("NameWindowSet",{int32_t(1)});
 layout.name_out("AB");check(ll.records[0].x==90&&ll.records[1].x==100,"NameOut centers first instruction");
 layout.clear(1);check(layout.output_step(1,"@of(7)")==6&&ll.records.back().kind==4&&ll.flow.x==20,"original-font layout advance");
 check(layout.str_in(1)=="@o(7)","original-font StrIn reconstruction");
 ll.style_token=0x123456789abcdef0ULL;layout.clear(1);layout.output_step(1,"X");layout.output_step(1,"@r(1,ab)");
 for(const auto& rec:ll.records)check(rec.style_token==ll.style_token,"glyph/ruby style inheritance");
 const auto snap=layout.snapshot();TextModel restored(layout_hooks);restored.restore(snap);check(restored.snapshot()==snap,"text snapshot roundtrip");
 check(restored.layer(1).records[1].style_token==ll.style_token&&restored.layer(1).flow.x==ll.flow.x,"text snapshot record/cursor/style");
 auto broken=snap;broken.back()^=1;reject([&]{restored.restore(broken);});check(restored.snapshot()==snap,"failed text restore must be atomic");
 broken=snap;broken.pop_back();reject([&]{restored.restore(broken);});
 layout.clear(1);ll.flow.right=200;ll.flow.bottom=200;
 layout.rules().kinsoku_pairs=std::string("\x81\x41",2);layout.rules().suppress_after_rollback=std::string("\x81\x42",2);
 ll.flow.x=0;ll.flow.y=20;ll.flow.anchor_x=20;ll.flow.anchor_y=0;
 check(layout.output_step(1,std::string("\x81\x41\x81\x42",4))==4,"rollback consumes trailing suppressed space");
 check(ll.records.back().x==20&&ll.records.back().y==0,"kinsoku restores pre-wrap anchor");
 layout.clear(1);layout.rules().hanging_prefixes={std::string("\x81\x43",2)};
 layout.output_step(1,std::string("\x81\x43X",3));check(ll.hanging_indent==20,"opening prefix establishes hanging indent");layout.line_feed(1);layout.output_step(1,"A");
 check(ll.records[ll.records.size()-2].encoded==std::string("\x81\x40",2)&&ll.records.back().x==20,"continuation inserts native fullwidth indent space");
 layout.clear(1);layout.rules().hanging_prefixes.clear();layout.rules().protect_word_runs=true;layout.rules().word_run_pairs=std::string("\x81\x44",2);ll.flow.x=40;ll.flow.anchor_x=40;ll.flow.right=100;
 layout.output_step(1,std::string("\x81\x43\x81\x44\x81\x44",6));check(ll.records.back().x==0&&ll.records.back().y==20,"word-run lookahead forces native LF");
 layout.clear(1);ll.flow.overflow=true;check(layout.output_step(1,"Z")==0&&ll.output_prepared,"overflow suspends before pending glyph");
 TextModel pending(layout_hooks);pending.restore(layout.snapshot());check(pending.layer(1).pending_output=="Z","snapshot retains pending output continuation");pending.clear(1);check(pending.output_step(1,"Z")==1&&pending.layer(1).records.back().encoded=="Z","PF resumes exact pending token");
 bool live=false,fail_create=false;TextBoxSpec live_spec;std::string live_value;unsigned touched=0;TextModelHooks box_hooks;
 box_hooks.create_box=[&](const TextBoxSpec& spec){++touched;if(fail_create){fail_create=false;throw std::runtime_error("injected native allocation failure");}live=true;live_spec=spec;live_value.clear();};box_hooks.delete_box=[&]{++touched;live=false;live_value.clear();};box_hooks.set_box=[&](const std::string& value){check(live,"set on dead provider");live_value=value;};box_hooks.get_box=[&]{check(live,"read on dead provider");return live_value;};
 TextModel boxed(box_hooks);boxed.call("TextBoxSet",{int32_t(10),int32_t(20),int32_t(200),int32_t(30),int32_t(0x123456)});live_value="typed in real UI";const auto box_snapshot=boxed.snapshot();boxed.call("TextBoxStringSet",{std::string("changed")});boxed.restore(box_snapshot);
 check(live&&live_spec.value_28==10&&live_spec.value_2c==20&&live_spec.value_24==200&&live_spec.font_height==30&&live_spec.style==0x123456&&live_value=="typed in real UI","TextBox snapshot omitted real spec/value");
 check(boxed.snapshot()==box_snapshot,"TextBox snapshot roundtrip");auto corrupt_box=box_snapshot;corrupt_box.back()^=1;auto before_touched=touched;reject([&]{boxed.restore(corrupt_box);});check(touched==before_touched,"invalid snapshot touched native provider before validation");
 boxed.call("TextBoxStringSet",{std::string("preserve old UI")});const auto stable_box=boxed.snapshot();fail_create=true;reject([&]{boxed.restore(box_snapshot);});check(live&&live_value=="preserve old UI"&&boxed.snapshot()==stable_box,"provider failure committed candidate model or lost previous UI");
 live_value="Unicode \xf0\x9f\x8c\xb8";auto unicode_box=boxed.snapshot();boxed.restore(unicode_box);check(live_value=="Unicode \xf0\x9f\x8c\xb8","own UTF-8 TextBox snapshot incorrectly forced source encoding");
 TextModel empty_model(box_hooks);auto legacy=empty_model.snapshot();auto read32=[&](size_t at){uint32_t v=0;for(unsigned k=0;k<4;++k)v|=uint32_t(legacy.at(at+k))<<(k*8);return v;};const auto extra=size_t(8+4+read32(8)+12);const auto value_size=read32(extra+20);legacy.erase(legacy.begin()+extra,legacy.begin()+extra+24+value_size);legacy[4]=3;
 uint64_t legacy_hash=14695981039346656037ULL;for(size_t i=0;i<legacy.size()-8;++i)legacy_hash=(legacy_hash^legacy[i])*1099511628211ULL;for(unsigned k=0;k<8;++k)legacy[legacy.size()-8+k]=uint8_t(legacy_hash>>(8*k));
 boxed.restore(legacy);check(!live,"legacy v3 without box did not destroy previous native control");check(boxed.snapshot()==empty_model.snapshot(),"legacy v3 model compatibility failed");
 std::cout<<"PASS native text layout/record and UI-provider contracts\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

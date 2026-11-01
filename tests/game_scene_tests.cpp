#include "yukari/game_scene.hpp"
#include <png.h>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
using yukari::Bytes;
void require(bool c,const char* m){if(!c)throw std::runtime_error(m);}
void put(Bytes& b,size_t at,uint32_t v){for(unsigned i=0;i<4;++i)b.at(at+i)=uint8_t(v>>(8*i));}
Bytes png(int w,int h,uint8_t r,uint8_t g,uint8_t b){png_image p{};p.version=PNG_IMAGE_VERSION;p.width=w;p.height=h;p.format=PNG_FORMAT_RGBA;Bytes rgba(size_t(w)*h*4);for(size_t i=0;i<rgba.size();i+=4){rgba[i]=r;rgba[i+1]=g;rgba[i+2]=b;rgba[i+3]=255;}png_alloc_size_t count=0;require(png_image_write_to_memory(&p,nullptr,&count,0,rgba.data(),0,nullptr),"PNG size");Bytes out(count);require(png_image_write_to_memory(&p,out.data(),&count,0,rgba.data(),0,nullptr),"PNG write");out.resize(count);return out;}
Bytes sprite(unsigned groups,uint8_t r,uint8_t g,uint8_t b){auto color=png(2,2,r,g,b);Bytes out(64);std::memcpy(out.data(),"YKG000\0\0",8);put(out,8,64);put(out,0x28,64);put(out,0x2c,color.size());put(out,0x38,64+color.size());put(out,0x3c,groups*64);out.insert(out.end(),color.begin(),color.end());for(unsigned i=0;i<groups;++i){const size_t at=out.size();out.resize(at+64);put(out,at+8,2);put(out,at+12,2);put(out,at+16,100);put(out,at+20,1);}return out;}
Bytes wav(){Bytes b(44+2205*2);std::memcpy(b.data(),"RIFF",4);put(b,4,b.size()-8);std::memcpy(b.data()+8,"WAVEfmt ",8);put(b,16,16);b[20]=1;b[22]=1;put(b,24,22050);put(b,28,44100);b[32]=2;b[34]=16;std::memcpy(b.data()+36,"data",4);put(b,40,b.size()-44);return b;}
Bytes pixels(yukari::Scene& s){Bytes out(size_t(s.width())*s.height()*4);require(SDL_RenderReadPixels(s.renderer(),nullptr,SDL_PIXELFORMAT_RGBA32,out.data(),s.width()*4)==0,"read pixels");return out;}
int main(){try{
 SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_AUDIODRIVER","dummy",1);yukari::Scene scene(1280,720,"game fixture");yukari::Audio audio(false);
 std::map<std::string,Bytes> files;files["system\\black.png"]=png(1280,720,0,0,0);files["system\\null.png"]=png(1,1,0,0,0);const std::string dir="system\\game\\";
 files[dir+"minigame_bg01.png"]=png(1280,720,0,0,255);files[dir+"minigame_wait.png"]=png(1280,720,200,200,200);files[dir+"minigame_finish_good.png"]=png(1280,720,0,255,0);files[dir+"minigame_finish_bad.png"]=png(1280,720,255,0,0);
 for(const char* name:{"red","yellow","white","green","blue"})files[dir+"minigame_button_"+name+".ykg"]=sprite(1,255,255,255);
 for(const char* name:{"otome","natumi","yuduki","megumi","saki"})files[dir+"minigame_"+name+".ykg"]=sprite(6,255,0,255);
 files[dir+"minigame_effect.ykg"]=sprite(2,255,255,0);files["song.wav"]=wav();std::string chart="SoundFile,song.wav\nSuccessBorder,1\nMusicTotalTime,1000\n3,500,0\n";files["chart"]={chart.begin(),chart.end()};
 auto reader=[&](const std::string& name){auto found=files.find(name);if(found==files.end())throw std::runtime_error("missing fixture: "+name);return found->second;};
 yukari::MusicGameScene game(scene,audio,reader);game.set_transition_sampling({1000,2,2});game.begin("chart");
 for(uint64_t time:{1000,2000,3000,3300})game.tick(time);require(game.phase()==yukari::MusicGameScene::Phase::Waiting,"game did not reach confirmation");game.render();require(pixels(scene)[0]>=98&&pixels(scene)[0]<=100,"wait prompt not rendered");
 game.confirm();game.tick(3301);game.tick(3601);require(game.phase()==yukari::MusicGameScene::Phase::Playing&&audio.playing(1),"music did not start after wait dismissal");game.tick(4100);game.render();auto image=pixels(scene);require(image[(536*1280+607)*4]==255&&image[(11*1280+9)*4]==255,"note/character geometry not rendered");
 game.tick(4101,{false,false,true,false,false});require(game.result()==1,"scene did not forward judgment keys");game.render();image=pixels(scene);require(image[(536*1280+607)*4]==0&&image[(491*1280+558)*4]==255,"hit did not hide note/show effect");
 game.tick(4602);require(game.phase()==yukari::MusicGameScene::Phase::GoodFade,"successful result missing");game.tick(5602);game.render();require(pixels(scene)[1]==255,"good result not displayed");game.tick(8602);game.tick(9602);require(game.finished()&&game.result()==1,"scene did not finish after result hold/fade");
 game.begin("chart",{},10000);for(uint64_t time:{11000,12000,13000,13300})game.tick(time);game.confirm();game.tick(13301);game.tick(13601);game.tick(14602);
 require(game.phase()==yukari::MusicGameScene::Phase::BadBounce&&game.result()==0,"missed chart did not select bad result");game.tick(14902);game.render();require(pixels(scene)[0]>=228&&pixels(scene)[0]<=230,"bad result did not reach y0");game.tick(15002);game.render();require(pixels(scene)[(719*1280)*4+2]==255,"bad result first bounce did not expose bottom strip");game.tick(15202);require(game.phase()==yukari::MusicGameScene::Phase::ResultHold,"bad bounce did not finish");game.tick(18202);game.tick(19202);require(game.finished(),"bad result did not finish");
 game.begin("chart",{},20000);for(uint64_t time:{21000,22000,23000,23300})game.tick(time);game.confirm();game.tick(23301);game.tick(23601);game.tick(23602,{},true);require(game.phase()==yukari::MusicGameScene::Phase::BadBounce,"abort skipped native result branch");game.tick(24202,{},true);game.tick(27202,{},true);game.tick(28202,{},true);require(game.finished(),"abort result/hold/fade did not complete");
 files.erase(dir+"minigame_effect.ykg");bool missing=false;try{game.begin("chart",{},30000);}catch(const std::runtime_error&){missing=true;}require(missing,"missing game resource was silently accepted");
 std::cout<<"synthetic GamePart scene, confirmation, judgment and fade tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

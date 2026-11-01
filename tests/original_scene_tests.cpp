#include "yukari/original_scene.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace yukari;
namespace {
void put(Bytes& b,size_t p,uint32_t n){for(unsigned i=0;i<4;++i)b.at(p+i)=uint8_t(n>>(i*8));}
void rejects(const std::function<void()>& f){bool threw=false;try{f();}catch(const std::runtime_error&){threw=true;}assert(threw);}
Bytes fixture(){
    Bytes b(0x600);b[0]='Y';b[1]='u';b[2]='k';b[3]='a';put(b,0x20,0x48);put(b,0x24,0x5b8);put(b,0x48,0x160);
    put(b,0x48+0x24,0x200);put(b,0x48+0x28,1);put(b,0x200,0x140);put(b,0x204,0x400);put(b,0x208,13);put(b,0x220,uint32_t(-10));b[0x228]=1;
    put(b,0x25c,0x500);put(b,0x260,6);for(unsigned i=0;i<5;++i)b[0x500+i]="a.ykg"[i];
    put(b,0x26c,0x340);put(b,0x270,1);put(b,0x340,0x7c);put(b,0x348,1);b[0x34c]=1;put(b,0x350,30);put(b,0x374,31);put(b,0x388,2);put(b,0x39c,9);b[0x3a0]=1;
    put(b,0x3b0,0x520);put(b,0x3b4,32);put(b,0x520+8,124);put(b,0x520+12,124);put(b,0x520+20,1);
    put(b,0x48+0x1c,0x400);put(b,0x48+0x20,1);put(b,0x400,0x20);put(b,0x404,0x430);put(b,0x408,1);b[0x40c]=1;b[0x40d]=1;put(b,0x410,0x508);put(b,0x414,6);put(b,0x418,800);for(unsigned i=0;i<5;++i)b[0x508+i]="a.ogg"[i];
    return b;
}
}
int main(){
    {auto data=fixture();auto state=decode_original_scene(data);OriginalSprite glyph;glyph.kind=3;glyph.font=2;glyph.font_effect=5;glyph.shadow_color=0x123456;glyph.border_color=0xabcdef;state.layers[0].sprites.push_back(glyph);encode_original_scene(data,state);auto restored=decode_original_scene(data);const auto& g=restored.layers[0].sprites.back();assert(g.font_effect==5&&g.shadow_color==0x123456&&g.border_color==0xabcdef);}
    auto b=fixture();auto scene=decode_original_scene(b);
    assert(scene.layers.size()==1&&scene.sounds.size()==1);const auto& l=scene.layers[0];assert(l.id==13&&l.x==-10&&l.visible&&l.resource=="a.ykg");
    assert(l.sprites.size()==1&&l.sprites[0].source_layer==31&&l.sprites[0].group==2&&l.sprites[0].slot==9);assert(l.sprites[0].animation.size()==1&&l.sprites[0].animation[0].w==124);
    assert(scene.sounds[0].resource=="a.ogg"&&scene.sounds[0].playing&&scene.sounds[0].loop&&scene.sounds[0].volume==800);
    scene.layers[0].native_header[0x134]=42;
    scene.layers[0].resource="画像.ykg";
    scene.layers[0].sprites[0].encoded_character=0xa082;
    const auto prefix=b;encode_original_scene(b,scene);auto roundtrip=decode_original_scene(b);
    assert(roundtrip.layers[0].resource=="画像.ykg"&&roundtrip.layers[0].native_header[0x134]==42);
    assert(roundtrip.layers[0].sprites[0].encoded_character==0xa082&&roundtrip.layers[0].sprites[0].animation[0].w==124);
    assert(roundtrip.sounds[0].resource=="a.ogg"&&roundtrip.sounds[0].volume==800);
    const auto before=b;scene.layers[0].resource="😀.ykg";rejects([&]{encode_original_scene(b,scene);});assert(b==before);
    scene.layers[0].resource="画像.ykg";
    Bytes new_container(0x1a8);new_container[0]='Y';new_container[1]='u';new_container[2]='k';new_container[3]='a';put(new_container,0x20,0x48);put(new_container,0x48,0x160);
    OriginalScene fresh;OriginalLayer fresh_layer;fresh_layer.id=8;fresh_layer.resource="new.ykg";fresh_layer.visible=true;fresh.layers.push_back(fresh_layer);encode_original_scene(new_container,fresh);assert(decode_original_scene(new_container).layers[0].resource=="new.ykg");
    b=prefix;
    Bytes nested(37,0xcc);nested.insert(nested.end(),b.begin(),b.end());assert(decode_original_scene(nested,37).layers[0].resource=="a.ykg");
    encode_original_scene(nested,scene,37);assert(decode_original_scene(nested,37).layers[0].resource=="画像.ykg");
    auto damaged=b;put(damaged,0x48+0x28,2);put(damaged,0x204,0x200);rejects([&]{decode_original_scene(damaged);});
    damaged=b;put(damaged,0x3b4,31);rejects([&]{decode_original_scene(damaged);});
    damaged=b;put(damaged,0x260,5);rejects([&]{decode_original_scene(damaged);});
    damaged=b;put(damaged,0x48+0x20,0xffffffff);rejects([&]{decode_original_scene(damaged);});
    damaged=b;damaged.resize(0x350);rejects([&]{decode_original_scene(damaged);});
    std::cout<<"original scene records and malformed descriptors passed\n";
}

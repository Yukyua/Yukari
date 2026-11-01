#include "yukari/midi.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace yukari;
Bytes midi(){return {'M','T','h','d',0,0,0,6,0,0,0,1,0,96,'M','T','r','k',0,0,0,22,0,0xff,0x51,3,7,0xa1,0x20,0,0xc0,0,0,0x90,60,100,96,0x80,60,0,0,0xff,0x2f,0};}
int main(){
 Midi player;assert(!player.loaded()&&!player.playing()&&player.position_ms()==0);player.stop();bool rejected=false;try{player.play();}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 const auto bytes=midi();player.load(bytes);assert(player.loaded()&&!player.playing()&&player.tracks()==1);assert(player.duration_ms()>=499&&player.duration_ms()<=501);assert(player.position_ms()==0);
 for(Bytes invalid:{Bytes(20,0),Bytes(bytes.begin(),bytes.end()-4)}){rejected=false;try{player.load(invalid);}catch(const std::runtime_error&){rejected=true;}assert(rejected&&player.loaded()&&player.tracks()==1&&player.duration_ms()==500);}
 player.stop();assert(!player.loaded()&&!player.playing()&&player.tracks()==0);player.load(bytes);assert(player.duration_ms()==500);
 std::cout<<"MIDI parse, tempo duration, replacement and silent stop passed (no output device opened)\n";
}

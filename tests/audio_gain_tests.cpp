#include "yukari/audio_gain.hpp"
#include <cassert>
#include <iostream>
using namespace yukari;
int main(){
 for(bool type:{false,true}){assert(native_sound_attenuation(0,type)==-10000&&native_sound_attenuation(1000,type)==0);assert(native_sound_pcm_gain(0,type)==0&&native_sound_pcm_gain(1000,type)==1);assert(!native_sound_pcm_gain(-1,type)&&!native_sound_pcm_gain(2000,type));}
 assert(native_sound_attenuation(500,false)==-5000);assert(native_sound_attenuation(500,true)==-752);assert(native_sound_attenuation(100,true)==-2500);assert(native_sound_attenuation(1,true)==-7500);
 assert(std::abs(*native_sound_pcm_gain(500,true)-.4207266283844441)<1e-12);assert(std::abs(*native_sound_pcm_gain(500,false)-.0031622776601683794)<1e-12);
 for(bool type:{false,true}){double previous=0;for(int v=0;v<=1000;++v){auto gain=native_sound_pcm_gain(v,type);assert(gain&&*gain>=previous&&*gain<=1);previous=*gain;}}
 assert(!native_sound_attenuation_valid(INT32_MIN)&&!native_sound_attenuation_valid(1));assert(direct_sound_pcm_gain(-10000)==0);
 assert(native_movie_attenuation(500,true)==-1003);assert(native_movie_attenuation(0,true)==-10000);assert(native_movie_attenuation(1000,true)==0);
 std::cout<<"original linear-decibel/logarithmic volume curves, integer attenuation, silence and invalid DirectSound ranges passed\n";
}

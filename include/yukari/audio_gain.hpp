#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
namespace yukari {
// Original 4608f0: input is the already combined integer sound volume /1000.
// Return the exact LONG supplied to DirectSound, including invalid values.
inline int32_t native_sound_attenuation(int32_t volume,bool logarithmic){
 if(!volume)return -10000;
 const double ratio=double(volume)/1000;
 const double db=logarithmic?(std::log10(ratio*10000)*.25-1)*10000:(ratio-1)*10000;
 return !std::isfinite(db)||db<std::numeric_limits<int32_t>::min()||db>=2147483648.0?INT32_MIN:int32_t(db);
}
inline int32_t native_movie_attenuation(int32_t volume,bool logarithmic){
 if(!volume)return -10000;
 const double db=logarithmic?std::log10(double(volume))/3*10000-10000:(double(volume)/1000-1)*10000;
 return !std::isfinite(db)||db<std::numeric_limits<int32_t>::min()||db>=2147483648.0?INT32_MIN:int32_t(db);
}
// DirectSound does not amplify; invalid LONGs leave the previous buffer volume
// in place. Callers must not clamp them to an invented successful new setting.
inline bool native_sound_attenuation_valid(int32_t attenuation){return attenuation>=-10000&&attenuation<=0;}
inline double direct_sound_pcm_gain(int32_t attenuation){
 if(!native_sound_attenuation_valid(attenuation))return 0;
 return attenuation==-10000?0:std::pow(10.0,double(attenuation)/2000);
}
inline std::optional<double> native_sound_pcm_gain(int32_t volume,bool logarithmic){
 const auto attenuation=native_sound_attenuation(volume,logarithmic);
 if(!native_sound_attenuation_valid(attenuation))return std::nullopt;
 return direct_sound_pcm_gain(attenuation);
}
}

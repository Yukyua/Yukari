#include "yukari/original_layer_state.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace yukari {
namespace {
int32_t integer(double value){return std::isfinite(value)&&value>=INT32_MIN&&value<2147483648.0?int32_t(value):INT32_MIN;}
int32_t signed_bits(uint32_t value){int32_t out;std::memcpy(&out,&value,4);return out;}
void valid(const Bytes& bytes){if(bytes.size()!=0x140||u32(bytes,0)!=0x140)throw std::runtime_error("unsupported original layer state header");}
}
int32_t original_saved_transmittance(double current,double velocity,double target){return integer(velocity==0?current:target);}
void OriginalLayerState::apply_motion(MotionModel& m)const{
    m.x=x;m.y=y;m.visible=visible!=0;m.scroll_mode=scroll_mode;m.scroll_x=scroll_x;m.scroll_y=scroll_y;
    m.scale_enabled=scale_enabled!=0;m.scale_scrolling=scale_scrolling!=0;m.scale_x=scale_x;m.scale_y=scale_y;
    m.scale_velocity_x=scale_velocity_x;m.scale_velocity_y=scale_velocity_y;m.scale_target_x=scale_target_x;m.scale_target_y=scale_target_y;
    m.angle=angle;m.angular_velocity=angular_velocity;m.rotate_mode=rotate_mode;m.rotate_target=rotate_target;
    m.raster_enabled=raster_enabled!=0;m.raster_target=raster_target;m.raster_frequency=raster_frequency;m.raster_period=raster_period;
    m.raster_phase=m.raster_remaining=m.raster_velocity=0;
}
MotionModel OriginalLayerState::motion()const{MotionModel m;apply_motion(m);return m;}
void OriginalLayerState::capture_motion(const MotionModel& m){
    x=integer(m.x);y=integer(m.y);visible=m.visible;scroll_mode=m.scroll_mode;scroll_x=m.scroll_x;scroll_y=m.scroll_y;
    scale_enabled=m.scale_enabled;scale_scrolling=m.scale_scrolling;scale_x=m.scale_x;scale_y=m.scale_y;
    scale_velocity_x=m.scale_velocity_x;scale_velocity_y=m.scale_velocity_y;scale_target_x=m.scale_target_x;scale_target_y=m.scale_target_y;
    angle=m.angle;angular_velocity=m.angular_velocity;rotate_mode=m.rotate_mode;rotate_target=m.rotate_target;
    raster_enabled=m.raster_enabled;raster_target=m.raster_target;raster_frequency=m.raster_frequency;raster_period=m.raster_period;
}
OriginalLayerState decode_original_layer_state(const Bytes& header){
    valid(header);OriginalLayerState out;
    auto word=[&](size_t at){return signed_bits(u32(header,at));};
    auto number=[&](size_t at){uint64_t bits=uint64_t(u32(header,at))|(uint64_t(u32(header,at+4))<<32);double value;std::memcpy(&value,&bits,8);return value;};
    out.image_type=word(0xc);out.x=word(0x20);out.y=word(0x24);out.visible=header[0x28];out.transmittance=word(0x74);
    out.scroll_mode=word(0x88);out.scroll_x=number(0x90);out.scroll_y=number(0x98);
    out.scale_enabled=header[0xa0];out.scale_x=number(0xa8);out.scale_y=number(0xb0);
    out.scale_center_x=word(0xb8);out.scale_center_y=word(0xbc);out.scale_scrolling=header[0xc0];
    out.scale_velocity_x=number(0xc8);out.scale_velocity_y=number(0xd0);out.scale_target_x=number(0xd8);out.scale_target_y=number(0xe0);
    out.angle=number(0xe8);out.rotate_center_x=word(0xf0);out.rotate_center_y=word(0xf4);out.rotate_mode=word(0xf8);out.angular_velocity=number(0x100);out.rotate_target=word(0x108);
    out.raster_enabled=header[0x115];out.raster_target=word(0x118);out.raster_frequency=word(0x11c);out.raster_period=word(0x120);
    out.add_color=u32(header,0x124);out.sub_color=u32(header,0x128);out.sepia=header[0x12c];out.monochrome=header[0x12d];out.negative=header[0x12e];return out;
}
Bytes encode_original_layer_state(const OriginalLayerState& s,const Bytes& preserved){
    Bytes out=preserved.empty()?Bytes(0x140):preserved;
    if(!preserved.empty())valid(preserved);
    auto word=[&](size_t at,uint32_t value){for(unsigned i=0;i<4;++i)out[at+i]=uint8_t(value>>(8*i));};
    auto number=[&](size_t at,double value){uint64_t bits;std::memcpy(&bits,&value,8);word(at,uint32_t(bits));word(at+4,uint32_t(bits>>32));};
    word(0,0x140);word(0xc,uint32_t(s.image_type));word(0x20,uint32_t(s.x));word(0x24,uint32_t(s.y));out[0x28]=s.visible;word(0x74,uint32_t(s.transmittance));
    word(0x88,uint32_t(s.scroll_mode));number(0x90,s.scroll_x);number(0x98,s.scroll_y);
    out[0xa0]=s.scale_enabled;number(0xa8,s.scale_x);number(0xb0,s.scale_y);word(0xb8,uint32_t(s.scale_center_x));word(0xbc,uint32_t(s.scale_center_y));out[0xc0]=s.scale_scrolling;
    number(0xc8,s.scale_velocity_x);number(0xd0,s.scale_velocity_y);number(0xd8,s.scale_target_x);number(0xe0,s.scale_target_y);
    number(0xe8,s.angle);word(0xf0,uint32_t(s.rotate_center_x));word(0xf4,uint32_t(s.rotate_center_y));word(0xf8,uint32_t(s.rotate_mode));number(0x100,s.angular_velocity);word(0x108,uint32_t(s.rotate_target));
    out[0x115]=s.raster_enabled;word(0x118,uint32_t(s.raster_target));word(0x11c,uint32_t(s.raster_frequency));word(0x120,uint32_t(s.raster_period));
    word(0x124,s.add_color);word(0x128,s.sub_color);out[0x12c]=s.sepia;out[0x12d]=s.monochrome;out[0x12e]=s.negative;return out;
}
}

#include "yukari/original_layer_state.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using yukari::Bytes;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void put(Bytes& b,size_t at,uint32_t value){for(unsigned i=0;i<4;++i)b.at(at+i)=uint8_t(value>>(8*i));}
double number(const Bytes& b,size_t at){uint64_t bits=yukari::u32(b,at)|(uint64_t(yukari::u32(b,at+4))<<32);double value;std::memcpy(&value,&bits,8);return value;}
int main(){try{
 Bytes header(0x140,0xa5);put(header,0,0x140);
 const auto unchanged=yukari::encode_original_layer_state(yukari::decode_original_layer_state(header),header);
 require(unchanged==header,"decode/encode changed unknown bytes, flag widths or native floating bits");
 yukari::OriginalLayerState state;state.image_type=5;state.x=-20;state.y=30;state.transmittance=477;state.visible=1;
 state.add_color=0x123456;state.sub_color=0xabcdef;state.sepia=2;state.monochrome=3;state.negative=4;
 state.scroll_mode=2;state.scroll_x=0.03125;state.scroll_y=-0.0625;state.scale_enabled=1;state.scale_scrolling=1;state.scale_x=1.25;state.scale_y=0.5;
 state.scale_center_x=-3;state.scale_center_y=75;state.scale_velocity_x=0.001;state.scale_velocity_y=-0.002;state.scale_target_x=2;state.scale_target_y=0.25;
 state.angle=1234.5;state.rotate_center_x=100;state.rotate_center_y=90;state.rotate_mode=2;state.rotate_target=1600;state.angular_velocity=0.125;
 state.raster_enabled=1;state.raster_target=42;state.raster_frequency=-4;state.raster_period=17;
 const auto encoded=yukari::encode_original_layer_state(state,header);
 require(yukari::u32(encoded,0xc)==5&&yukari::u32(encoded,0x20)==uint32_t(-20)&&yukari::u32(encoded,0x74)==477,"basic native header offsets differ");
 require(yukari::u32(encoded,0x88)==2&&number(encoded,0x90)==0.03125&&number(encoded,0x98)==-0.0625,"scroll native header offsets differ");
 require(encoded[0xa0]==1&&number(encoded,0xa8)==1.25&&number(encoded,0xb0)==0.5&&yukari::u32(encoded,0xb8)==uint32_t(-3)&&yukari::u32(encoded,0xbc)==75&&encoded[0xc0]==1,"scale header mapping differs");
 require(number(encoded,0xc8)==0.001&&number(encoded,0xd0)==-0.002&&number(encoded,0xd8)==2&&number(encoded,0xe0)==0.25,"scale tween header mapping differs");
 require(number(encoded,0xe8)==1234.5&&yukari::u32(encoded,0xf0)==100&&yukari::u32(encoded,0xf4)==90&&yukari::u32(encoded,0xf8)==2&&number(encoded,0x100)==0.125&&yukari::u32(encoded,0x108)==1600,"rotation header mapping differs");
 require(encoded[0x115]==1&&yukari::u32(encoded,0x118)==42&&yukari::u32(encoded,0x11c)==uint32_t(-4)&&yukari::u32(encoded,0x120)==17,"raster header mapping differs");
 require(yukari::u32(encoded,0x124)==0x123456&&yukari::u32(encoded,0x128)==0xabcdef&&encoded[0x12c]==2&&encoded[0x12d]==3&&encoded[0x12e]==4,"native color offsets/flag bytes differ");
 require(encoded[0x114]==0xa5&&encoded[0x116]==0xa5&&encoded[0x12f]==0xa5&&encoded[0x139]==0xa5&&encoded[4]==0xa5,"writer overwrote unrelated native fields");
 const auto decoded=yukari::decode_original_layer_state(encoded);require(yukari::encode_original_layer_state(decoded,encoded)==encoded,"native layer-state roundtrip differs");
 auto motion=decoded.motion();require(motion.x==-20&&motion.y==30&&motion.scale_x==1.25&&motion.angle==1234.5&&motion.raster_period==17,"native state was not applied to MotionModel");
 motion.left=-100;motion.right=500;motion.top=-80;motion.bottom=800;motion.raster_phase=3.5;motion.raster_velocity=7;motion.raster_remaining=55;decoded.apply_motion(motion);
 require(motion.left==-100&&motion.right==500&&motion.top==-80&&motion.bottom==800,"reader invented serialized scroll bounds");
 require(motion.raster_phase==0&&motion.raster_velocity==0&&motion.raster_remaining==0,"native reader must reset unsaved raster tween state");
 motion.x=45.9;motion.y=-21.9;motion.scale_x=1.125;motion.angular_velocity=-0.5;yukari::OriginalLayerState from_motion;from_motion.capture_motion(motion);
 require(from_motion.x==45&&from_motion.y==-21&&from_motion.scale_x==1.125&&from_motion.angular_velocity==-0.5,"motion capture lost truncation/native precision");
 motion.x=std::numeric_limits<double>::infinity();from_motion.capture_motion(motion);require(from_motion.x==INT32_MIN,"native x87 indefinite position conversion differs");
 require(yukari::original_saved_transmittance(477.9,0,900.9)==477&&yukari::original_saved_transmittance(477.9,0.125,900.9)==900&&yukari::original_saved_transmittance(477.9,-0.125,100.9)==100,"native opacity writer did not save active tween target");
 uint64_t payload=0x7ff812345678abcdULL;std::memcpy(&state.scale_x,&payload,8);auto nan_header=yukari::encode_original_layer_state(state);require(yukari::encode_original_layer_state(yukari::decode_original_layer_state(nan_header),nan_header)==nan_header,"native IEEE NaN payload was normalized");
 bool rejected=false;try{yukari::decode_original_layer_state(Bytes(319));}catch(const std::runtime_error&){rejected=true;}require(rejected,"truncated native header accepted");
 std::cout<<"native layer color, transform, motion and save/reset mapping tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#pragma once
#include "yukari/formats.hpp"
namespace yukari {
// Original layer fields, independent of SDL. Position velocity is pixels/ms;
// scale velocity is ratio/ms; rotation uses 2000 units per clockwise turn.
struct MotionModel {
 bool visible=true;
 double x=0,y=0;
 int32_t scroll_mode=0,left=0,top=0,right=0,bottom=0;
 double scroll_x=0,scroll_y=0;
 bool scale_enabled=false,scale_scrolling=false;
 double scale_x=1,scale_y=1,scale_target_x=1,scale_target_y=1,scale_velocity_x=0,scale_velocity_y=0;
 int32_t rotate_mode=0,rotate_target=0;
 double angle=0,angular_velocity=0;
 bool raster_enabled=false;
 int32_t raster_target=0,raster_frequency=0,raster_period=0;
 double raster_phase=0,raster_remaining=0,raster_velocity=0;
 // GraphicScroll(id,duration,x,y), (id,denominator,magnitude,x,y),
 // (id,vx1000,vy1000,left,top,right,bottom), or
 // (id,denominator,dx,dy,left,top,right,bottom).
 // Arguments include the layer ID at index 0. Known commands with native
 // unsupported argument counts do nothing and return true.
 bool command(const std::string& name,const std::vector<int32_t>& arguments);
 // timeGetTime DWORD delta, not seconds. The collection's +0x30 multiplier
 // affects scroll/scale/rotation. Raster always uses the unscaled delta.
 // Return is the native redraw decision; fractional positions still advance.
 bool tick(uint32_t delta_ms,double graphics_factor=1,int quality=2);
 void move(double new_x,double new_y);
 double raster_amplitude()const;
 bool raster_visible(int quality=2)const;
 int32_t raster_row_offset(int32_t row,int32_t source_height,int quality=2)const;
 Bytes snapshot()const;
 void restore(const Bytes& bytes);
};
}

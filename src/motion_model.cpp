#include "yukari/motion_model.hpp"
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace yukari {
namespace {
int32_t integer(double n){return !std::isfinite(n)||n<-2147483648.0||n>=2147483648.0?INT32_MIN:int32_t(n);}
int32_t bits(uint32_t n){int32_t out;std::memcpy(&out,&n,4);return out;}
int32_t difference(int32_t a,int32_t b){return bits(uint32_t(a)-uint32_t(b));}
double clamp_target(double current,double velocity,double target){return (velocity<=0?current<target:target<current)?target:current;}
}
void MotionModel::move(double new_x,double new_y){x=new_x;y=new_y;scroll_mode=0;scroll_x=scroll_y=0;}
bool MotionModel::command(const std::string& n,const std::vector<int32_t>& a){
 if(n=="GraphicScroll"){
  if(a.size()==1){if(scroll_mode==2)scroll_mode=1;}
  else if(a.size()==4||a.size()==5){
   const size_t target=a.size()==4?2:3;
   const double dx=double(a[target])-x,dy=double(a[target+1])-y;
   // Native early return leaves a previous motion intact at an equal target.
   if(dx==0&&dy==0)return true;
   if(a.size()==4){scroll_x=dx/a[1];scroll_y=dy/a[1];}
   else{const double magnitude=std::sqrt(double(a[2])*a[2]);const double distance=std::sqrt(dx*dx+dy*dy);scroll_x=(dx*magnitude/distance)/a[1];scroll_y=(magnitude*dy/distance)/a[1];}
   const int32_t ix=integer(x),iy=integer(y);
   if(ix<a[target]){left=ix;right=a[target];}else{left=a[target];right=bits(uint32_t(ix)+1);}
   if(iy<a[target+1]){top=iy;bottom=a[target+1];}else{top=a[target+1];bottom=bits(uint32_t(iy)+1);}
   scroll_mode=1;
  }
  else if(a.size()==7){scroll_x=double(a[1])/1000;scroll_y=double(a[2])/1000;left=a[3];top=a[4];right=a[5];bottom=a[6];scroll_mode=1;}
  else if(a.size()==8){scroll_x=double(a[2])/a[1];scroll_y=double(a[3])/a[1];left=a[4];top=a[5];right=a[6];bottom=a[7];scroll_mode=1;}
  return true;
 }
 if(n=="GraphicScrollLoop"){
  if(a.size()==7){scroll_x=double(a[1])/1000;scroll_y=double(a[2])/1000;left=a[3];top=a[4];right=a[5];bottom=a[6];scroll_mode=2;}
  else if(a.size()==8){scroll_x=double(a[2])/a[1];scroll_y=double(a[3])/a[1];left=a[4];top=a[5];right=a[6];bottom=a[7];scroll_mode=2;}return true;
 }
 if(n=="GraphicScaleScroll"){
  if(a.size()>3){if(!scale_enabled)scale_x=scale_y=1;scale_target_x=double(a[1])/1000;scale_target_y=double(a[2])/1000;scale_velocity_x=(scale_target_x-scale_x)/a[3];scale_velocity_y=(scale_target_y-scale_y)/a[3];scale_scrolling=true;}return true;
 }
 if(n=="GraphicScale"){
  if(a.size()>=3){scale_x=double(a[1])/1000;scale_y=double(a[2])/1000;scale_enabled=scale_x!=1||scale_y!=1;}return true;
 }
 if(n=="GraphicRotateScroll"||n=="GraphicRotateScrollLoop"){
  if(n=="GraphicRotateScrollLoop"&&a.size()==1){rotate_mode=0;rotate_target=0;angular_velocity=0;}
  else if(a.size()>2){rotate_target=a[1];angular_velocity=(double(a[1])-angle)/a[2];rotate_mode=n=="GraphicRotateScroll"?1:2;}return true;
 }
 if(n=="GraphicRotate"){
  if(a.size()>1){angle=a[1];rotate_mode=0;rotate_target=0;angular_velocity=0;}return true;
 }
 if(n=="GraphicRasterScroll"){
  if(a.size()==1)raster_enabled=false;
  else if(a.size()>3){const int duration=a.size()>4?a[4]:0;if(!raster_enabled){raster_target=a[1];raster_frequency=a[2];raster_period=a[3];raster_enabled=true;raster_velocity=double(a[1])/duration;raster_remaining=duration;raster_phase=0;}else{const auto old=raster_target;raster_target=a[1];raster_velocity=double(difference(a[1],old))/duration;raster_remaining=duration;}}return true;
 }
 return false;
}
bool MotionModel::tick(uint32_t milliseconds,double factor,int quality){
 const double raw=milliseconds,dt=raw*factor;bool dirty=false;
 if(scroll_mode&&visible){
  const double nx=x+scroll_x*dt,ny=y+scroll_y*dt;const bool outside=nx<left||right<nx||ny<top||bottom<ny;
  if(outside){x=nx;y=ny;if(scroll_mode==2){if(x<left)x+=difference(right,left);else if(right<x)x-=difference(right,left);if(y<top)y+=difference(bottom,top);else if(bottom<y)y-=difference(bottom,top);}else{scroll_mode=0;if(x<left)x=left;else if(right<x)x=right;if(y<top)y=top;else if(bottom<y)y=bottom;}dirty=true;}
  else{dirty=integer(x)!=integer(nx)||integer(y)!=integer(ny);x=nx;y=ny;}
 }
 if(scale_scrolling){
  scale_x=clamp_target(scale_x+scale_velocity_x*dt,scale_velocity_x,scale_target_x);scale_y=clamp_target(scale_y+scale_velocity_y*dt,scale_velocity_y,scale_target_y);
  if(scale_x==scale_target_x&&scale_y==scale_target_y){scale_scrolling=false;if(scale_x==1&&scale_y==1)scale_enabled=false;}
  else scale_enabled=true;
  dirty=true;
 }
 if(rotate_mode){angle+=angular_velocity*dt;if(rotate_mode==1){angle=clamp_target(angle,angular_velocity,rotate_target);if(angle==rotate_target)rotate_mode=0;}dirty=true;}
 if(raster_enabled&&quality>0){if(milliseconds){raster_phase+=raw/raster_period;if(raster_remaining!=0){raster_remaining-=raw;if(raster_remaining<0)raster_remaining=0;}if(raster_target==0&&raster_remaining==0)raster_enabled=false;}dirty=true;}
 return dirty;
}
double MotionModel::raster_amplitude()const{return raster_remaining!=0?raster_target-raster_velocity*raster_remaining:raster_target;}
bool MotionModel::raster_visible(int quality)const{return raster_enabled&&quality>0&&angle==0&&!(scale_enabled&&(scale_x!=1||scale_y!=1));}
int32_t MotionModel::raster_row_offset(int32_t row,int32_t height,int quality)const{
 if(!raster_visible(quality)||height<=0)return 0;
 const int32_t phase=bits(uint32_t(integer(raster_phase))+uint32_t(row));
 // Original 0x4140f0 multiplies by the stored constant 3.141592, not 2*pi.
 return integer(std::sin(double(phase)/height*raster_frequency*3.141592)*raster_amplitude());
}
Bytes MotionModel::snapshot()const{
 Bytes out={'Y','U','K','A','M','O','T','N'};auto put=[&](uint64_t n,unsigned size=4){for(unsigned i=0;i<size;++i)out.push_back(uint8_t(n>>(8*i)));};auto number=[&](double n){uint64_t bits;std::memcpy(&bits,&n,8);put(bits,8);};put(1);put(unsigned(visible)|(unsigned(scale_enabled)<<1)|(unsigned(scale_scrolling)<<2)|(unsigned(raster_enabled)<<3));
 for(int32_t n:{scroll_mode,left,top,right,bottom,rotate_mode,rotate_target,raster_target,raster_frequency,raster_period})put(uint32_t(n));
 for(double n:{x,y,scroll_x,scroll_y,scale_x,scale_y,scale_target_x,scale_target_y,scale_velocity_x,scale_velocity_y,angle,angular_velocity,raster_phase,raster_remaining,raster_velocity})number(n);return out;
}
void MotionModel::restore(const Bytes& bytes){
 if(bytes.size()!=176||std::memcmp(bytes.data(),"YUKAMOTN",8))throw std::runtime_error("invalid motion snapshot");size_t at=8;auto get=[&](unsigned size=4){uint64_t n=0;for(unsigned i=0;i<size;++i)n|=uint64_t(bytes[at++])<<(8*i);return n;};if(get()!=1)throw std::runtime_error("unsupported motion snapshot");MotionModel candidate;const auto flags=get();if(flags&~uint64_t(15))throw std::runtime_error("invalid motion flags");candidate.visible=flags&1;candidate.scale_enabled=flags&2;candidate.scale_scrolling=flags&4;candidate.raster_enabled=flags&8;
 for(auto p:{&candidate.scroll_mode,&candidate.left,&candidate.top,&candidate.right,&candidate.bottom,&candidate.rotate_mode,&candidate.rotate_target,&candidate.raster_target,&candidate.raster_frequency,&candidate.raster_period})*p=bits(uint32_t(get()));
 for(auto p:{&candidate.x,&candidate.y,&candidate.scroll_x,&candidate.scroll_y,&candidate.scale_x,&candidate.scale_y,&candidate.scale_target_x,&candidate.scale_target_y,&candidate.scale_velocity_x,&candidate.scale_velocity_y,&candidate.angle,&candidate.angular_velocity,&candidate.raster_phase,&candidate.raster_remaining,&candidate.raster_velocity}){const uint64_t raw=get(8);std::memcpy(p,&raw,8);}*this=candidate;
}
}

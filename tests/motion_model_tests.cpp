#include "yukari/motion_model.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace yukari;
void near(double a,double b){assert(std::abs(a-b)<1e-9);}
void scroll(){
 MotionModel m;m.x=99;m.command("GraphicScrollLoop",{1,1000,0,0,0,100,100});assert(m.tick(1));near(m.x,100);assert(m.scroll_mode==2);m.tick(1);near(m.x,1);m.tick(350);near(m.x,251); // Native wraps one span, not modulo.
 m.visible=false;m.tick(100);near(m.x,251);m.visible=true;m.move(99,0);assert(!m.scroll_mode);m.command("GraphicScrollLoop",{1,1000,0,0,0,100,100});m.command("GraphicScroll",{1});m.tick(2);near(m.x,100);assert(!m.scroll_mode);
 m.move(0,0);m.command("GraphicScrollLoop",{1,200,100,-200,-100,-100,100,100});m.tick(10,2);near(m.x,10);near(m.y,-20);
 m.move(.1,.1);m.command("GraphicScrollLoop",{1,1,0,0,0,100,100});assert(!m.tick(1));near(m.x,.101);assert(m.tick(1000));near(m.x,1.101);
}
void ordinary_scroll(){
 MotionModel m;m.move(10.75,20.25);m.command("GraphicScroll",{1,100,30,40});near(m.scroll_x,.1925);near(m.scroll_y,.1975);assert(m.left==10&&m.right==30&&m.top==20&&m.bottom==40&&m.scroll_mode==1);m.tick(100);near(m.x,30);near(m.y,40);assert(m.scroll_mode==1);m.tick(1);assert(m.scroll_mode==0);near(m.x,30);near(m.y,40);
 m.move(30.9,40.9);m.command("GraphicScroll",{1,100,10,20});assert(m.left==10&&m.right==31&&m.top==20&&m.bottom==41);near(m.scroll_x,-.209);near(m.scroll_y,-.209);m.tick(101);near(m.x,10);near(m.y,20);assert(!m.scroll_mode);
 m.move(0,0);m.command("GraphicScroll",{1,10,50,300,400});near(m.scroll_x,3);near(m.scroll_y,4);m.tick(100);near(m.x,300);near(m.y,400);assert(m.scroll_mode==1);m.tick(1);assert(!m.scroll_mode);
 m.move(0,0);m.command("GraphicScroll",{1,10,-50,-300,-400});near(m.scroll_x,-3);near(m.scroll_y,-4);assert(m.right==1&&m.bottom==1); // sqrt(square(magnitude)), negative is not reverse.
 const auto previous=m.snapshot();m.command("GraphicScroll",{1,10,0,0});assert(m.snapshot()==previous);m.command("GraphicScroll",{1,10,0,0,0});assert(m.snapshot()==previous); // same target leaves running motion.
 m.command("GraphicScroll",{1,1000,-2000,-100,-100,100,100});near(m.scroll_x,1);near(m.scroll_y,-2);assert(m.scroll_mode==1);m.tick(1000);near(m.x,100);near(m.y,-100);assert(!m.scroll_mode);
 m.move(0,0);m.command("GraphicScroll",{1,200,100,-200,-100,-100,100,100});m.tick(10,2);near(m.x,10);near(m.y,-20);assert(m.scroll_mode==1);
 const auto saved=m.snapshot();assert(m.command("GraphicScroll",{1,2,3})&&m.snapshot()==saved);m.scroll_mode=0;m.command("GraphicScroll",{1});assert(!m.scroll_mode);
}
void scale_rotate(){
 MotionModel m;m.command("GraphicScaleScroll",{1,2000,500,100});m.tick(25);near(m.scale_x,1.25);near(m.scale_y,.875);assert(m.scale_enabled&&m.scale_scrolling);m.visible=false;m.tick(25);near(m.scale_x,1.5);m.tick(50);assert(!m.scale_scrolling&&m.scale_enabled);near(m.scale_y,.5);
 m.command("GraphicScaleScroll",{1,1000,1000,100});m.tick(100);assert(!m.scale_enabled&&!m.scale_scrolling);
 MotionModel immediate;immediate.command("GraphicScaleScroll",{1,2000,2000,10});immediate.tick(10);near(immediate.scale_x,2);assert(!immediate.scale_enabled); // Original +1132 flag quirk.
 m.command("GraphicScaleScroll",{1,2000,2000,100});m.tick(10);m.command("GraphicScale",{1,1500,1500});assert(m.scale_scrolling);m.tick(10);near(m.scale_x,1.6);
 m.command("GraphicRotateScroll",{1,1000,200});m.tick(50,2);near(m.angle,500);m.tick(100);near(m.angle,1000);assert(!m.rotate_mode);
 m.command("GraphicRotateScrollLoop",{1,2000,100});m.tick(150);near(m.angle,2500);m.tick(100);near(m.angle,3500);m.command("GraphicRotateScrollLoop",{1});assert(!m.rotate_mode&&m.angular_velocity==0);near(m.angle,3500);
 m.command("GraphicRotateScrollLoop",{1,4500,100});m.command("GraphicRotate",{1,1800});assert(!m.rotate_mode);near(m.angle,1800);
}
void raster(){
 MotionModel m;m.command("GraphicRasterScroll",{1,20,4,10,100});near(m.raster_amplitude(),0);m.tick(25,4);near(m.raster_phase,2.5);near(m.raster_amplitude(),5);
 m.command("GraphicRasterScroll",{1,40,99,999,50});assert(m.raster_frequency==4&&m.raster_period==10);near(m.raster_amplitude(),20);m.tick(25,0);near(m.raster_phase,5);near(m.raster_amplitude(),30);
 m.tick(25,1,0);near(m.raster_phase,5);near(m.raster_remaining,25);m.tick(25);near(m.raster_amplitude(),40);assert(m.tick(0));
 m.command("GraphicRasterScroll",{1});assert(!m.raster_enabled);m.command("GraphicRasterScroll",{1,10,2,10});assert(m.raster_visible());assert(m.raster_row_offset(0,8)==0&&m.raster_row_offset(2,8)==9&&m.raster_row_offset(6,8)==-9);m.raster_phase=.9;assert(m.raster_row_offset(2,8)==9);m.raster_phase=1.1;assert(m.raster_row_offset(3,8)==0);
 m.scale_enabled=true;m.scale_x=2;assert(!m.raster_visible());m.scale_x=1;assert(m.raster_visible());m.angle=1;assert(!m.raster_visible());m.angle=0;m.command("GraphicRasterScroll",{1,0,0,0,100});m.tick(100);assert(!m.raster_enabled);
}
void state(){
 MotionModel m;m.move(13.5,-3);m.command("GraphicScrollLoop",{1,99,44,-100,-100,100,100});m.command("GraphicScaleScroll",{1,2000,500,100});m.command("GraphicRotateScrollLoop",{1,500,200});m.command("GraphicRasterScroll",{1,10,3,20,80});m.tick(17);
 const auto data=m.snapshot();MotionModel restored;restored.restore(data);assert(restored.snapshot()==data);m.tick(31);restored.tick(31);assert(restored.snapshot()==m.snapshot());auto before=restored.snapshot();auto bad=before;bad.pop_back();bool rejected=false;try{restored.restore(bad);}catch(const std::runtime_error&){rejected=true;}assert(rejected&&restored.snapshot()==before);
 bad=before;bad[12]|=0x80;rejected=false;try{restored.restore(bad);}catch(const std::runtime_error&){rejected=true;}assert(rejected&&restored.snapshot()==before);assert(!restored.command("Unknown",{}));
}
int main(){scroll();ordinary_scroll();scale_rotate();raster();state();std::cout<<"native millisecond scroll, scale/rotate, raster phase, boundary rules and snapshot passed\n";}

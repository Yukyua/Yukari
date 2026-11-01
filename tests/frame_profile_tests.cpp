#include "yukari/frame_profile.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace yukari;
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
int main(){
 const auto dir=std::filesystem::temp_directory_path()/"yukari-frame-profile-test";
 std::filesystem::remove_all(dir);FrameProfile p;std::ostringstream log;
 require(!p.active(),"unstarted profiler active");p.begin("boundary",4,20,dir);
 for(int i=0;i<3;++i)require(!p.add({double(i+1),0,0,0,0,1,2,3},log),"profile finished early");
 require(log.str().empty()&&!std::filesystem::exists(dir),"report IO occurred inside measured frames");
 require(p.add({20,0,0,0,0,1,2,3},log)&&!p.active(),"20ms boundary must fail strict budget");
 require(log.str().find("p50_ms=2.000")!=std::string::npos&&log.str().find("p99_ms=20.000")!=std::string::npos&&log.str().find("over_budget=1")!=std::string::npos,"quantiles/budget wrong");
 std::ifstream csv(dir/"frames-boundary.csv");std::string line;int count=0;while(std::getline(csv,line))++count;require(count==5,"CSV lost frames");
 p.begin("ok",1,20,dir);require(!p.add({19.99},log),"under budget rejected");
 bool rejected=false;try{p.begin("../escape",1,0,dir);}catch(const std::runtime_error&){rejected=true;}require(rejected,"unsafe label accepted");
 std::filesystem::remove_all(dir);std::cout<<"frame quantiles, strict budget and CSV passed\n";
}

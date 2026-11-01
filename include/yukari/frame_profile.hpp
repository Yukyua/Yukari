#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace yukari {
struct FrameSample {double frame_ms{},update_ms{},render_ms{},present_ms{},sleep_ms{};uint64_t gpu_operations{},texture_updates{},image_decodes{},vm_steps{};double vm_ms{};};
class FrameProfile {
public:
 void begin(std::string label,size_t frames,double budget,std::filesystem::path directory){
  if(label.empty()||label.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")!=std::string::npos||!frames||frames>100000||!std::isfinite(budget)||budget<0)throw std::runtime_error("invalid profile label/frame count/budget");
  label_=std::move(label);count_=frames;budget_=budget;directory_=std::move(directory);samples_.clear();samples_.reserve(frames);
 }
 bool active()const{return samples_.size()<count_;}
 size_t frame()const{return samples_.size();}
 bool add(FrameSample sample,std::ostream& log){
  samples_.push_back(sample);if(active())return false;
  std::filesystem::create_directories(directory_);std::ofstream out(directory_/("frames-"+label_+".csv"));if(!out)throw std::runtime_error("cannot create frame profile");
  out<<"frame,frame_ms,update_ms,render_ms,present_ms,sleep_ms,gpu_operations,texture_updates,image_decodes,vm_steps,vm_ms\n"<<std::fixed<<std::setprecision(6);
  std::vector<double> elapsed,update,render,present;size_t over=0;uint64_t operations=0,updates=0,decodes=0;
  for(size_t i=0;i<samples_.size();++i){const auto& s=samples_[i];elapsed.push_back(s.frame_ms);update.push_back(s.update_ms);render.push_back(s.render_ms);present.push_back(s.present_ms);if(budget_&&s.frame_ms>=budget_)++over;operations+=s.gpu_operations;updates+=s.texture_updates;decodes+=s.image_decodes;
   out<<i<<','<<s.frame_ms<<','<<s.update_ms<<','<<s.render_ms<<','<<s.present_ms<<','<<s.sleep_ms<<','<<s.gpu_operations<<','<<s.texture_updates<<','<<s.image_decodes<<','<<s.vm_steps<<','<<s.vm_ms<<'\n';}
  if(!out)throw std::runtime_error("cannot write frame profile");
  auto q=[](std::vector<double> values,double p){std::sort(values.begin(),values.end());return values[size_t(std::ceil(p*values.size()))-1];};
  log<<std::fixed<<std::setprecision(3)<<"FRAME_PROFILE label="<<label_<<" frames="<<samples_.size()<<" p50_ms="<<q(elapsed,.5)<<" p95_ms="<<q(elapsed,.95)<<" p99_ms="<<q(elapsed,.99)<<" max_ms="<<q(elapsed,1)<<" update_p99_ms="<<q(update,.99)<<" render_p99_ms="<<q(render,.99)<<" present_p99_ms="<<q(present,.99)<<" gpu_operations="<<operations<<" texture_updates="<<updates<<" image_decodes="<<decodes<<" budget_ms="<<budget_<<" over_budget="<<over<<'\n';log.unsetf(std::ios::floatfield);return over!=0;
 }
private:
 std::string label_;size_t count_{};double budget_{};std::filesystem::path directory_;std::vector<FrameSample> samples_;
};
}

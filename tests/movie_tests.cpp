#include "yukari/movie.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <set>
using namespace yukari;
namespace {
void check(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
template<class F>void fails(F f){try{f();}catch(const std::runtime_error&){return;}throw std::runtime_error("expected movie error");}
uint64_t hash(const Bytes& bytes){uint64_t h=14695981039346656037ULL;for(auto x:bytes)h=(h^x)*1099511628211ULL;return h;}
}
int main(int argc,char** argv){try{
    if(argc<2)throw std::runtime_error("usage: movie_tests GENERATED_ONE_SECOND_MPEG [--game-movie ORIGINAL_MPG]");
    Movie movie(argv[1]);check(movie.info().width==64&&movie.info().height==48,"fixture dimensions");
    check(movie.info().has_audio&&movie.info().audio_rate==48000&&movie.info().audio_channels==2,"resampled audio format");
    std::set<uint64_t> hashes;uint64_t audio_samples=0,last_audio=0;bool first_audio=true,signal=false;
    bool done=false;
    for(uint64_t t=0;t<=3000;t+=20){
        bool changed=movie.advance(t);
        if(changed){auto f=movie.frame();check(f&&f->rgba.size()==64*48*4,"RGBA dimensions");check(f->position_ms<=t,"no future frame presented");
            for(size_t i=3;i<f->rgba.size();i+=4)check(f->rgba[i]==255,"opaque movie alpha");hashes.insert(hash(f->rgba));}
        for(const auto& chunk:movie.take_audio()){
            check(first_audio||chunk.position_ms>=last_audio,"audio timestamps monotonic");first_audio=false;last_audio=chunk.position_ms;
            check(chunk.samples.size()%2==0,"stereo alignment");audio_samples+=chunk.samples.size()/2;
            for(float x:chunk.samples){check(std::isfinite(x),"finite audio sample");if(std::abs(x)>0.001f)signal=true;}
        }
        if(movie.finished(t)){done=true;break;}
    }
    check(done,"finite movie completes after decoder/resampler flush");
    check(movie.decoded_video_frames()==25,"all 25 frames including delayed B frames");
    check(movie.decoded_audio_frames()>0&&audio_samples>=47000&&audio_samples<=53000,"one second audio is decoded/resampled");
    check(signal&&hashes.size()>10,"video changes and audio contains signal");
    fails([&]{movie.advance(0);});
    fails([]{Movie absent("/tmp/yukari-intentionally-missing-movie-file.mpg");});
    std::cout<<"movie fixture: frames="<<movie.decoded_video_frames()<<" audio_samples="<<audio_samples<<" distinct_images="<<hashes.size()<<'\n';
    if(argc==4&&(std::string(argv[2])=="--game-movie"||std::string(argv[2])=="--game-movie-full")){
        Movie original(argv[3]);check(original.info().width==1024&&original.info().height==576,"original movie dimensions");
        check(original.info().has_audio,"original MPEG audio");uint64_t samples=0;
        bool full=std::string(argv[2])=="--game-movie-full",complete=false;
        uint64_t limit=full?original.info().duration_ms+2000:2000;
        for(uint64_t t=0;t<=limit;t+=20){original.advance(t);for(auto& c:original.take_audio())samples+=c.samples.size()/2;if(original.finished(t)){complete=true;break;}}
        check(original.frame()&&original.decoded_video_frames()>50&&samples>=96000,"original movie demux keeps audio buffered ahead of video clock");
        if(full)check(complete&&original.decoded_video_frames()>2900&&samples>4600000,"complete original movie decodes and flushes");
        std::cout<<"original movie: frames="<<original.decoded_video_frames()<<" audio_samples="<<samples<<" duration_ms="<<original.info().duration_ms<<'\n';
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

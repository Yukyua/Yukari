#include "yukari/audio.hpp"
#include <SDL.h>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
using namespace yukari;
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
void put(Bytes& out,size_t at,uint64_t n,unsigned count=4){for(unsigned i=0;i<count;++i)out[at+i]=uint8_t(n>>(8*i));}
Bytes wav(uint32_t rate,uint32_t frames){Bytes b(44+size_t(frames)*2);std::copy_n("RIFF",4,b.begin());std::copy_n("WAVEfmt ",8,b.begin()+8);std::copy_n("data",4,b.begin()+36);put(b,4,b.size()-8);put(b,16,16);put(b,20,1,2);put(b,22,1,2);put(b,24,rate);put(b,28,rate*2);put(b,32,2,2);put(b,34,16,2);put(b,40,size_t(frames)*2);for(size_t i=0;i<frames;++i)put(b,44+i*2,uint16_t(int(i%123)*123-7000),2);return b;}
std::vector<float> whole_pcm(const Bytes& wav,uint32_t rate){
 std::vector<float> input((wav.size()-44)/2);for(size_t i=0;i<input.size();++i){uint16_t bits=uint16_t(wav[44+i*2])|(uint16_t(wav[45+i*2])<<8);int16_t sample;std::memcpy(&sample,&bits,2);input[i]=float(sample)/32768.0f;}
 std::unique_ptr<SDL_AudioStream,decltype(&SDL_FreeAudioStream)> converter(SDL_NewAudioStream(AUDIO_F32SYS,1,rate,AUDIO_F32SYS,2,48000),SDL_FreeAudioStream);check(bool(converter),"oracle converter creation failed");check(SDL_AudioStreamPut(converter.get(),input.data(),int(input.size()*sizeof(float)))==0&&SDL_AudioStreamFlush(converter.get())==0,"oracle conversion failed");auto bytes=SDL_AudioStreamAvailable(converter.get());check(bytes>0&&bytes%8==0,"oracle PCM size invalid");std::vector<float> output(size_t(bytes)/sizeof(float));check(SDL_AudioStreamGet(converter.get(),output.data(),bytes)==bytes,"oracle output failed");return output;
}
struct Temp {std::filesystem::path path=std::filesystem::temp_directory_path()/("yukari-stream-"+std::to_string(getpid()));Temp(){std::filesystem::create_directory(path);}~Temp(){std::filesystem::remove_all(path);}};
int main(){try{
 SDL_setenv("SDL_AUDIODRIVER","dummy",1);Temp temp;
 auto short_wav=wav(48000,48000);auto path=temp.path/"range.bin";
 {std::ofstream f(path,std::ios::binary);std::string prefix(37,'P'),suffix(19,'S');f.write(prefix.data(),prefix.size());f.write(reinterpret_cast<const char*>(short_wav.data()),short_wav.size());f.write(suffix.data(),suffix.size());}
 auto container=ResourceSource::file(path),slice=container.slice(37,short_wav.size());check(slice.read_all()==short_wav,"range crossed archive entry or changed bytes");uint8_t byte{};check(slice.read_at(slice.size(),&byte,1)==0,"entry EOF read leaked neighbor");bool rejected=false;try{slice.read_at(slice.size()+1,&byte,1);}catch(...){rejected=true;}check(rejected,"out-of-range read accepted");
 std::thread one([&]{for(size_t i=0;i<500;++i){uint8_t b{};auto at=(i*191)%short_wav.size();slice.read_at(at,&b,1);check(b==short_wav[at],"concurrent cursor changed");}});std::thread two([&]{for(size_t i=0;i<500;++i){uint8_t b{};auto at=(i*97)%short_wav.size();slice.read_at(at,&b,1);check(b==short_wav[at],"concurrent cursor changed");}});one.join();two.join();
 // A 1-hour sparse PCM source would need 1.38 GB of decoded stereo floats.
 auto big=temp.path/"hour.wav";constexpr uint32_t frames=48000*3600;
 {auto header=wav(48000,0);put(header,4,uint64_t(frames)*2+36);put(header,40,uint64_t(frames)*2);std::ofstream f(big,std::ios::binary);f.write(reinterpret_cast<const char*>(header.data()),header.size());f.seekp(43+uint64_t(frames)*2);f.put(0);}
 auto large=ResourceSource::file(big);Audio bounded(false);bounded.load_sources(1,{large});check(!bounded.streamed(1),"ordinary SoundLoad lost native contract");bounded.play(1);std::vector<float> output(4096);bounded.render(output.data(),2048);auto stats=bounded.decode_stats();check(stats.pcm_capacity_bytes==65536&&stats.queued_frames<=8192&&stats.decoded_frames<=12288,"PCM grew with file duration");check(large.io_stats().bytes<256*1024,"stream eagerly read complete encoded source");check(bounded.duration_ms(1)==3600000,"long stream duration truncated");
 for(auto rate:{22050u,44100u,48000u}){
  auto bytes=wav(rate,rate*2);Audio a(false),b(false);a.load(2,bytes);b.load(2,bytes);a.play(2,true);a.render(output.data(),777);auto saved=a.snapshot();SDL_Delay(8);check(a.snapshot()==saved,"read-ahead changed consumed save cursor");b.restore_playback(saved);check(b.snapshot()==saved,"stream snapshot metadata roundtrip changed");std::vector<float> expected(8192),actual(8192);a.render(expected.data(),4096);b.render(actual.data(),4096);check(expected==actual,"stream restore changed resampled samples");
 }
 // Canonical legacy v5 buffered payload, independent of the v6 stream layout.
 Audio legacy(false),reference(false);legacy.load_sources(3,{slice});reference.load_sources(3,{slice});auto old=legacy.snapshot();check(u32(old,8)==6,"unexpected new audio format");old.erase(old.begin()+74,old.begin()+111);put(old,8,5);put(old,20,48000*2,8);put(old,28,1234*2,8);old[44]=1;legacy.restore_playback(old);reference.play(3);std::vector<float> discard(1234*2);reference.render(discard.data(),1234);std::vector<float> expected(256),actual(256);reference.render(expected.data(),128);legacy.render(actual.data(),128);check(expected==actual,"legacy buffered save did not resume same sample");
 {
  auto first=wav(22050,44063),second=wav(44100,88183);auto a=whole_pcm(first,22050),b=whole_pcm(second,44100);std::vector<float> oracle=a;oracle.insert(oracle.end(),b.begin(),b.end());Audio old_parts(false);old_parts.load_parts(8,{first,second});auto saved=old_parts.snapshot();auto end=74+4+u32(saved,74)*16+4+8+4+1;saved.erase(saved.begin()+74,saved.begin()+end);put(saved,8,5);put(saved,20,oracle.size(),8);put(saved,28,a.size()+646,8);saved[44]=1;old_parts.restore_playback(saved);std::vector<float> actual(8192);old_parts.render(actual.data(),4096);check(std::equal(actual.begin(),actual.end(),oracle.begin()+a.size()+646),"legacy multipart cursor changed against whole-file resampling oracle");
 }
 {Audio realtime;realtime.load_sources(4,{large});realtime.play(4,true);SDL_Delay(220);auto live=realtime.decode_stats();check(live.decoded_frames>8192,"background decoder did not refill during idle main thread");check(live.pcm_capacity_bytes==65536,"real device allocated unbounded PCM");check(realtime.playing(4),"real streaming source stopped early");std::cout<<"real callback underruns="<<live.underruns<<'\n';}
 std::cout<<"Entry-bounded concurrent I/O, 1-hour bounded stream, background refill, resampled sample-exact v6 restore and legacy v5 migration passed; encoded bytes read="<<large.io_stats().bytes<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#include "yukari/audio.hpp"
#include "yukari/audio_gain.hpp"
#include <SDL.h>
#include <sndfile.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <atomic>

namespace yukari {
namespace {
int negative_id(int id){const uint32_t value=uint32_t(0)-uint32_t(id);int32_t result;std::memcpy(&result,&value,4);return result;}
int scale_gain(int a,int b){const int64_t n=int64_t(a)*b/1000;return n<std::numeric_limits<int32_t>::min()||n>std::numeric_limits<int32_t>::max()?std::numeric_limits<int32_t>::min():int(n);}
struct MemoryFile {ResourceSource source;sf_count_t cursor{};bool failed=false;};
sf_count_t file_size(void* data){return sf_count_t(static_cast<MemoryFile*>(data)->source.size());}
sf_count_t seek_file(sf_count_t offset,int whence,void* data){auto& f=*static_cast<MemoryFile*>(data);sf_count_t base=whence==SEEK_SET?0:whence==SEEK_CUR?f.cursor:whence==SEEK_END?sf_count_t(f.source.size()):-1;if(base<0||(offset>0&&base>std::numeric_limits<sf_count_t>::max()-offset)||(offset<0&&offset< -base))return -1;auto next=base+offset;if(next<0||uint64_t(next)>f.source.size())return -1;f.cursor=next;return next;}
sf_count_t read_file(void* output,sf_count_t count,void* data){auto& f=*static_cast<MemoryFile*>(data);if(count<=0)return 0;try{auto n=f.source.read_at(uint64_t(f.cursor),output,size_t(std::min<uint64_t>(uint64_t(count),SIZE_MAX)));f.cursor+=sf_count_t(n);return sf_count_t(n);}catch(...){f.failed=true;return 0;}}
sf_count_t write_file(const void*,sf_count_t,void*){return 0;}
sf_count_t tell_file(void* data){return static_cast<MemoryFile*>(data)->cursor;}

struct StreamDecoder {
    ResourceSource bytes;
    MemoryFile memory;
    SF_INFO info{};
    std::unique_ptr<SNDFILE,decltype(&sf_close)> file{nullptr,sf_close};
    std::unique_ptr<SDL_AudioStream,decltype(&SDL_FreeAudioStream)> converter{nullptr,SDL_FreeAudioStream};
    bool eof=false;
    explicit StreamDecoder(ResourceSource source):bytes(std::move(source)),memory{bytes}{
        SF_VIRTUAL_IO io{file_size,seek_file,read_file,write_file,tell_file};
        file.reset(sf_open_virtual(&io,SFM_READ,&info,&memory));
        if(!file)throw std::runtime_error(std::string("cannot open audio stream: ")+sf_strerror(nullptr));
        if(info.frames<=0||info.channels<=0||info.channels>8||info.samplerate<1000||info.samplerate>384000||uint64_t(info.frames)>(std::numeric_limits<uint64_t>::max()-384000)/48000)throw std::runtime_error("invalid audio stream dimensions");
        converter.reset(SDL_NewAudioStream(AUDIO_F32SYS,Uint8(info.channels),info.samplerate,AUDIO_F32SYS,2,48000));
        if(!converter)throw std::runtime_error(SDL_GetError());
    }
    uint64_t frames()const{return (uint64_t(info.frames)*48000+unsigned(info.samplerate)-1)/unsigned(info.samplerate);}
    size_t read(float* output,size_t frames){
        size_t written=0;std::array<float,4096> input;
        while(written<frames){
            int available=SDL_AudioStreamAvailable(converter.get());if(available<0)throw std::runtime_error(SDL_GetError());
            if(available){size_t n=std::min<size_t>(size_t(available)/(2*sizeof(float)),frames-written);if(!n)throw std::runtime_error("unaligned streamed audio output");const int bytes=int(n*2*sizeof(float));if(SDL_AudioStreamGet(converter.get(),output+written*2,bytes)!=bytes)throw std::runtime_error(SDL_GetError());written+=n;continue;}
            if(eof)break;
            auto n=sf_readf_float(file.get(),input.data(),512);
            if(n<0||memory.failed||sf_error(file.get())!=SF_ERR_NO_ERROR)throw std::runtime_error("cannot decode streamed audio payload");
            if(n){if(SDL_AudioStreamPut(converter.get(),input.data(),int(n*info.channels*sizeof(float)))<0)throw std::runtime_error(SDL_GetError());}
            else{eof=true;if(sf_seek(file.get(),0,SEEK_CUR)<info.frames)throw std::runtime_error("truncated streamed audio payload");if(SDL_AudioStreamFlush(converter.get())<0)throw std::runtime_error(SDL_GetError());}
        }
        return written;
    }
};
// Eight fixed chunks = 64 KiB of decoded stereo PCM per loaded channel.
// Producer owns the decoder. Consumer owns logical segment/cursor; read-ahead
// never advances save positions. Registry retains streams for off-callback GC.
struct StreamState {
    static constexpr size_t chunk_frames=1024,chunk_count=8;
    struct Chunk {std::array<float,chunk_frames*2> pcm;size_t count{},used{};uint32_t segment{};uint64_t begin{};};
    std::vector<ResourceSource> parts;std::vector<uint64_t> lengths;
    uint32_t segment=0;int32_t loop_start=0;uint64_t cursor=0,total_cursor=0;
    bool command_stream=true;std::atomic<bool> failed{false};
    std::mutex mutex;std::condition_variable ready;std::condition_variable* wake{};
    std::array<Chunk,chunk_count> chunks;size_t head{},queued{};
    bool wanted_loop=false,producer_loop=false,ended=false;uint64_t generation=1,producer_generation=0;
    uint32_t producer_segment=0;uint64_t producer_cursor=0;std::unique_ptr<StreamDecoder> decoder;
    std::string error;uint64_t decoded_frames{};std::atomic<uint64_t> underruns{};uint64_t seek_remaining{},absolute_seek{};bool seek_across_parts=false;
    void configure(bool loop){std::lock_guard<std::mutex> lock(mutex);if(wanted_loop!=loop){wanted_loop=loop;++generation;head=queued=0;ended=false;}if(wake)wake->notify_one();}
    bool produce(){
        std::unique_lock<std::mutex> lock(mutex);
        if(!error.empty()||queued==chunk_count)return false;
        const auto revision=generation;const auto at=segment;const auto position=cursor;const auto loop=wanted_loop;
        if(producer_generation==revision&&ended)return false;
        lock.unlock();
        try{
            if(producer_generation!=revision){decoder.reset();producer_segment=at;producer_cursor=0;producer_loop=loop;producer_generation=revision;
                if(at<parts.size()){decoder=std::make_unique<StreamDecoder>(parts[at]);seek_remaining=seek_across_parts?absolute_seek:position;}}
            if(seek_remaining){std::array<float,8192> discard;auto n=decoder->read(discard.data(),size_t(std::min<uint64_t>(seek_remaining,4096)));
                if(!n){if(seek_across_parts&&producer_segment+1<parts.size()){++producer_segment;producer_cursor=0;decoder=std::make_unique<StreamDecoder>(parts[producer_segment]);return true;}throw std::runtime_error("stream snapshot cursor exceeds decoded segment");}
                seek_remaining-=n;producer_cursor+=n;if(!seek_remaining&&seek_across_parts){lock.lock();if(generation==revision){segment=producer_segment;cursor=producer_cursor;total_cursor=absolute_seek;seek_across_parts=false;}lock.unlock();}return true;}
            // Decode away from the queue lock. The fixed local chunk is bounded.
            Chunk output{};size_t transitions=0;
            while(producer_segment<parts.size()){
                if(!decoder)decoder=std::make_unique<StreamDecoder>(parts[producer_segment]);output.segment=producer_segment;output.begin=producer_cursor;output.count=decoder->read(output.pcm.data(),chunk_frames);
                if(output.count){producer_cursor+=output.count;break;}
                decoder.reset();producer_cursor=0;
                if(producer_segment+1<parts.size())++producer_segment;else producer_segment=producer_loop&&loop_start>=0&&uint32_t(loop_start)<parts.size()?uint32_t(loop_start):uint32_t(parts.size());
                if(++transitions>parts.size()+1)throw std::runtime_error("empty audio stream loop");
            }
            lock.lock();if(generation!=revision)return true;
            if(output.count){auto& slot=chunks[(head+queued)%chunk_count];slot=std::move(output);++queued;decoded_frames+=slot.count;}else ended=true;
            ready.notify_all();return true;
        }catch(const std::exception& e){if(!lock.owns_lock())lock.lock();if(generation==revision){error=e.what();failed=true;ended=true;ready.notify_all();}return true;}
    }
    size_t read(float* output,size_t frames,bool loop,bool wait){
        std::unique_lock<std::mutex> lock(mutex,std::defer_lock);if(wait)lock.lock();else if(!lock.try_lock()){++underruns;return 0;}
        if(wanted_loop!=loop){wanted_loop=loop;++generation;head=queued=0;ended=false;if(wake)wake->notify_one();}
        size_t done=0;
        while(done<frames){
            if(!queued){if(ended||!error.empty()||!wait)break;ready.wait(lock,[&]{return queued||ended||!error.empty();});continue;}
            auto& chunk=chunks[head];auto n=std::min(frames-done,chunk.count-chunk.used);std::copy_n(chunk.pcm.data()+chunk.used*2,n*2,output+done*2);chunk.used+=n;done+=n;
            segment=chunk.segment;cursor=chunk.begin+chunk.used;total_cursor=cursor;for(uint32_t i=0;i<segment;++i)total_cursor+=lengths[i];if(loop&&total_cursor>=logical_size())total_cursor=0;
            if(chunk.used==chunk.count){head=(head+1)%chunk_count;--queued;if(wake)wake->notify_one();}
        }
        if(done<frames&&!ended&&error.empty())++underruns;
        return done;
    }
    void prime(){std::unique_lock<std::mutex> lock(mutex);ready.wait(lock,[&]{return queued||ended||!error.empty();});if(!error.empty())throw std::runtime_error("audio stream: "+error);}
    bool finished(){std::unique_lock<std::mutex> lock(mutex,std::try_to_lock);return lock.owns_lock()&&((ended&&!queued)||!error.empty());}
    void check_error()const{auto& self=const_cast<StreamState&>(*this);std::lock_guard<std::mutex> lock(self.mutex);if(!error.empty())throw std::runtime_error("audio stream: "+error);}
    uint64_t logical_size()const{uint64_t sum=0;for(auto n:lengths)sum+=n;return sum;}
    std::shared_ptr<StreamState> clone_total(uint64_t frame)const{auto out=clone(0,0);out->absolute_seek=frame;out->seek_across_parts=frame!=0;return out;}
    std::shared_ptr<StreamState> clone(uint32_t at,uint64_t frame)const{
        if(at>parts.size()||(at==parts.size()&&frame)||(at<parts.size()&&frame>lengths[at]+4096))throw std::runtime_error("invalid streamed audio cursor: segment="+std::to_string(at)+" frame="+std::to_string(frame)+" parts="+std::to_string(parts.size()));
        auto out=std::make_shared<StreamState>();out->parts=parts;out->lengths=lengths;out->loop_start=loop_start;out->command_stream=command_stream;out->segment=at;out->cursor=frame;for(uint32_t i=0;i<at;++i)out->total_cursor+=lengths[i];out->total_cursor+=frame;return out;
    }
};
struct DecodeWorker {
    std::mutex mutex;std::condition_variable wake;std::vector<std::shared_ptr<StreamState>> states;bool stop=false;std::thread thread;
    DecodeWorker():thread([this]{run();}){}
    ~DecodeWorker(){{std::lock_guard<std::mutex> lock(mutex);stop=true;}wake.notify_all();thread.join();}
    void track(const std::shared_ptr<StreamState>& state){std::lock_guard<std::mutex> lock(mutex);state->wake=&wake;states.push_back(state);wake.notify_one();}
    void run(){
        std::unique_lock<std::mutex> lock(mutex);
        while(!stop){
            for(auto it=states.begin();it!=states.end();)if(it->use_count()==1)it=states.erase(it);else ++it;
            auto work=states;lock.unlock();bool progress=false;for(auto& state:work)progress=state->produce()||progress;work.clear();lock.lock();
            if(!progress)wake.wait_for(lock,std::chrono::milliseconds(2));
        }
        states.clear();
    }
};
struct DeviceLock {
    SDL_AudioDeviceID device;
    explicit DeviceLock(SDL_AudioDeviceID id) : device(id) { if(device)SDL_LockAudioDevice(device); }
    ~DeviceLock() { if(device)SDL_UnlockAudioDevice(device); }
};
}
struct Audio::Impl {
    struct Fade {
        int from{},target{};uint64_t elapsed{},length{};bool active{};
        int advance(uint64_t samples){elapsed=std::min(length,elapsed+samples);if(elapsed==length)active=false;return length?int(from+(int64_t(target)-from)*int64_t(elapsed)/int64_t(length)):target;}
    };
    struct Channel {
        std::shared_ptr<const std::vector<float>> samples;
        std::shared_ptr<StreamState> stream;std::string error;
        size_t cursor{};
        int volume{1000}, group{};
        bool has_group{}, active{}, loop{}, play_loop{}, silent{}, fade_stop{}, fade_erase{},one_only{};
        int saved_volume{1000},attenuation{};
        Fade fade;
    };
    struct Group { int first{}, last{}, volume{1000}; uint64_t assignment{}; bool assigned{},silent{};int auto_skip{};Fade fade; };
    struct AudioSubsystem {
        AudioSubsystem() { if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) throw std::runtime_error(SDL_GetError()); }
        ~AudioSubsystem() { SDL_QuitSubSystem(SDL_INIT_AUDIO); }
    };
    std::unique_ptr<AudioSubsystem> subsystem;
    SDL_AudioDeviceID device{};
    DecodeWorker worker;
    std::map<int, Channel> channels;
    std::map<int, Group> groups;
    uint64_t group_assignment{};
    bool muted{},virtual_clock{},logarithmic_volume{};
    int restrain_level=1000;std::vector<int> history_groups,restrain_groups;
    explicit Impl(bool open_device) {
        if(!open_device)return;
        subsystem=std::make_unique<AudioSubsystem>();
        SDL_AudioSpec desired{};
        desired.freq = 48000; desired.format = AUDIO_F32SYS; desired.channels = 2; desired.samples = 1024;
        desired.callback = callback; desired.userdata = this;
        device = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
        if (!device) throw std::runtime_error(std::string("cannot open audio device: ") + SDL_GetError());
        SDL_PauseAudioDevice(device, 0);
    }
    ~Impl() { if (device) SDL_CloseAudioDevice(device); }
    void assign_group(int id, Channel& channel) {
        channel.has_group = false;
        uint64_t newest{};
        for (const auto& item : groups) {
            const auto& group = item.second;
            if (group.assigned && id >= group.first && id <= group.last && group.assignment > newest) {
                newest = group.assignment;
                channel.has_group = true; channel.group = item.first;
            }
        }
    }
    int gain(int id,const Channel& channel,bool include_muted=true) const {
        if((include_muted&&muted)||channel.silent)return 0;
        int combined=1000;
        for(const auto& item:groups){const auto& group=item.second;if(group.assigned&&group.first<=id&&id<=group.last){if(group.silent)return 0;combined=scale_gain(combined,group.volume);}}
        return scale_gain(combined,channel.volume);
    }
    void refresh_gain(int id,Channel& channel){const auto attenuation=native_sound_attenuation(gain(id,channel,false),logarithmic_volume);if(native_sound_attenuation_valid(attenuation))channel.attenuation=attenuation;}
    void refresh_gains(){for(auto& item:channels)refresh_gain(item.first,item.second);}
    float pcm_gain(int id,Channel& channel){refresh_gain(id,channel);return muted?0:float(direct_sound_pcm_gain(channel.attenuation));}
    void restrain(){
        if(restrain_level==1000)return;
        bool voice=false;
        for(const auto& item:channels){const auto& c=item.second;if(!c.active||c.loop)continue;for(int id:history_groups){auto g=groups.find(id);if(g!=groups.end()&&g->second.assigned&&g->second.first<=item.first&&item.first<=g->second.last){voice=true;break;}}if(voice)break;}
        for(int id:restrain_groups){auto found=groups.find(id);if(found==groups.end())continue;auto& g=found->second;if(g.fade.active)continue;int target=voice?restrain_level:1000;if(g.volume!=target){g.fade={g.volume,target,0,uint64_t(voice?200:500)*96,!g.silent};if(g.silent)g.volume=target;}}
    }
    static void callback(void* userdata, Uint8* output, int bytes) {
        auto& self = *static_cast<Impl*>(userdata);
        std::memset(output, 0, size_t(bytes));
        auto* samples = reinterpret_cast<float*>(output);
        const size_t count = size_t(bytes) / sizeof(float);
        self.restrain();
        for(auto& item:self.groups)if(item.second.fade.active)item.second.volume=item.second.fade.advance(count);
        self.refresh_gains();
        for (auto& item : self.channels) {
            auto& channel = item.second;
            if(channel.fade.active){channel.volume=channel.fade.advance(count);if(!channel.fade.active&&channel.fade_stop){channel.active=false;channel.cursor=0;if(channel.fade_erase){channel.samples.reset();channel.stream.reset();}else channel.volume=channel.saved_volume;self.refresh_gain(item.first,channel);continue;}}
            self.refresh_gain(item.first,channel);
            if(!channel.active&&channel.one_only){channel.samples.reset();if(!channel.stream||!channel.stream->failed.load())channel.stream.reset();}
            if(channel.active&&channel.stream){
                const float gain=self.pcm_gain(item.first,channel);
                std::array<float,1024> scratch;size_t written=0;
                try{while(written<count/2){size_t frames=std::min<size_t>(512,count/2-written);auto got=channel.stream->read(scratch.data(),frames,channel.stream->command_stream?channel.loop:channel.play_loop,!self.device||self.virtual_clock);for(size_t i=0;i<got*2;++i)samples[written*2+i]+=scratch[i]*gain;written+=got;if(!channel.stream->command_stream){channel.cursor=size_t(channel.stream->total_cursor*2);if(!channel.play_loop&&channel.stream->finished())channel.active=false;}if(got<frames){if(channel.stream->finished()){channel.active=false;if(channel.one_only&&!channel.stream->failed.load())channel.stream.reset();}break;}}}
                catch(const std::exception& e){channel.error=e.what();channel.active=false;}
                if(!channel.active&&channel.one_only&&channel.stream&&!channel.stream->failed.load())channel.stream.reset();
                continue;
            }
            if (!channel.active || !channel.samples || channel.samples->empty()) continue;
            const float gain=self.pcm_gain(item.first,channel);
            size_t written{};
            while (written < count && channel.active) {
                const size_t remaining = channel.samples->size() - channel.cursor;
                const size_t chunk = std::min(remaining, count - written);
                for (size_t i = 0; i < chunk; ++i)
                    samples[written + i] += (*channel.samples)[channel.cursor + i] * gain;
                written += chunk; channel.cursor += chunk;
                if (channel.cursor == channel.samples->size()) {
                    if (channel.play_loop) channel.cursor = 0;
                    else {channel.active = false;if(channel.one_only)channel.samples.reset();}
                }
            }
        }

        for (size_t i = 0; i < count; ++i) samples[i] = std::clamp(samples[i], -1.0f, 1.0f);
    }
};
Audio::Audio(bool open_device) : impl_(std::make_unique<Impl>(open_device)) {}
void Audio::render(float* samples,size_t frames){if(impl_->device)throw std::runtime_error("cannot manually render an active audio device");if(!samples||frames>size_t(std::numeric_limits<int>::max())/(2*sizeof(float)))throw std::runtime_error("invalid audio output buffer");Impl::callback(impl_.get(),reinterpret_cast<Uint8*>(samples),int(frames*2*sizeof(float)));}
void Audio::set_virtual_clock(bool enabled){
    if(impl_->device&&enabled)SDL_PauseAudioDevice(impl_->device,1);
    {DeviceLock lock(impl_->device);impl_->virtual_clock=enabled;}
    if(impl_->device&&!enabled)SDL_PauseAudioDevice(impl_->device,0);
}
void Audio::advance_virtual(uint32_t ms){
    DeviceLock lock(impl_->device);
    if(!impl_->virtual_clock)throw std::runtime_error("audio virtual clock is disabled");
    uint64_t frames=uint64_t(ms)*48;std::array<float,1024> scratch;
    while(frames){const auto count=std::min<uint64_t>(frames,512);Impl::callback(impl_.get(),reinterpret_cast<Uint8*>(scratch.data()),int(count*2*sizeof(float)));frames-=count;}
}
Audio::~Audio() = default;
void Audio::load(int id,const Bytes& bytes){load_sources(id,{ResourceSource::memory(std::make_shared<const Bytes>(bytes))},false,0);}
void Audio::load_parts(int id,const std::vector<Bytes>& parts){std::vector<ResourceSource> sources;for(const auto& bytes:parts)sources.push_back(ResourceSource::memory(std::make_shared<const Bytes>(bytes)));load_sources(id,sources,false,0);}
void Audio::load_stream(int id,const std::vector<Bytes>& parts,int32_t loop_start){std::vector<ResourceSource> sources;for(const auto& bytes:parts)sources.push_back(ResourceSource::memory(std::make_shared<const Bytes>(bytes)));load_sources(id,sources,true,loop_start);}
void Audio::load_sources(int id,const std::vector<ResourceSource>& parts,bool command_stream,int32_t loop_start){
    if(parts.empty()||parts.size()>1024)throw std::runtime_error("invalid audio stream fragment count");auto stream=std::make_shared<StreamState>();stream->parts=parts;stream->loop_start=loop_start;stream->command_stream=command_stream;
    for(const auto& source:parts){if(!source||!source.size()||source.size()>uint64_t(std::numeric_limits<sf_count_t>::max()))throw std::runtime_error("invalid audio stream source");StreamDecoder probe(source);auto length=probe.frames();if(length>UINT64_MAX/2-stream->logical_size())throw std::runtime_error("audio duration overflow");stream->lengths.push_back(length);}
    DeviceLock lock(impl_->device);for(auto it=impl_->channels.begin();it!=impl_->channels.end();)if(!it->second.samples&&!it->second.stream&&(it->second.one_only||it->second.fade_erase))it=impl_->channels.erase(it);else ++it;auto& channel=impl_->channels[id];channel=Impl::Channel{};channel.stream=std::move(stream);impl_->assign_group(id,channel);impl_->refresh_gain(id,channel);
}
bool Audio::streamed(int id)const{DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);return i!=impl_->channels.end()&&bool(i->second.stream)&&i->second.stream->command_stream;}
int32_t Audio::stream_loop_start(int id)const{DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);return i==impl_->channels.end()||!i->second.stream?0:i->second.stream->loop_start;}
void Audio::play(int id) { play(id,loop(id)); }
void Audio::play(int id,bool loop){
    std::shared_ptr<StreamState> source;{DeviceLock lock(impl_->device);auto it=impl_->channels.find(id);if(it==impl_->channels.end()||(!it->second.stream&&!it->second.samples))throw std::runtime_error("unloaded audio channel "+std::to_string(id));source=it->second.stream;}
    std::shared_ptr<StreamState> prepared;if(source){prepared=source->clone(0,0);prepared->configure(loop);impl_->worker.track(prepared);prepared->prime();}
    DeviceLock lock(impl_->device);auto& channel=impl_->channels.at(id);channel.error.clear();if(prepared)channel.stream=std::move(prepared);channel.cursor=0;channel.active=bool(channel.stream)||(channel.samples&&!channel.samples->empty());channel.loop=loop;channel.play_loop=loop;impl_->refresh_gain(id,channel);
}
void Audio::stop(int id) {
    DeviceLock lock(impl_->device);
    auto it = impl_->channels.find(id);
    if (it != impl_->channels.end()) { it->second.active = false; it->second.cursor = 0;it->second.fade={}; }
}
void Audio::stop_all() {
    DeviceLock lock(impl_->device);
    for (auto& item : impl_->channels) { item.second.active = false; item.second.cursor = 0;item.second.fade={};item.second.fade_stop=item.second.fade_erase=false; }
}
bool Audio::playing(int id) const {
    DeviceLock lock(impl_->device);
    const auto it = impl_->channels.find(id);
    if(it!=impl_->channels.end()&&it->second.stream)it->second.stream->check_error();
    if(it!=impl_->channels.end()&&!it->second.error.empty())throw std::runtime_error("audio stream "+std::to_string(id)+": "+it->second.error);
    return it != impl_->channels.end() && it->second.active;
}
uint64_t Audio::duration_ms(int id) const {
    DeviceLock lock(impl_->device);
    const auto it = impl_->channels.find(id);
    if (it == impl_->channels.end() || (!it->second.samples&&!it->second.stream))
        throw std::runtime_error("unloaded audio channel " + std::to_string(id));
    uint64_t frames = 0;if(it->second.stream){for(auto length:it->second.stream->lengths)frames+=length;}else frames=it->second.samples->size()/2;
    return (frames * 1000 + 47999) / 48000;
}
void Audio::set_volume(int id, int volume) {
    DeviceLock lock(impl_->device);
    auto [it, inserted] = impl_->channels.try_emplace(id);
    if (inserted) impl_->assign_group(id, it->second);
    it->second.volume = volume;it->second.fade={};impl_->refresh_gain(id,it->second);
}
void Audio::set_group(int group, int first, int last) {
    DeviceLock lock(impl_->device);
    auto& config = impl_->groups[group];
    config.first = first; config.last = last; config.assigned = true; config.assignment = ++impl_->group_assignment;
    for (auto& item : impl_->channels) impl_->assign_group(item.first, item.second);impl_->refresh_gains();
}
void Audio::set_group_volume(int group, int volume) {
    DeviceLock lock(impl_->device);
    auto it=impl_->groups.find(group);if(it!=impl_->groups.end()){it->second.volume = volume;it->second.fade={};}impl_->refresh_gains();
}
void Audio::set_one_only(int id,bool enabled){DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);if(i!=impl_->channels.end())i->second.one_only=enabled;}
bool Audio::one_only(int id)const{DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);return i!=impl_->channels.end()&&i->second.one_only;}
void Audio::erase(int id){DeviceLock lock(impl_->device);impl_->channels.erase(id);}
bool Audio::loaded(int id)const{DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);return i!=impl_->channels.end()&&(bool(i->second.samples)||bool(i->second.stream));}
void Audio::set_loop(int id,bool loop){DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);if(i!=impl_->channels.end())i->second.loop=loop;}
bool Audio::loop(int id)const{DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);return i!=impl_->channels.end()&&i->second.loop;}
int Audio::status(int id)const{DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);if(i!=impl_->channels.end()&&i->second.stream)i->second.stream->check_error();if(i!=impl_->channels.end()&&!i->second.error.empty())throw std::runtime_error("audio stream "+std::to_string(id)+": "+i->second.error);return i!=impl_->channels.end()&&i->second.active?(i->second.loop?2:1):0;}
int Audio::volume(int id)const{DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);return i==impl_->channels.end()?0:i->second.volume;}
int Audio::effective_volume(int id)const{DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);return i==impl_->channels.end()?0:impl_->gain(id,i->second);}
void Audio::fade_volume(int id,int volume,uint32_t ms){DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);if(i==impl_->channels.end())return;auto& c=i->second;c.fade={c.volume,volume,0,uint64_t(ms)*96,ms!=0};c.fade_stop=c.fade_erase=false;if(!ms)c.volume=volume;impl_->refresh_gain(id,c);}
void Audio::fade_stop(int id,uint32_t ms,bool erase){DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);if(i==impl_->channels.end())return;auto& c=i->second;if(!ms||!c.active){c.active=false;c.cursor=0;c.fade={};if(erase)impl_->channels.erase(i);return;}c.saved_volume=c.volume;c.fade={c.volume,0,0,uint64_t(ms)*96,true};c.fade_stop=true;c.fade_erase=erase;}
int Audio::group_volume(int group)const{DeviceLock lock(impl_->device);auto i=impl_->groups.find(group);return i==impl_->groups.end()?0:i->second.volume;}
void Audio::fade_group_volume(int group,int volume,uint32_t ms){DeviceLock lock(impl_->device);auto i=impl_->groups.find(group);if(i==impl_->groups.end())return;auto& g=i->second;g.fade={g.volume,volume,0,uint64_t(ms)*96,ms!=0&&!g.silent};if(!ms||g.silent)g.volume=volume;impl_->refresh_gains();}
void Audio::set_group_silent(int group,bool silent){DeviceLock lock(impl_->device);auto i=impl_->groups.find(group);if(i!=impl_->groups.end()){i->second.silent=silent;i->second.fade={};}impl_->refresh_gains();}
bool Audio::group_silent(int group)const{DeviceLock lock(impl_->device);auto i=impl_->groups.find(group);return i!=impl_->groups.end()&&i->second.silent;}
void Audio::set_silent(int id,bool silent){DeviceLock lock(impl_->device);auto [i,inserted]=impl_->groups.try_emplace(negative_id(id));if(inserted){i->second.first=id;i->second.last=id;i->second.assigned=true;i->second.assignment=++impl_->group_assignment;}i->second.silent=silent;i->second.fade={};impl_->refresh_gains();}
bool Audio::silent(int id)const{return group_silent(negative_id(id));}
void Audio::set_group_auto_skip(int group,int enabled){DeviceLock lock(impl_->device);auto i=impl_->groups.find(group);if(i!=impl_->groups.end())i->second.auto_skip=enabled;}
int Audio::group_auto_skip(int group)const{DeviceLock lock(impl_->device);auto i=impl_->groups.find(group);return i==impl_->groups.end()?0:i->second.auto_skip;}
bool Audio::auto_skip(int id)const{DeviceLock lock(impl_->device);for(const auto& item:impl_->groups){const auto& g=item.second;if(g.assigned&&g.first<=id&&id<=g.last&&(g.auto_skip==1||(g.auto_skip&2)))return true;}return false;}
bool Audio::voice_wait_pending()const{
    DeviceLock lock(impl_->device);
    for(const auto& [id,channel]:impl_->channels){
        bool silent=false,wait=false;
        for(const auto& [group_id,g]:impl_->groups){(void)group_id;if(g.assigned&&g.first<=id&&id<=g.last){silent|=g.silent;wait|=g.auto_skip==1||(g.auto_skip&4);}}
        if(!silent&&wait&&channel.active&&!channel.loop){if(!channel.error.empty())throw std::runtime_error("audio stream "+std::to_string(id)+": "+channel.error);return true;}
    }
    return false;
}
void Audio::clear(){DeviceLock lock(impl_->device);impl_->channels.clear();impl_->groups.clear();impl_->group_assignment=0;impl_->restrain_level=1000;impl_->history_groups.clear();impl_->restrain_groups.clear();}
AudioDecodeStats Audio::decode_stats()const{
    DeviceLock lock(impl_->device);AudioDecodeStats stats;for(const auto& item:impl_->channels)if(item.second.stream){auto& stream=*item.second.stream;std::lock_guard<std::mutex> state(stream.mutex);++stats.sources;stats.pcm_capacity_bytes+=sizeof(float)*2*StreamState::chunk_frames*StreamState::chunk_count;stats.decoded_frames+=stream.decoded_frames;stats.underruns+=stream.underruns.load();for(size_t n=0;n<stream.queued;++n){const auto& chunk=stream.chunks[(stream.head+n)%StreamState::chunk_count];stats.queued_frames+=chunk.count-chunk.used;}}return stats;
}
Bytes Audio::snapshot()const{
    DeviceLock lock(impl_->device);if(impl_->channels.size()>65536||impl_->groups.size()>65536)throw std::runtime_error("audio snapshot object limit exceeded");Bytes out={'Y','U','K','A','U','D','I','O'};
    auto write=[&](uint64_t value,unsigned bytes=4){for(unsigned i=0;i<bytes;++i)out.push_back(uint8_t(value>>(8*i)));};
    auto fade=[&](const Impl::Fade& f){write(uint32_t(f.from));write(uint32_t(f.target));write(f.elapsed,8);write(f.length,8);write(f.active,1);};
    write(6);write(impl_->channels.size());
    for(const auto& item:impl_->channels){const auto& c=item.second;write(uint32_t(item.first));write(c.samples?c.samples->size():0,8);write(c.samples?c.cursor:0,8);write(uint32_t(c.volume));write(uint32_t(c.saved_volume));
        write(unsigned(c.active)|(unsigned(c.loop)<<1)|(unsigned(c.silent)<<2)|(unsigned(c.fade_stop)<<3)|(unsigned(c.fade_erase)<<4)|(unsigned(c.one_only)<<5)|(unsigned(c.play_loop)<<6)|(unsigned(bool(c.stream))<<7),1);fade(c.fade);write(uint32_t(c.attenuation));
        if(c.stream){write(c.stream->parts.size());for(size_t i=0;i<c.stream->parts.size();++i){write(c.stream->parts[i].size(),8);write(c.stream->lengths[i],8);}write(c.stream->segment);write(c.stream->cursor,8);write(uint32_t(c.stream->loop_start));write(c.stream->command_stream,1);}}
    write(impl_->groups.size());
    for(const auto& item:impl_->groups){const auto& g=item.second;write(uint32_t(item.first));write(uint32_t(g.first));write(uint32_t(g.last));write(uint32_t(g.volume));write(uint32_t(g.auto_skip));write(g.assignment,8);write(unsigned(g.assigned)|(unsigned(g.silent)<<1),1);fade(g.fade);}
    write(impl_->group_assignment,8);write(uint32_t(impl_->restrain_level));write(impl_->history_groups.size());for(int id:impl_->history_groups)write(uint32_t(id));write(impl_->restrain_groups.size());for(int id:impl_->restrain_groups)write(uint32_t(id));return out;
}
std::vector<AudioSnapshotSource> Audio::sources_from_snapshot(const Bytes& data){
    if(data.size()<20||data.size()>16*1024*1024||std::memcmp(data.data(),"YUKAUDIO",8))throw std::runtime_error("invalid audio snapshot");
    size_t at=8;auto read=[&](unsigned n=4){if(n>data.size()-at)throw std::runtime_error("truncated audio source metadata");uint64_t value=0;for(unsigned i=0;i<n;++i)value|=uint64_t(data[at++])<<(8*i);return value;};
    auto integer=[&](){auto value=uint32_t(read());int32_t n;std::memcpy(&n,&value,4);return n;};
    const auto version=read();if(version<1||version>6)throw std::runtime_error("unsupported audio snapshot version");
    auto count=read();if(count>65536)throw std::runtime_error("excessive audio sources");std::vector<AudioSnapshotSource> result;
    while(count--){const auto id=integer();const auto size=read(8);read(8);read(4);read(4);const auto flags=read(1);for(unsigned n:{4u,4u,8u,8u,1u})read(n);if(version>=5)read(4);bool stream=version>=4&&(flags&128);int32_t start=0;
        if(stream){auto parts=read();if(!parts||parts>1024)throw std::runtime_error("invalid audio stream fragments");while(parts--){read(8);read(8);}read(4);read(8);start=integer();if(version>=6){auto command=read(1);if(command>1)throw std::runtime_error("invalid audio stream command flag");stream=command!=0;}}
        if(size||(version>=4&&(flags&128)))result.push_back({id,stream,start});}
    return result;
}
void Audio::restore_playback(const Bytes& data){
    if(data.size()<20||data.size()>16*1024*1024||std::memcmp(data.data(),"YUKAUDIO",8))throw std::runtime_error("invalid audio snapshot");
    std::map<int,Impl::Channel> loaded;{DeviceLock lock(impl_->device);loaded=impl_->channels;}size_t at=8;
    auto read=[&](unsigned bytes=4){if(bytes>data.size()-at)throw std::runtime_error("truncated audio snapshot");uint64_t n=0;for(unsigned i=0;i<bytes;++i)n|=uint64_t(data[at++])<<(i*8);return n;};
    auto integer=[&](){auto u=uint32_t(read());int32_t n;std::memcpy(&n,&u,4);return n;};
    auto fade=[&](){Impl::Fade f;f.from=integer();f.target=integer();f.elapsed=read(8);f.length=read(8);auto active=read(1);if(active>1||f.elapsed>f.length||(active&&(!f.length||f.elapsed==f.length)))throw std::runtime_error("invalid audio fade snapshot");f.active=active!=0;return f;};
    const auto version=read();if(version<1||version>6)throw std::runtime_error("unsupported audio snapshot version");
    std::map<int,Impl::Channel> channels;auto count=read();if(count>65536)throw std::runtime_error("excessive audio snapshot channels");
    for(uint64_t i=0;i<count;++i){int id=integer();Impl::Channel c;const auto size=read(8);const auto cursor=read(8);c.volume=integer();c.saved_volume=integer();const auto flags=read(1);if(flags&~uint64_t(version>=4?255:version>=3?127:63))throw std::runtime_error("invalid audio channel flags");
        c.active=flags&1;c.loop=flags&2;c.silent=flags&4;c.fade_stop=flags&8;c.fade_erase=flags&16;c.one_only=flags&32;c.play_loop=version>=3?bool(flags&64):c.loop;c.fade=fade();if(version>=5){c.attenuation=integer();if(!native_sound_attenuation_valid(c.attenuation))throw std::runtime_error("invalid audio attenuation snapshot");}
        if(version>=4&&(flags&128)){
            auto old=loaded.find(id);if(old==loaded.end()||!old->second.stream)throw std::runtime_error("audio snapshot stream missing: "+std::to_string(id));
            const auto& source=*old->second.stream;const auto parts=read();if(parts!=source.parts.size()||parts>1024)throw std::runtime_error("audio snapshot stream fragments changed");
            for(size_t p=0;p<parts;++p)if(read(8)!=source.parts[p].size()||read(8)!=source.lengths[p])throw std::runtime_error("audio snapshot stream source changed");
            const auto segment=uint32_t(read());const auto frame=read(8);const auto start=integer();c.stream=source.clone(segment,frame);c.stream->loop_start=start;if(version>=6){auto command=read(1);if(command>1)throw std::runtime_error("invalid audio stream command flag");c.stream->command_stream=command!=0;}else c.stream->command_stream=true;
        }
        if(size){auto old=loaded.find(id);if(old==loaded.end())throw std::runtime_error("audio snapshot source missing: "+std::to_string(id));
            if(old->second.stream&&!old->second.stream->command_stream){auto& base=*old->second.stream;auto predicted=base.logical_size()*2;if((size>predicted?size-predicted:predicted-size)>8192*base.parts.size())throw std::runtime_error("audio snapshot source length changed");c.stream=base.clone_total(cursor/2);c.stream->configure(c.play_loop);}
            else{if(!old->second.samples||old->second.samples->size()!=size)throw std::runtime_error("audio snapshot clip missing or changed: "+std::to_string(id));c.samples=old->second.samples;}}
        if(c.stream){c.stream->configure(c.stream->command_stream?c.loop:c.play_loop);impl_->worker.track(c.stream);c.stream->prime();}

        if(cursor>size||(cursor&1)||(c.active&&!c.stream&&(!size||cursor==size)))throw std::runtime_error("invalid audio playback cursor");c.cursor=size_t(cursor);
        if(!channels.emplace(id,std::move(c)).second)throw std::runtime_error("duplicate audio snapshot channel");}
    std::map<int,Impl::Group> groups;count=read();if(count>65536)throw std::runtime_error("excessive audio snapshot groups");
    uint64_t newest=0;
    for(uint64_t i=0;i<count;++i){int id=integer();Impl::Group g;g.first=integer();g.last=integer();g.volume=integer();g.auto_skip=integer();g.assignment=read(8);const auto flags=read(1);if(flags&~uint64_t(3))throw std::runtime_error("invalid audio group flags");g.assigned=flags&1;g.silent=flags&2;g.fade=fade();
        newest=std::max(newest,g.assignment);if(!groups.emplace(id,g).second)throw std::runtime_error("duplicate audio snapshot group");}
    const auto assignment=read(8);int level=1000;std::vector<int> history,targets;
    if(version>=2){level=integer();auto n=read();if(n>65536)throw std::runtime_error("excessive audio history groups");while(n--)history.push_back(integer());n=read();if(n>65536)throw std::runtime_error("excessive audio restrain groups");while(n--)targets.push_back(integer());}
    if(at!=data.size()||assignment<newest)throw std::runtime_error("invalid audio snapshot suffix");
    DeviceLock commit(impl_->device);impl_->channels=std::move(channels);impl_->groups=std::move(groups);impl_->group_assignment=assignment;impl_->restrain_level=level;impl_->history_groups=std::move(history);impl_->restrain_groups=std::move(targets);
    for(auto& item:impl_->channels)impl_->assign_group(item.first,item.second);
    if(version<5)impl_->refresh_gains();
}
void Audio::set_history_groups(const std::vector<int>& groups){if(groups.size()>65536)throw std::runtime_error("excessive audio history groups");DeviceLock lock(impl_->device);impl_->history_groups=groups;}
void Audio::add_history_groups(const std::vector<int>& groups){DeviceLock lock(impl_->device);if(groups.size()>65536-impl_->history_groups.size())throw std::runtime_error("excessive audio history groups");impl_->history_groups.insert(impl_->history_groups.end(),groups.begin(),groups.end());}
void Audio::set_restrain_level(int volume){DeviceLock lock(impl_->device);impl_->restrain_level=volume;}
void Audio::set_restrain_groups(const std::vector<int>& groups){if(groups.size()>65536)throw std::runtime_error("excessive audio restrain groups");DeviceLock lock(impl_->device);impl_->restrain_groups=groups;}
void Audio::set_volume_type(bool logarithmic){DeviceLock lock(impl_->device);impl_->logarithmic_volume=logarithmic;impl_->refresh_gains();}
bool Audio::volume_type()const{DeviceLock lock(impl_->device);return impl_->logarithmic_volume;}
int Audio::applied_attenuation(int id)const{DeviceLock lock(impl_->device);auto i=impl_->channels.find(id);return i==impl_->channels.end()?0:i->second.attenuation;}
void Audio::set_muted(bool muted) { DeviceLock lock(impl_->device); impl_->muted = muted; }
}

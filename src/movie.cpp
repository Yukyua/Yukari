#include "yukari/movie.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <stdexcept>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
namespace yukari {
namespace {
constexpr size_t max_video_bytes=256*1024*1024;
constexpr size_t max_audio_bytes=32*1024*1024;
std::runtime_error failure(const char* operation,int code) {
    char detail[AV_ERROR_MAX_STRING_SIZE]{};av_strerror(code,detail,sizeof(detail));
    return std::runtime_error(std::string("movie ")+operation+": "+detail);
}
void checked(int result,const char* operation){if(result<0)throw failure(operation,result);}
}
struct Movie::Impl {
    AVFormatContext* format{};
    AVCodecContext *video{},*audio{};
    AVPacket* packet{};
    AVFrame* decoded{};
    SwsContext* scaler{};
    SwrContext* resampler{};
    int video_stream{-1},audio_stream{-1};
    MovieInfo information;
    std::deque<MovieFrame> pending;
    std::optional<MovieFrame> displayed;
    std::vector<MovieAudio> audio_chunks;
    uint64_t origin_us{},last_position{},video_count{},audio_count{},video_end{},audio_end{};
    uint64_t fallback_video{},fallback_audio{};
    size_t pending_bytes{},audio_bytes{};
    bool drained{},advanced{};
    ~Impl(){
        swr_free(&resampler);sws_freeContext(scaler);av_frame_free(&decoded);av_packet_free(&packet);
        avcodec_free_context(&video);avcodec_free_context(&audio);avformat_close_input(&format);
    }
    AVCodecContext* open_codec(int index) {
        const auto* params=format->streams[index]->codecpar;
        auto* codec=avcodec_find_decoder(params->codec_id);
        if(!codec)throw std::runtime_error("movie codec is unavailable");
        AVCodecContext* context=avcodec_alloc_context3(codec);
        if(!context)throw std::runtime_error("movie codec allocation failed");
        try{checked(avcodec_parameters_to_context(context,params),"codec parameters");checked(avcodec_open2(context,codec,nullptr),"open codec");}
        catch(...){avcodec_free_context(&context);throw;}
        return context;
    }
    void open(const std::filesystem::path& path) {
        if(!std::filesystem::is_regular_file(path))throw std::runtime_error("movie file not found: "+path.string());
        checked(avformat_open_input(&format,path.c_str(),nullptr,nullptr),"open input");
        checked(avformat_find_stream_info(format,nullptr),"stream information");
        video_stream=av_find_best_stream(format,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
        if(video_stream<0)throw failure("no video stream",video_stream);
        video=open_codec(video_stream);
        if(video->width<=0||video->height<=0||video->width>8192||video->height>8192||
           uint64_t(video->width)*video->height*4>max_video_bytes)throw std::runtime_error("movie video dimensions exceed decoder limit");
        information.width=video->width;information.height=video->height;
        auto rate=av_guess_frame_rate(format,format->streams[video_stream],nullptr);
        information.frame_rate=rate.num&&rate.den?av_q2d(rate):25;
        if(information.frame_rate<=0||!std::isfinite(information.frame_rate))information.frame_rate=25;
        if(format->duration>0)information.duration_ms=uint64_t(format->duration/1000);
        if(format->start_time!=AV_NOPTS_VALUE&&format->start_time>0)origin_us=uint64_t(format->start_time);
        audio_stream=av_find_best_stream(format,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0);
        if(audio_stream>=0){
            audio=open_codec(audio_stream);information.has_audio=true;
            if(audio->sample_rate<=0||audio->ch_layout.nb_channels<=0)throw std::runtime_error("movie audio format is invalid");
            AVChannelLayout stereo=AV_CHANNEL_LAYOUT_STEREO;
            checked(swr_alloc_set_opts2(&resampler,&stereo,AV_SAMPLE_FMT_FLT,48000,&audio->ch_layout,audio->sample_fmt,audio->sample_rate,0,nullptr),"audio resampler allocation");
            checked(swr_init(resampler),"audio resampler initialization");
        }
        packet=av_packet_alloc();decoded=av_frame_alloc();
        if(!packet||!decoded)throw std::runtime_error("movie packet/frame allocation failed");
    }
    uint64_t position(AVFrame* frame,int stream,uint64_t fallback) const {
        auto stamp=frame->best_effort_timestamp;
        if(stamp==AV_NOPTS_VALUE)return fallback;
        int64_t us=av_rescale_q(stamp,format->streams[stream]->time_base,AV_TIME_BASE_Q);
        return us<=int64_t(origin_us)?0:uint64_t(us-int64_t(origin_us))/1000;
    }
    void video_frame() {
        if(decoded->width!=information.width||decoded->height!=information.height)throw std::runtime_error("movie resolution changed during playback");
        MovieFrame frame;frame.width=decoded->width;frame.height=decoded->height;
        frame.position_ms=position(decoded,video_stream,fallback_video);
        frame.rgba.resize(size_t(frame.width)*frame.height*4);
        if(frame.rgba.size()>max_video_bytes-pending_bytes)throw std::runtime_error("movie queued video exceeds memory limit");
        scaler=sws_getCachedContext(scaler,frame.width,frame.height,AVPixelFormat(decoded->format),frame.width,frame.height,AV_PIX_FMT_RGBA,SWS_BILINEAR,nullptr,nullptr,nullptr);
        if(!scaler)throw std::runtime_error("movie pixel conversion allocation failed");
        uint8_t* output[]={frame.rgba.data(),nullptr,nullptr,nullptr};int stride[]={frame.width*4,0,0,0};
        int rows=sws_scale(scaler,decoded->data,decoded->linesize,0,frame.height,output,stride);
        if(rows!=frame.height)throw std::runtime_error("movie pixel conversion returned incomplete frame");
        uint64_t duration=std::max<uint64_t>(1,uint64_t(1000/information.frame_rate));
        fallback_video=frame.position_ms+duration;video_end=std::max(video_end,fallback_video);
        pending_bytes+=frame.rgba.size();pending.push_back(std::move(frame));++video_count;
    }
    void push_audio(std::vector<float> samples,uint64_t stamp) {
        if(samples.empty())return;
        if(samples.size()*sizeof(float)>max_audio_bytes-audio_bytes)throw std::runtime_error("movie audio must be consumed before decoding further");
        audio_bytes+=samples.size()*sizeof(float);fallback_audio=stamp+samples.size()/2*1000/48000;
        audio_end=std::max(audio_end,fallback_audio);audio_chunks.push_back({stamp,std::move(samples)});
    }
    void audio_frame() {
        const int64_t delay=swr_get_delay(resampler,audio->sample_rate);
        const int capacity=swr_get_out_samples(resampler,decoded->nb_samples);
        if(capacity<0||size_t(capacity)>max_audio_bytes/(2*sizeof(float)))throw std::runtime_error("movie audio frame exceeds memory limit");
        std::vector<float> samples(size_t(capacity)*2);
        uint8_t* output[]={reinterpret_cast<uint8_t*>(samples.data())};
        const int count=swr_convert(resampler,output,capacity,const_cast<const uint8_t**>(decoded->extended_data),decoded->nb_samples);
        checked(count,"audio conversion");samples.resize(size_t(count)*2);
        uint64_t stamp=position(decoded,audio_stream,fallback_audio);
        // Timestamp the first output sample, accounting for retained resampler input.
        uint64_t delay_ms=uint64_t(std::max<int64_t>(0,delay))*1000/uint64_t(audio->sample_rate);
        stamp=stamp>delay_ms?stamp-delay_ms:0;
        push_audio(std::move(samples),stamp);++audio_count;
    }
    void receive(AVCodecContext* codec,bool is_video) {
        for(;;){
            int result=avcodec_receive_frame(codec,decoded);
            if(result==AVERROR(EAGAIN)||result==AVERROR_EOF)return;
            checked(result,"receive frame");
            try{if(is_video)video_frame();else audio_frame();}catch(...){av_frame_unref(decoded);throw;}
            av_frame_unref(decoded);
        }
    }
    void send(AVCodecContext* codec,AVPacket* input,bool is_video) {
        int result=avcodec_send_packet(codec,input);
        if(result==AVERROR(EAGAIN)){receive(codec,is_video);result=avcodec_send_packet(codec,input);}
        if(result!=AVERROR_EOF)checked(result,"send packet");
        receive(codec,is_video);
    }
    void read_packet() {
        int result=av_read_frame(format,packet);
        if(result==AVERROR_EOF){
            send(video,nullptr,true);if(audio)send(audio,nullptr,false);
            if(resampler){
                for(;;){int capacity=swr_get_out_samples(resampler,0);if(capacity<=0)break;
                    if(size_t(capacity)>max_audio_bytes/(2*sizeof(float)))throw std::runtime_error("movie resampler tail exceeds limit");
                    std::vector<float> samples(size_t(capacity)*2);uint8_t* output[]={reinterpret_cast<uint8_t*>(samples.data())};
                    int count=swr_convert(resampler,output,capacity,nullptr,0);checked(count,"resampler flush");if(!count)break;
                    samples.resize(size_t(count)*2);push_audio(std::move(samples),fallback_audio);}
            }
            drained=true;return;
        }
        checked(result,"read packet");
        try{if(packet->size>64*1024*1024)throw std::runtime_error("movie packet exceeds memory limit");
            if(packet->stream_index==video_stream)send(video,packet,true);
            else if(packet->stream_index==audio_stream)send(audio,packet,false);
        }catch(...){av_packet_unref(packet);throw;}
        av_packet_unref(packet);
    }
};
Movie::Movie(const std::filesystem::path& path):impl_(std::make_unique<Impl>()){impl_->open(path);}
Movie::~Movie()=default;
const MovieInfo& Movie::info() const{return impl_->information;}
bool Movie::advance(uint64_t position_ms){
    auto& p=*impl_;
    if(p.advanced&&position_ms<p.last_position)throw std::runtime_error("movie playback clock moved backwards");
    p.advanced=true;p.last_position=position_ms;bool changed=false;
    for(;;){
        if(!p.pending.empty()){
            if(p.pending.front().position_ms>position_ms){
                // MPEG program streams can place audio packets behind several
                // future video packets. Read through that interleave so SDL's
                // audio queue does not starve while video advances normally.
                if(p.audio&&!p.drained&&p.audio_end<position_ms+250){p.read_packet();continue;}
                return changed;
            }
            p.pending_bytes-=p.pending.front().rgba.size();p.displayed=std::move(p.pending.front());p.pending.pop_front();changed=true;
        }else if(p.drained)return changed;
        else p.read_packet();
    }
}
const MovieFrame* Movie::frame() const{return impl_->displayed?&*impl_->displayed:nullptr;}
std::vector<MovieAudio> Movie::take_audio(){auto result=std::move(impl_->audio_chunks);impl_->audio_chunks.clear();impl_->audio_bytes=0;return result;}
bool Movie::finished(uint64_t position_ms) const{return impl_->drained&&impl_->pending.empty()&&position_ms>=std::max(impl_->video_end,impl_->audio_end);}
uint64_t Movie::decoded_video_frames()const{return impl_->video_count;}
uint64_t Movie::decoded_audio_frames()const{return impl_->audio_count;}
}

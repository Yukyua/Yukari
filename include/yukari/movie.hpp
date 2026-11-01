#pragma once
#include "yukari/formats.hpp"
#include <memory>

namespace yukari {
struct MovieInfo {
    int width{},height{},audio_rate{48000},audio_channels{2};
    double frame_rate{};
    uint64_t duration_ms{};
    bool has_audio{};
};
struct MovieFrame {
    uint64_t position_ms{};
    int width{},height{};
    Bytes rgba;
};
struct MovieAudio {
    uint64_t position_ms{};
    // Interleaved IEEE float stereo, 48000 Hz. The caller queues it to SDL and
    // applies movie volume/mute without changing the decoder's source clock.
    std::vector<float> samples;
};
// Streaming local-file movie decoder. No SDL/UI side effects. A runtime starts
// its playback clock on open, calls advance(elapsed_ms), displays frame(), and
// consumes take_audio(). Packet demux/decode is bounded rather than preloading
// an entire movie. Unknown codecs and invalid files fail explicitly.
class Movie {
public:
    explicit Movie(const std::filesystem::path& path);
    ~Movie();
    Movie(const Movie&)=delete;
    Movie& operator=(const Movie&)=delete;
    const MovieInfo& info() const;
    // Monotonic playback positions only. True when a new frame was presented.
    bool advance(uint64_t position_ms);
    const MovieFrame* frame() const;
    std::vector<MovieAudio> take_audio();
    bool finished(uint64_t position_ms) const;
    uint64_t decoded_video_frames() const;
    uint64_t decoded_audio_frames() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}

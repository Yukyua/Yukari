#pragma once
#include "yukari/formats.hpp"
#include <memory>

namespace yukari {
struct AudioDecodeStats {uint64_t sources{},pcm_capacity_bytes{},queued_frames{},decoded_frames{},underruns{};};
struct AudioSnapshotSource { int id; bool streamed; int32_t loop_start; };
// All sources decode incrementally on a worker into bounded PCM queues.
// Native SoundLoad and SoundLoadStream retain their distinct loop/save semantics.
class Audio {
public:
    explicit Audio(bool open_device=true);
    // Offline mixer uses the same callback; production devices mix themselves.
    void render(float* interleaved_stereo,size_t frames);
    void set_virtual_clock(bool enabled);
    void advance_virtual(uint32_t milliseconds);
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;
    void load(int id, const Bytes& bytes);
    void load_parts(int id,const std::vector<Bytes>& parts);
    // TAB-separated source fragments are decoded incrementally; loop_start is
    // a fragment index, matching native SoundLoadStream third argument.
    void load_stream(int id,const std::vector<Bytes>& parts,int32_t loop_start=0);
    void load_sources(int id,const std::vector<ResourceSource>& sources,bool command_stream=false,int32_t loop_start=0);
    bool streamed(int id) const; // Native SoundLoadStream contract, not physical buffering.
    int32_t stream_loop_start(int id) const;
    void clear();
    // PCM stays in resource files. Restore requires the saved clips preloaded.
    AudioDecodeStats decode_stats()const;
    Bytes snapshot() const;
    static std::vector<AudioSnapshotSource> sources_from_snapshot(const Bytes& state);
    void restore_playback(const Bytes& state);
    void play(int id);
    void play(int id, bool loop);
    void erase(int id);
    // Native transient/no-save bit; completed one-only sounds release their clip.
    void set_one_only(int id,bool enabled=true);
    bool one_only(int id) const;
    bool loaded(int id) const;
    void set_loop(int id, bool loop);
    bool loop(int id) const;
    int status(int id) const;
    int volume(int id) const;
    int effective_volume(int id) const;
    void fade_volume(int id,int volume,uint32_t milliseconds);
    void fade_stop(int id,uint32_t milliseconds,bool erase=false);
    void stop(int id);
    void stop_all();
    bool playing(int id) const;
    uint64_t duration_ms(int id) const;
    void set_volume(int id, int volume);
    // Inclusive ranges. Every matching group contributes gain and silence;
    // native integer gain is truncated after each group multiplication.
    void set_group(int group, int first, int last);
    void set_group_volume(int group, int volume);
    int group_volume(int group) const;
    void fade_group_volume(int group,int volume,uint32_t milliseconds);
    void set_group_silent(int group,bool silent);
    bool group_silent(int group) const;
    void set_silent(int id,bool silent);
    bool silent(int id) const;
    void set_group_auto_skip(int group,int enabled);
    int group_auto_skip(int group) const;
    bool auto_skip(int id) const;
    // Native45dcd0: active nonlooping source in a group with flag1 or bit4,
    // provided no matching group is marked silent. Volume/mute do not gate it.
    bool voice_wait_pending() const;
    void set_history_groups(const std::vector<int>& groups);
    void add_history_groups(const std::vector<int>& groups);
    void set_restrain_level(int volume);
    void set_restrain_groups(const std::vector<int>& groups);
    // External INI setting; defaults false like native BSS and survives clear/restore.
    void set_volume_type(bool logarithmic);
    bool volume_type() const;
    int applied_attenuation(int id) const;
    void set_muted(bool muted);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}

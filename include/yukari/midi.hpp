#pragma once
#include "yukari/formats.hpp"
#include <memory>
namespace yukari {
// macOS Standard MIDI File player. Parsing never opens an output device;
// playback uses AudioToolbox's default GM/GS software synthesizer.
class Midi {
public:
    Midi();
    ~Midi();
    Midi(const Midi&)=delete;
    Midi& operator=(const Midi&)=delete;
    void load(const Bytes& bytes);
    void play();
    void stop();
    // Original MCI_NOTIFY completion reopens/replays the MIDI indefinitely.
    void set_loop(bool loop);
    bool loaded() const;
    bool playing() const;
    uint32_t tracks() const;
    uint64_t duration_ms() const;
    uint64_t position_ms() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}

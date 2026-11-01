#pragma once
#include "yukari/audio.hpp"
#include "yukari/game_part.hpp"
#include "yukari/render.hpp"
#include <array>
#include <functional>
#include <memory>

namespace yukari {
// The dedicated original GamePart scene. Caller owns SDL events/presentation
// and restores the parent Scene/Audio state after finished(). Missing assets
// are errors; the game distribution has no dedicated game assets.
class MusicGameScene {
public:
    using ResourceReader=std::function<Bytes(const std::string&)>;
    enum class Phase { Idle,ToBlack,BackgroundFade,CharactersFade,WaitIn,Waiting,WaitOut,Playing,GoodFade,BadBounce,ResultHold,FinalBlack,Finished };
    MusicGameScene(Scene& scene,Audio& audio,ResourceReader reader);
    ~MusicGameScene();
    MusicGameScene(const MusicGameScene&)=delete;
    MusicGameScene& operator=(const MusicGameScene&)=delete;
    void set_audio_reader(std::function<ResourceSource(const std::string&)> reader);
    void begin(const std::string& chart,const MusicGameOptions& options={},uint64_t now=0);
    // One host iteration. Gameplay preserves native one-note/lane processing;
    // the caller should schedule approximately every 10ms rather than catch up.
    void tick(uint64_t now,const std::array<bool,5>& keys={},bool abort=false);
    void confirm(); // Full-window click, Enter or other host confirmation.
    void render(); // Backbuffer only; caller presents.
    bool finished()const;
    uint32_t result()const; // Hit count, never a success boolean.
    bool successful()const;
    Phase phase()const;
    // Helper-effective duration and native quality/timing, after 425f70's
    // duration scaling. quality<0 suppresses fades as in that wrapper.
    void set_transition_sampling(TransitionSampling sampling);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}

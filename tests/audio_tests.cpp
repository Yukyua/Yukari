#include "yukari/audio.hpp"
#include "yukari/audio_gain.hpp"
#include "yukari/audio_commands.hpp"
#include <cmath>
#include <SDL.h>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void put16(yukari::Bytes& bytes, size_t at, uint16_t value) {
    bytes[at] = uint8_t(value); bytes[at+1] = uint8_t(value >> 8);
}
void put32(yukari::Bytes& bytes, size_t at, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes[at+i] = uint8_t(value >> (8*i));
}
yukari::Bytes wav() {
    // Approximately 50 ms of 22.05 kHz mono PCM16, generated in memory.
    constexpr uint32_t rate = 22050, frames = rate / 20;
    yukari::Bytes bytes(44 + frames*2);
    const auto text = [&](size_t at, const char* str) {
        for (unsigned i = 0; i < 4; ++i) bytes[at+i] = uint8_t(str[i]);
    };
    text(0, "RIFF"); put32(bytes, 4, uint32_t(bytes.size()-8)); text(8, "WAVE");
    text(12, "fmt "); put32(bytes, 16, 16); put16(bytes, 20, 1); put16(bytes, 22, 1);
    put32(bytes, 24, rate); put32(bytes, 28, rate*2); put16(bytes, 32, 2); put16(bytes, 34, 16);
    text(36, "data"); put32(bytes, 40, frames*2);
    for (uint32_t i = 0; i < frames; ++i)
        put16(bytes, 44+i*2, uint16_t(i % 40 < 20 ? 4096 : -4096));
    return bytes;
}
}

int main() {
    try {
        require(SDL_setenv("SDL_AUDIODRIVER", "dummy", 1) == 0, "cannot select dummy audio driver");
        yukari::Audio audio(false);audio.set_volume_type(true);
        const auto clip = wav();
        audio.set_group(1, 10, 19); audio.set_group_volume(1, 500);
        audio.set_volume(10, 750); audio.load(10, clip);
        require(audio.duration_ms(10) >= 49 && audio.duration_ms(10) <= 51, "resampled WAV duration is incorrect");
        require(!audio.playing(10), "loaded sound must initially be stopped");
        audio.play(10);
        require(audio.playing(10), "play must activate the channel");
        {std::vector<float> pcm(48000*2*150/1000);audio.render(pcm.data(),pcm.size()/2);}
        require(!audio.playing(10), "audio callback must finish non-looping playback");
        audio.play(10, true);
        {std::vector<float> pcm(48000*2*150/1000);audio.render(pcm.data(),pcm.size()/2);}
        require(audio.playing(10), "looping playback must survive its first duration");
        audio.stop(10);
        require(!audio.playing(10), "stop must deactivate the channel");
        audio.play(10, true); audio.load(10, clip);
        require(!audio.playing(10), "reloading a playing channel must stop it");
        audio.set_muted(true); audio.set_group_volume(1, 1000); audio.set_volume(10, 500);
        audio.play(10, true); audio.stop_all(); audio.set_muted(false);
        require(!audio.playing(10), "stop_all must stop the channel");
        auto mix=[&](unsigned ms){std::vector<float> pcm(size_t(ms)*48*2);audio.render(pcm.data(),pcm.size()/2);return pcm;};
        audio.load(10,clip);audio.set_volume(10,800);audio.set_loop(10,true);audio.play(10);
        require(audio.status(10)==2&&audio.loop(10),"play must preserve configured loop and expose native status 2");
        audio.set_group(2,10,10);audio.set_group_volume(1,500);audio.set_group_volume(2,500);
        auto pcm=mix(1);require(std::abs(pcm[20])>0.001f&&std::abs(pcm[20])<0.04f,"overlapping group gain must affect mixed samples");
        audio.set_group_silent(1,true);pcm=mix(1);for(float x:pcm)require(x==0,"any matching silent group must mute");
        audio.set_group_silent(1,false);audio.set_silent(10,true);require(audio.silent(10),"individual silence state missing");pcm=mix(1);for(float x:pcm)require(x==0,"individual mute must silence mixed samples");audio.set_silent(10,false);
        audio.set_group_auto_skip(1,4);audio.set_group_auto_skip(2,0);require(!audio.auto_skip(10),"auto skip bit 4 alone must not enable suppression");audio.set_group_auto_skip(2,2);require(audio.auto_skip(10),"auto skip bit 2 must apply across matching ranges");
        audio.play(10,false);require(audio.voice_wait_pending(),"voice wait must accept group bit4 independently of bit2 skip suppression");
        audio.set_group_silent(2,true);require(!audio.voice_wait_pending(),"any overlapping silent group must suppress voice wait");audio.set_group_silent(2,false);
        audio.set_silent(10,true);require(!audio.voice_wait_pending(),"individual silence must suppress voice wait");audio.set_silent(10,false);
        audio.set_loop(10,true);require(!audio.voice_wait_pending(),"looping background source must not block voice wait");audio.set_loop(10,false);
        audio.set_group_auto_skip(1,0);require(!audio.voice_wait_pending(),"bit2-only group must not block voice wait");audio.set_group_auto_skip(1,1);require(audio.voice_wait_pending(),"flag1 must enable both voice wait and auto skip");
        audio.set_volume(10,0);audio.set_muted(true);require(audio.voice_wait_pending(),"zero volume or host mute is not a native silent-group flag");audio.set_muted(false);audio.stop(10);require(!audio.voice_wait_pending(),"stopped source must release voice wait");
        audio.set_volume(10,800);audio.play(10,true);
        audio.fade_volume(10,200,10);mix(5);require(audio.volume(10)==500,"channel fade midpoint incorrect");mix(5);require(audio.volume(10)==200,"channel fade endpoint incorrect");
        audio.fade_stop(10,10);mix(10);require(!audio.playing(10)&&audio.volume(10)==200&&audio.loaded(10),"stop fade must stop and retain volume/clip");
        audio.play(10);audio.fade_stop(10,10,true);mix(10);require(!audio.loaded(10),"delete fade must release decoded samples");
        audio.load(10,clip);audio.set_one_only(10);audio.play(10);mix(60);require(!audio.loaded(10)&&!audio.playing(10)&&!audio.loop(10),"one-only transient sound must remove its channel on completion");
        yukari::AudioCommands::Records records;
        yukari::AudioCommands commands(audio,[&](const std::string&){return clip;},records);
        auto command=[&](const std::string& name,std::vector<yukari::Value> args={}){auto result=commands.handle({name,{"test",0},std::move(args),{},{} });require(bool(result),"known audio service unhandled");return *result;};
        command("SoundPlayLoop",{int32_t(30),std::string("loop.wav")});require(audio.status(30)==2&&records.at(30).second,"loop command load/play failed");require(!command("SoundPlayWait",{int32_t(30)}).result.waiting,"loop PlayWait must return immediately");
        command("SoundLoopSet",{int32_t(30),int32_t(0)});require(audio.status(30)==1,"LoopSet must affect current playback");require(command("SoundPlayWait",{int32_t(30)}).wait_sound==30,"nonloop PlayWait must expose pending channel");
        require(command("SoundStop",{int32_t(30)}).wait_ms==1000,"SoundStop default must block for 1000 ms");mix(1000);command("SoundDelete",{int32_t(30)});require(!audio.loaded(30)&&!records.count(30),"SoundDelete must release resource metadata");
        command("SoundGroupSet",{int32_t(8),int32_t(40),int32_t(49)});command("SoundGroupVolumeSet",{int32_t(8),int32_t(700)});require(std::get<int32_t>(command("SoundGroupVolumeGet",{int32_t(8)}).result.value)==700,"group volume query failed");
        command("SoundGroupSilentSetToFade",{int32_t(8),int32_t(1),int32_t(10)});mix(10);require(audio.group_volume(8)==1&&!audio.group_silent(8),"native SilentSetToFade must fade numerical volume to 1");
        command("CDSilentSet",{int32_t(1)});require(std::get<int32_t>(command("CDSilentGet").result.value)==1,"CD configuration getter failed");
        bool unsupported=false;try{command("CD",{int32_t(1)});}catch(const std::runtime_error&){unsupported=true;}require(unsupported,"missing CD device must not claim playback");
        unsupported=false;try{command("MIDIPlay",{std::string("song.mid")});}catch(const std::runtime_error&){unsupported=true;}require(unsupported,"missing MIDI synthesizer must not claim playback");
        audio.set_virtual_clock(true);audio.load(70,clip);audio.play(70);audio.advance_virtual(25);require(audio.playing(70),"virtual clock advanced too far");audio.advance_virtual(30);require(!audio.playing(70),"virtual clock did not finish sound");
        audio.play(70,true);audio.fade_volume(70,0,20);audio.advance_virtual(20);require(audio.playing(70)&&audio.volume(70)==0,"virtual clock must advance loop and fade together");audio.fade_stop(70,20,true);audio.advance_virtual(20);require(!audio.loaded(70),"virtual clock did not complete delete fade");audio.load(88,clip);audio.play(88);audio.set_loop(88,true);require(audio.status(88)==2,"LoopSet must affect native status query");audio.advance_virtual(60);require(!audio.playing(88)&&audio.loop(88),"LoopSet must preserve current buffer mode until replay");audio.play(88);audio.advance_virtual(60);require(audio.playing(88),"replay must use configured loop flag");audio.stop(88);audio.set_virtual_clock(false);
        {
            yukari::Audio source(false), restored(false);source.set_virtual_clock(true);restored.set_virtual_clock(true);
            source.load(1,clip);source.load(2,clip);source.set_group(4,1,5);source.set_group_volume(4,800);source.set_group_auto_skip(4,6);source.set_silent(2,true);source.set_one_only(2);source.play(1,true);source.play(2);
            source.fade_volume(1,100,100);source.fade_group_volume(4,300,200);source.advance_virtual(7);
            auto state=source.snapshot();restored.load(1,clip);restored.load(2,clip);restored.set_group(99,1,99);restored.set_group_silent(99,true);restored.set_muted(true);restored.restore_playback(state);
            require(restored.snapshot()==state,"audio playback/group snapshot did not restore exactly");require(restored.effective_volume(1)==0,"restore must preserve external master mute");restored.set_muted(false);
            require(restored.group_volume(99)==0&&restored.group_auto_skip(4)==6&&restored.silent(2)&&restored.one_only(2),"restored group/flag state incorrect");
            std::vector<float> expected(128),actual(128);source.render(expected.data(),64);restored.render(actual.data(),64);require(expected==actual,"restored sample cursor or fade progression changed audible output");
            source.fade_stop(1,20,true);source.advance_virtual(5);auto deleting=source.snapshot();restored.restore_playback(deleting);restored.advance_virtual(15);require(!restored.loaded(1),"restore lost pending stop/delete fade");
            auto before=restored.snapshot();auto damaged=before;damaged.pop_back();bool rejected=false;try{restored.restore_playback(damaged);}catch(const std::runtime_error&){rejected=true;}require(rejected&&restored.snapshot()==before,"invalid audio restore must be transactional");
            yukari::Audio missing(false);rejected=false;try{missing.restore_playback(state);}catch(const std::runtime_error&){rejected=true;}require(rejected,"restore must reject missing PCM resources");
            restored.clear();require(!restored.loaded(2)&&restored.group_volume(4)==0,"clear must erase channel and group configuration");restored.advance_virtual(1);
        }
        {
            auto voice=clip;const yukari::Bytes payload(clip.begin()+44,clip.end());voice.resize(44+payload.size()*20);for(size_t i=0;i<20;++i)std::copy(payload.begin(),payload.end(),voice.begin()+44+i*payload.size());put32(voice,4,uint32_t(voice.size()-8));put32(voice,40,uint32_t(voice.size()-44));
            yukari::Audio duck(false), restored(false);duck.set_virtual_clock(true);restored.set_virtual_clock(true);duck.load(10,clip);duck.load(100,voice);duck.set_group(1,100,199);duck.set_group(2,10,19);duck.set_history_groups({1});duck.set_restrain_level(400);duck.set_restrain_groups({2});duck.play(10,true);duck.play(100);duck.advance_virtual(200);require(duck.group_volume(2)==400,"history voice must restrain target group over 200ms");
            restored.load(10,clip);restored.load(100,voice);restored.restore_playback(duck.snapshot());restored.stop(100);restored.advance_virtual(500);require(restored.group_volume(2)==1000,"restored restraint must recover to 1000 over 500ms");
            duck.set_loop(100,true);duck.advance_virtual(500);require(duck.group_volume(2)==1000,"looping history group sound must not trigger restraint");
        }
        {
            auto intro=clip,body=clip;for(size_t i=44;i+1<clip.size();i+=2){put16(intro,i,2048);put16(body,i,uint16_t(-4096));}
            yukari::Audio stream(false),restored(false);stream.set_virtual_clock(true);restored.set_virtual_clock(true);stream.load_stream(1,{intro,body},1);require(stream.streamed(1)&&stream.stream_loop_start(1)==1&&stream.duration_ms(1)>=99,"stream segment metadata incorrect");stream.play(1,true);
            std::vector<float> pcm(480);stream.render(pcm.data(),240);require(pcm[100]>0,"stream intro must decode first");stream.advance_virtual(60);stream.render(pcm.data(),240);require(pcm[100]<0,"stream must advance to second fragment");stream.advance_virtual(100);stream.render(pcm.data(),240);require(pcm[100]<0&&stream.playing(1),"nonzero loop_start must not replay intro fragment");
            stream.fade_volume(1,200,100);stream.advance_virtual(7);auto state=stream.snapshot();auto sources=yukari::Audio::sources_from_snapshot(state);require(sources.size()==1&&sources[0].id==1&&sources[0].streamed&&sources[0].loop_start==1,"stream snapshot source inspection failed");restored.load_stream(1,{intro,body},1);restored.restore_playback(state);std::vector<float> expected(128),actual(128);stream.render(expected.data(),64);restored.render(actual.data(),64);require(expected==actual,"stream snapshot must preserve decoded cursor and fade sample output");
            stream.set_loop(1,false);stream.advance_virtual(120);require(!stream.playing(1),"stream LoopSet must affect refill behavior immediately");
            yukari::AudioCommands::Records paths;std::vector<std::string> loaded;yukari::AudioCommands commands(stream,[&](const std::string& name){loaded.push_back(name);return name=="intro.wav"?intro:body;},paths);
            commands.handle({"SoundLoadStream",{"test",0},{int32_t(2),std::string("intro.wav\tbody.wav"),int32_t(1)},{},{}});require(loaded==std::vector<std::string>({"intro.wav","body.wav"})&&stream.streamed(2)&&stream.stream_loop_start(2)==1,"SoundLoadStream must split TAB names and preserve third argument");
            commands.handle({"SoundPlayLoop",{"test",0},{int32_t(2)},{},{}});stream.advance_virtual(150);require(stream.playing(2),"stream provider command must loop from selected fragment");
            commands.handle({"SoundLoad",{"test",0},{int32_t(3),std::string("intro.wav\tbody.wav")},{},{}});require(!stream.streamed(3)&&stream.duration_ms(3)>=99,"buffered SoundLoad must concatenate fragments");
        }
        {
            yukari::Audio curves(false),restore(false);require(!curves.volume_type(),"native default volume type must be false");curves.load(1,clip);curves.set_volume(1,500);require(curves.applied_attenuation(1)==-5000,"linear-decibel mode attenuation incorrect");curves.play(1,true);std::vector<float> quiet(256),loud(256);curves.render(quiet.data(),128);curves.set_volume_type(true);curves.play(1,true);curves.render(loud.data(),128);require(curves.applied_attenuation(1)==-752,"logarithmic mode attenuation incorrect");require(std::abs(loud[20]/quiet[20]-float(yukari::direct_sound_pcm_gain(-752)/yukari::direct_sound_pcm_gain(-5000)))<.001f,"mixer did not apply DirectSound dB gain");
            curves.set_volume(1,2000);require(curves.volume(1)==2000&&curves.applied_attenuation(1)==-752,"invalid DirectSound gain must retain previous attenuation");auto state=curves.snapshot();restore.set_volume_type(true);restore.load(1,clip);restore.restore_playback(state);require(restore.applied_attenuation(1)==-752&&restore.volume(1)==2000&&restore.snapshot()==state,"invalid-volume cached attenuation lost in snapshot");curves.render(quiet.data(),128);restore.render(loud.data(),128);require(quiet==loud,"cached invalid-volume gain must survive restore");
            curves.clear();require(curves.volume_type(),"clear must preserve external volume curve");
            yukari::Audio legacy(false);legacy.load(1,clip);legacy.set_volume(1,500);auto old=legacy.snapshot();put32(old,8,4);old.erase(old.begin()+110);old.erase(old.begin()+70,old.begin()+74);restore.restore_playback(old);require(restore.applied_attenuation(1)==-752,"version4 restore must derive attenuation using current INI curve");require(yukari::Audio::sources_from_snapshot(old).size()==1,"version4 source metadata compatibility lost");
        }
        bool rejected{};
        try { audio.load(11, {1, 2, 3, 4}); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "invalid audio must be rejected");
        rejected = false;
        try { (void)audio.duration_ms(999); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "unknown channel duration must be rejected");
        std::cout << "audio playback, groups, fades, commands, virtual clock, snapshots, restraint and segmented streams passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

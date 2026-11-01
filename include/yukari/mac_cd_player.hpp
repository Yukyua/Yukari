#pragma once
#include "yukari/audio.hpp"
#include "yukari/cd_commands.hpp"
#include <memory>
namespace yukari {
struct CdMount {std::string path,name,device;};
struct CdTrackFile {int32_t number;std::string path;};
struct MacCdHooks {
 std::function<std::vector<CdMount>()> mounts;
 std::function<std::vector<CdTrackFile>(const CdMount&)> tracks;
 std::function<Bytes(const std::string&)> read_file;
 std::function<std::unique_ptr<Audio>()> make_audio;
 std::function<uint32_t()> now_ms;
};
// Uses mounted cddafs virtual AIFF files; never writes optical media. Must
// outlive CdCommands made with provider(). Audio is independent and lazy.
class MacCdPlayer {
public:
 explicit MacCdPlayer(MacCdHooks hooks={});
 ~MacCdPlayer();
 CdProvider provider();
 void set_clock_sink(std::function<void(uint32_t,uint32_t)> sink);
 void set_muted(bool muted);
 // Bind to CdCommands::state().silent so a new CD request cannot defeat silence.
 void set_silent_query(std::function<bool()> query);
 bool audio_created()const;
 const std::string& last_error()const;
 static std::vector<CdMount> mounted_audio_discs();
 static std::vector<CdTrackFile> track_files(const CdMount&);
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}

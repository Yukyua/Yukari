#pragma once
#include "yukari/formats.hpp"
namespace yukari {
struct CdTrack {
 uint32_t number{},start_sector{},end_sector{};
 bool audio{};
 uint64_t duration_ms()const{return uint64_t(end_sector-start_sector)*1000/75;}
};
struct CdMedia {std::string device;std::vector<CdTrack> tracks;};
// Read-only IOKit IOCDMedia discovery. Empty means no mounted optical disc;
// this does not manufacture a playing state when a drive is absent.
std::vector<CdMedia> enumerate_cd_media();
// Apple's full binary CDTOC (READ TOC format 2), including session lead-outs.
std::vector<CdTrack> decode_cd_toc(const Bytes& toc);
}

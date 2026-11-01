#pragma once
#include "yukari/formats.hpp"

namespace yukari {
// Original 0x42acb0 persists the two 10,000-entry global banks in System.dat.
// Unknown font/config sections remain in the complete source template.
struct OriginalSystemState {
    std::vector<int32_t> flags;
    std::vector<std::string> strings;
    Bytes template_bytes;
    std::string encoding{"CP932"};
};
OriginalSystemState decode_original_system(const Bytes& bytes,const char* encoding = "CP932");
// Requires a decoded native template; returns a new file and never modifies it.
Bytes encode_original_system(const OriginalSystemState& state);
}

#pragma once
#include "yukari/formats.hpp"
namespace yukari {
// Read-only PE32 resource lookup. Language falls back to the first resource.
Bytes pe_resource(const Bytes& executable,uint16_t type,uint16_t id);
Bytes pe_icon(const Bytes& executable,uint16_t group_id);
}

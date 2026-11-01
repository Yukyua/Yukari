#pragma once
#include <filesystem>
#include <iosfwd>

namespace yukari {
// Static extraction of every type-5 YKS string literal in the effective original
// resource set. Creates a new UTF-8 JSONL/CSV export; never runs scripts or writes
// game resources. Aliases at the same data offset share a record and all uses.
void dump_game_text(const std::filesystem::path& game_directory,
                    const std::filesystem::path& output_directory,std::ostream& log);
}

#pragma once
#include "yukari/formats.hpp"
#include "yukari/vm.hpp"
#include <map>

namespace yukari {
// Yukari's own versioned container. It is deliberately distinct from original
// Yuka save files; runtime state is an opaque, independently versioned payload.
struct SaveGame {
    int64_t timestamp{};
    std::string comment;
    Bytes thumbnail;
    Bytes vm;
    Bytes runtime;
};
Bytes encode_save(const SaveGame& game);
SaveGame decode_save(const Bytes& bytes);

// Read-only structural inspection of observed "Yuka  " containers. Descriptor
// offsets identify header fields; their engine semantics are not inferred.
struct OriginalSaveRegion { uint32_t descriptor, offset, size; };
struct OriginalSaveInfo {
    uint32_t header_size{};
    std::vector<OriginalSaveRegion> regions;
};
OriginalSaveInfo inspect_original_save(const Bytes& bytes);
struct OriginalSaveMetadata {
    std::string comment1, comment2, title;
    Bytes thumbnail_bmp;
};
OriginalSaveMetadata read_original_save_metadata(const Bytes& bytes, const char* encoding = "CP932");
struct OriginalSaveMetadataChange {
    // SaveDataCommentChange/Get use this first string descriptor (0x10/14).
    std::optional<std::string> comment1;
    std::optional<std::string> comment2;
    // Already encoded/resized BMP. Native ThumbnailChange first loads and
    // scales its input image; rendering/conversion belongs to the caller.
    std::optional<Bytes> thumbnail_bmp;
    const char* encoding = "CP932";
};
// Native metadata replacement appends new payloads and only patches the root
// selected comment/thumbnail descriptors. No existing byte is relocated, preserving
// unknown state, absolute scene/frame pointers, history and nested containers.
// Failure is atomic: the const input is never modified.
Bytes rewrite_original_save_metadata(const Bytes& bytes, const OriginalSaveMetadataChange& change);
struct OriginalFrame {
    std::string name;
    uint32_t current_word{}, next_word{}, target_index{}, mode{};
    bool target_active{}, embedded_script{};
    Script script;
    std::vector<Value> locals;
    Bytes header;
};
struct OriginalSaveState {
    std::vector<int32_t> flags;
    std::map<int32_t, std::string> strings;
    std::vector<OriginalFrame> frames;
    Bytes state_header;
};
// Offsets in nested original containers are relative to their own base.
// This decodes VM state only; scene/audio/history require their own readers.
OriginalSaveState decode_original_vm(const Bytes& bytes, uint32_t base = 0, const char* encoding = "CP932");
struct OriginalSaveWriteOptions {
    // Native initial save: comment1=user comment, comment2=Index; title is
    // the distinct game/window title stored at header0x40/44.
    std::string comment1, comment2, title = "Yukari";
    Bytes thumbnail_bmp, history, previous, template_bytes;
    const char* encoding = "CP932";
    // Appends and records scene/audio sections in the 0x160-byte state header.
    std::function<void(Bytes&, uint32_t)> append_scene;
};
// Experimental native layout writer. Structural roundtrip is tested; Windows
// engine interoperability must be verified separately before claiming support.
Bytes encode_original_save(const OriginalSaveState& state, const OriginalSaveWriteOptions& options = {});
}

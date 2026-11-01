#include "yukari/save.hpp"
#include "yukari/vm.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace yukari;
void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
int32_t number(const Value& value) {
    auto p = std::get_if<int32_t>(&value);
    check(p != nullptr, "expected integer value");
    return *p;
}
void rejects(const std::function<void()>& work) {
    try { work(); } catch (const std::runtime_error&) { return; }
    throw std::runtime_error("expected invalid save/state to be rejected");
}
struct Program {
    Script script;
    uint32_t word = 0;
    uint32_t element(uint32_t type, const std::string& text = {}, int32_t value = 0, uint32_t b = 0) {
        const auto index = uint32_t(script.elements.size());
        script.elements.push_back({type, 0, b, 0, text, value}); return index;
    }
    uint32_t integer(int32_t n) { return element(4, {}, n, uint32_t(script.elements.size()) * 4); }
    uint32_t local(uint32_t index) {
        script.max_locals = std::max(script.max_locals, index + 1);
        return element(10, {}, 0, index);
    }
    uint32_t flag(int32_t index, const std::string& bank = "Flag") { return element(8, bank, index); }
    uint32_t label(const std::string& text) { return element(1, text, -1); }
    uint32_t emit(uint32_t index, const std::vector<uint32_t>& args = {}) {
        const auto result = word;
        script.instructions.push_back({word, index, args});
        word += script.elements[index].type == 0 ? 2 + uint32_t(args.size()) : 1;
        return result;
    }
    void call(const std::string& name, const std::vector<uint32_t>& args = {}) { emit(element(0, name), args); }
    void assign(uint32_t target, const std::vector<uint32_t>& args) { emit(target); call("=", args); }
};
Vm::Loader loader(std::map<std::string, Script> scripts) {
    return [scripts = std::move(scripts)](const std::string& name) { return scripts.at(name); };
}
void codec() {
    SaveGame original;
    original.timestamp = -7; original.comment = "Yukari save \xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88";
    original.vm = {0,1,2,255}; original.runtime = {5,4,3,2,1}; original.thumbnail = {137,80,78,71};
    const auto bytes = encode_save(original);
    const auto decoded = decode_save(bytes);
    check(decoded.timestamp == original.timestamp && decoded.comment == original.comment &&
          decoded.vm == original.vm && decoded.runtime == original.runtime &&
          decoded.thumbnail == original.thumbnail, "save container roundtrip lost fields");
    auto corrupt = bytes; corrupt.back() ^= 1;
    rejects([&] { decode_save(corrupt); });
    corrupt = bytes; corrupt.pop_back(); rejects([&] { decode_save(corrupt); });
    corrupt = bytes; corrupt.push_back(0); rejects([&] { decode_save(corrupt); });
    corrupt = bytes; corrupt[12] = 99; rejects([&] { decode_save(corrupt); });
    original.comment.resize(1024 * 1024 + 1);
    rejects([&] { encode_save(original); });
}
void suspended_menu_roundtrip() {
    Program root, menu;
    const auto a = root.local(0), b = root.local(1), sum = root.local(2);
    root.assign(a, {root.integer(5)}); root.emit(b); root.call("="); root.call("Pause");
    root.assign(sum, {a, root.label("+"), b}); root.call("Return", {sum});
    menu.assign(menu.flag(0), {menu.integer(77)});
    menu.assign(menu.flag(0, "GlobalFlag"), {menu.integer(88)});
    menu.call("MenuPause"); menu.call("Return");
    Vm vm(loader({{"root", root.script}, {"menu", menu.script}}));
    unsigned hosts = 0;
    vm.set_host([&](const HostCall&) -> std::optional<HostResult> { ++hosts; return HostResult{true,{}}; });
    vm.start("root"); check(vm.run() == VmStatus::Waiting, "root did not pause");
    const auto root_snapshot = vm.snapshot();
    vm.call("menu"); check(vm.in_call(), "injected menu was not marked active");
    check(vm.run() == VmStatus::Waiting, "menu did not pause");
    SaveGame save; save.vm = vm.snapshot(); save.runtime = {1,2,3};
    const auto restored = decode_save(encode_save(save));
    vm.restore(restored.vm); check(vm.in_call(), "snapshot lost injected context stack");
    vm.resume();
    check(vm.run() == VmStatus::Waiting && !vm.in_call(), "menu completion did not restore original wait");
    check(number(vm.variable("Flag", 0)) == 77 && number(vm.variable("GlobalFlag", 0)) == 88,
          "menu modifications to shared banks were lost");
    vm.resume(int32_t{99});
    check(vm.run() == VmStatus::Halted && number(vm.last_result()) == 104,
          "snapshot lost root locals or pending host result target");
    vm.restore(root_snapshot);
    check(number(vm.variable("Flag", 0)) == 0, "restore did not restore shared bank contents");
    vm.resume(int32_t{7});
    check(vm.run() == VmStatus::Halted && number(vm.last_result()) == 12, "root snapshot did not resume correctly");
    check(hosts == 2, "restore replayed a suspended host call");
    const auto stable = vm.snapshot(); auto corrupt = stable; corrupt.back() ^= 1;
    rejects([&] { vm.restore(corrupt); });
    check(vm.snapshot() == stable, "failed restore changed current VM");
    auto changed = root.script; changed.elements.back().text += "changed";
    Vm incompatible(loader({{"root", changed}, {"menu", menu.script}}));
    rejects([&] { incompatible.restore(root_snapshot); });
}
void selection_roundtrip() {
    Program p;
    const auto outer = p.label("{"), closing = p.label("}"), branch = p.label("{"), end = p.label("}");
    p.call("MouseSelect"); const auto begin_word = p.emit(outer);
    p.call("MouseCase2", {p.integer(1)}); const auto branch_word = p.emit(branch);
    p.assign(p.flag(5), {p.integer(42)}); p.call("Return", {p.integer(42)});
    const auto branch_end = p.emit(end), outer_end = p.emit(closing); p.call("Return");
    p.script.elements[outer].value = int32_t(outer_end); p.script.elements[closing].value = int32_t(begin_word);
    p.script.elements[branch].value = int32_t(branch_end); p.script.elements[end].value = int32_t(branch_word);
    Vm vm(loader({{"selection", p.script}}));
    unsigned callbacks = 0;
    vm.set_host([&](const HostCall& call) -> std::optional<HostResult> {
        ++callbacks; return HostResult{false,{},call.function == "MouseSelect"};
    });
    vm.start("selection"); check(vm.run() == VmStatus::Waiting, "selection did not wait");
    auto snapshot = vm.snapshot(); vm.restore(snapshot);
    check(vm.run() == VmStatus::Waiting && callbacks == 2, "restored selection replayed declarations");
    vm.choose(branch_word);
    check(vm.run() == VmStatus::Halted && number(vm.last_result()) == 42,
          "restored selection could not execute chosen branch");
}
void data_alias_roundtrip() {
    Program p;
    const auto alias = p.element(4, {}, 3, 0), nine = p.element(4, {}, 9, 4), answer = p.element(4, {}, 42, 8);
    const auto flag = p.flag(3); p.script.elements[flag].c = 0;
    p.script.data = {3,0,0,0,9,0,0,0,42,0,0,0};
    p.assign(alias, {nine}); p.assign(flag, {answer}); p.call("Return");
    Vm vm(loader({{"alias", p.script}})); vm.start("alias");
    check(vm.run(2) == VmStatus::StepLimit, "alias snapshot was not taken after data mutation");
    const auto snapshot = vm.snapshot(); vm.restore(snapshot);
    check(vm.run() == VmStatus::Halted && number(vm.variable("Flag", 9)) == 42 &&
          number(vm.variable("Flag", 3)) == 0, "mutable script data alias did not survive restore");
}
void original_header_only() {
    Bytes fixture(72, 0);
    const std::string magic("Yuka  \0\0", 8);
    std::copy(magic.begin(), magic.end(), fixture.begin()); fixture[8] = 72;
    const auto info = inspect_original_save(fixture);
    check(info.header_size == 72 && info.regions.size() == 7, "original header inspection failed");
    rejects([&] { decode_save(fixture); });
    fixture[0x10] = 73;
    rejects([&] { inspect_original_save(fixture); });
}
void native_vm_roundtrip() {
    Script script;
    script.max_locals = 3;
    script.data = {'R','e','t','u','r','n',0,42,0,0,0};
    script.elements = {{0,0,0,0,"Return",0},{4,0,7,0,{},42}};
    script.instructions = {{0,0,{1}}};
    OriginalSaveState original;
    original.flags.resize(65536); original.flags[3] = -17;
    original.strings.emplace(7,"native string");
    OriginalFrame frame;
    frame.name = "native.yks"; frame.current_word = 0; frame.next_word = 3;
    frame.embedded_script = true; frame.script = script; frame.mode = 1;
    frame.locals = {int32_t{10},std::string("local string"),Value{}};
    original.frames.push_back(frame);
    OriginalSaveWriteOptions options;
    options.comment2 = "native layout test";
    const auto bytes = encode_original_save(original,options);
    const auto info = inspect_original_save(bytes);
    check(info.regions[4].size == 26654,"default original BMP thumbnail layout changed");
    const auto decoded = decode_original_vm(bytes);
    check(decoded.flags == original.flags && decoded.strings == original.strings &&
          decoded.frames.size() == 1 && decoded.frames[0].locals == frame.locals &&
          decoded.frames[0].script.data == script.data,"native VM writer/reader lost state");
    Vm vm(loader({{"native.yks",script}}));
    vm.set_variable("GlobalFlag",42,int32_t{99}); vm.import_original(decoded);
    check(number(vm.variable("GlobalFlag",42)) == 99,"native import overwrote global System.dat state");
    const auto snapshot = vm.snapshot(); vm.restore(snapshot);
    const auto exported = vm.export_original();
    check(exported.state_header == decoded.state_header && exported.frames[0].header == decoded.frames[0].header,
          "snapshot lost native header templates");
    check(exported.frames[0].current_word == 0 && exported.frames[0].next_word == 3,
          "snapshot lost distinct native current/next program counters");
    check(vm.run() == VmStatus::Halted && number(vm.last_result()) == 42,"native imported code did not execute");
    auto bad = bytes;
    const auto state_header = u32(bad,0x20);
    bad[state_header+0x18] = 1;
    rejects([&] { decode_original_vm(bad); });
}
void native_wait_does_not_advance() {
    Script script;
    script.max_locals = 1;
    script.data = {'=',0,'K','e','y','W','a','i','t',0,'R','e','t','u','r','n',0};
    script.elements = {{0,0,0,0,"=",0},{0,2,0,0,"KeyWait",0},
                       {0,10,0,0,"Return",0},{10,0,0,0,{},0}};
    script.instructions = {{0,3,{}},{1,0,{}},{3,1,{}},{5,2,{3}}};
    const auto sources = loader({{"keywait.yks",script}});
    unsigned calls = 0;
    const Vm::Host host = [&](const HostCall& call) -> std::optional<HostResult> {
        check(call.function == "KeyWait", "unexpected host after native restore");
        ++calls; return HostResult{true,{}};
    };
    Vm original(sources); original.set_host(host); original.start("keywait.yks");
    check(original.run() == VmStatus::Waiting,"source KeyWait did not pause");
    const auto state = original.export_original();
    check(state.frames[0].current_word == 3 && state.frames[0].next_word == 5 &&
          state.frames[0].target_active,"native export lost pending KeyWait state");
    const auto decoded = decode_original_vm(encode_original_save(state));
    Vm restored(sources); restored.set_host(host); restored.import_original(decoded);
    check(restored.run() == VmStatus::Waiting && calls == 2,"native restore advanced beyond the saved KeyWait");
    restored.resume(int32_t{42});
    check(restored.run() == VmStatus::Halted && number(restored.last_result()) == 42,
          "native KeyWait restore lost its pending assignment target");
}
void native_data_alias_uses_live_bytes() {
    Script script;
    script.data = {'=',0,'R','e','t','u','r','n',0,3,0,0,0,9,0,0,0,42,0,0,0,'F','l','a','g',0};
    script.elements = {{0,0,0,0,"=",0},{0,2,0,0,"Return",0},
                       {4,0,9,0,{},3},{4,0,13,0,{},9},{4,0,17,0,{},42},{8,21,0,9,"Flag",3}};
    script.instructions = {{0,2,{}},{1,0,{3}},{4,5,{}},{5,0,{4}},{8,1,{}}};
    const auto sources = loader({{"native-alias.yks",script}});
    Vm vm(sources); vm.start("native-alias.yks");
    check(vm.run(2) == VmStatus::StepLimit,"native alias save point was not reached");
    const auto exported = vm.export_original();
    check(u32(exported.frames[0].script.data,9) == 9 && exported.frames[0].script.elements[2].value == 3,
          "alias fixture did not separate live data from cached element value");
    Vm restored(sources); restored.import_original(decode_original_vm(encode_original_save(exported)));
    check(restored.run() == VmStatus::Halted && number(restored.variable("Flag",9)) == 42 &&
          number(restored.variable("Flag",3)) == 0,"native writer serialized a stale cached constant instead of live script bytes");
}
void native_metadata_rewrite() {
    OriginalSaveState state;state.flags.resize(65536);state.flags[77]=12345;
    state.strings[3]="native string";
    OriginalSaveWriteOptions options;options.comment1="chapter";options.comment2="old comment";options.title="native title";
    options.history={1,2,3,4,5,6,7};
    OriginalSaveWriteOptions nested_options;nested_options.comment2="nested comment";
    options.previous=encode_original_save(state,nested_options);
    auto bytes=encode_original_save(state,options);
    bytes[12]=0x5a;bytes.push_back(0xab);bytes.push_back(0xcd); // unknown header/trailer preserved
    const auto before=bytes;
    const auto original=read_original_save_metadata(bytes);
    check(original.comment1=="chapter"&&original.comment2=="old comment"&&original.title=="native title","native metadata reader fields");
    auto bmp=original.thumbnail_bmp;bmp[54]=0x44;bmp[55]=0x55;bmp[56]=0x66;
    OriginalSaveMetadataChange change;change.comment2="new \xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88";change.thumbnail_bmp=bmp;
    const auto output=rewrite_original_save_metadata(bytes,change);
    check(bytes==before,"metadata rewrite modified const source");
    for(size_t i=0;i<bytes.size();++i){
        if((i>=0x18&&i<0x20)||(i>=0x30&&i<0x38))continue;
        check(output[i]==bytes[i],"metadata rewrite changed original native/unknown byte at "+std::to_string(i));
    }
    const auto metadata=read_original_save_metadata(output);
    check(metadata.comment1==original.comment1&&metadata.title==original.title&&metadata.comment2==*change.comment2&&metadata.thumbnail_bmp==bmp,"metadata replacement lost unrelated fields");
    check(u32(output,0x18)==bytes.size()&&u32(output,0x30)>bytes.size(),"native metadata payloads not appended");
    check(u32(output,0x38)==u32(bytes,0x38)&&u32(output,0x3c)==u32(bytes,0x3c),"nested descriptor changed");
    check(read_original_save_metadata(Bytes(output.begin()+u32(output,0x38),output.begin()+u32(output,0x38)+u32(output,0x3c))).comment2=="nested comment","nested comment unexpectedly replaced");
    const auto decoded=decode_original_vm(output);check(decoded.flags==state.flags&&decoded.strings==state.strings,"metadata rewrite changed VM data");
    OriginalSaveMetadataChange native_comment;native_comment.comment1="native edited comment";
    const auto edited_first=rewrite_original_save_metadata(output,native_comment);
    const auto first_metadata=read_original_save_metadata(edited_first);
    check(first_metadata.comment1==*native_comment.comment1&&first_metadata.comment2==metadata.comment2&&first_metadata.thumbnail_bmp==metadata.thumbnail_bmp,"native CommentChange must edit only first descriptor");
    for(size_t i=0;i<output.size();++i)if(i<0x10||i>=0x18)check(edited_first[i]==output[i],"first-comment edit altered unrelated original byte");
    OriginalSaveMetadataChange no_change;check(rewrite_original_save_metadata(output,no_change)==output,"empty metadata change is not byte-identical");
    OriginalSaveMetadataChange empty;empty.comment2="";const auto cleared=rewrite_original_save_metadata(output,empty);
    check(read_original_save_metadata(cleared).comment2.empty()&&u32(cleared,0x1c)==1,"empty comment must retain native NUL payload");
    OriginalSaveMetadataChange bad;bad.comment2=std::string("x\0y",3);rejects([&]{rewrite_original_save_metadata(output,bad);});
    bad.comment2="\xf0\x9f\x98\x80";rejects([&]{rewrite_original_save_metadata(output,bad);});
    bad.comment2.reset();bad.thumbnail_bmp=Bytes{1,2,3};rejects([&]{rewrite_original_save_metadata(output,bad);});
    bad.thumbnail_bmp=bmp;(*bad.thumbnail_bmp)[2]=0;(*bad.thumbnail_bmp)[3]=0;(*bad.thumbnail_bmp)[4]=0;(*bad.thumbnail_bmp)[5]=0;
    rejects([&]{rewrite_original_save_metadata(output,bad);});
    auto malformed=output;malformed[0x18]=0xff;malformed[0x19]=0xff;malformed[0x1a]=0xff;malformed[0x1b]=0xff;
    rejects([&]{rewrite_original_save_metadata(malformed,no_change);});
    check(bytes==before,"failed metadata rewrite changed source bytes");
}
}
int main() {
    const std::pair<const char*, std::function<void()>> tests[] = {
        {"save codec", codec}, {"suspended menu snapshot", suspended_menu_roundtrip},
        {"selection snapshot", selection_roundtrip}, {"mutable script snapshot", data_alias_roundtrip},
        {"original header inspection", original_header_only},
        {"native VM roundtrip", native_vm_roundtrip},
        {"native KeyWait resume", native_wait_does_not_advance},
        {"native live data alias", native_data_alias_uses_live_bytes},
        {"native metadata rewrite", native_metadata_rewrite},
    };
    unsigned failed = 0;
    for (const auto& test : tests) {
        try { test.second(); std::cout << "PASS " << test.first << '\n'; }
        catch (const std::exception& e) { ++failed; std::cerr << "FAIL " << test.first << ": " << e.what() << '\n'; }
    }
    return failed ? 1 : 0;
}

#include "yukari/formats.hpp"
#include "yukari/vm.hpp"
#include "yukari/runtime.hpp"
#include "yukari/text_dump.hpp"
#include "yukari/translation_patch.hpp"
#include "yukari/original_scene.hpp"
#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void usage() {
    std::cout << "Yukari 0.1 - Yuka System / SDL2 runtime\n"
              << "Usage:\n"
              << "  yukari list ARCHIVE.ykc\n"
              << "  yukari extract ARCHIVE.ykc ENTRY OUTPUT\n"
              << "  yukari script FILE.yks\n"
              << "  yukari script-entry ARCHIVE.ykc ENTRY\n"
              << "  yukari graphic FILE.ykg\n"
              << "  yukari view FILE [--once]\n"
              << "  yukari view-entry ARCHIVE.ykc ENTRY [--once]\n"
              << "  yukari game-script GAME_DIRECTORY ENTRY\n"
              << "  yukari game-view GAME_DIRECTORY ENTRY [--once]\n"
              << "  yukari dump-text GAME_DIRECTORY OUTPUT_DIRECTORY\n"
              << "  yukari validate-translation-patch GAME_DIRECTORY PATCH\n"
              << "  yukari vm GAME_DIRECTORY [ENTRY] [--trace] [--max-steps N]\n"
              << "  yukari play GAME_DIRECTORY [--until-title] [--screenshot PATH] [--fast] [--mute]\n"
              << "    [--new-game] [--until-text] [--advance N] [--state-dir PATH]\n"
              << "    [--translation-patch PATH] [--original-exe PATH] [--font-directory PATH] [--gdi-cache PATH] [--require-gdi-capture]\n"
              << "    [--load-slot N | --load-original-slot N] [--original-encoding CP932|CP936]\n"
              << "    [--require-gpu] [--actions FILE] [--trace] [--max-ms N] [--seed N]\n"
              << "VM stops at unimplemented engine functions. Esc closes preview.\n";
}
void graphic(const yukari::Bytes& bytes) {
    const auto g = yukari::parse_graphic(bytes);
    std::cout << "color_bytes=" << g.color.size() << " alpha_bytes=" << g.alpha.size() << " frames=" << g.frames.size() << '\n';
    for (size_t i = 0; i < g.frames.size(); ++i) {
        const auto& f = g.frames[i];
        std::cout << i << ": rect=" << f.x << ',' << f.y << ',' << f.w << ',' << f.h
                  << " duration_raw=" << f.duration << " type=" << f.type << " unknown=" << f.unknown << " action=" << f.action << '\n';
    }
}
int run_vm(int argc, char** argv) {
    std::string entry = "start.yks";
    bool has_entry = false, trace = false;
    uint64_t limit = 100000;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--trace") trace = true;
        else if (arg == "--max-steps" && i + 1 < argc) {
            const std::string number = argv[++i];
            const auto result = std::from_chars(number.data(), number.data()+number.size(), limit);
            if (result.ec != std::errc{} || result.ptr != number.data()+number.size() || !limit)
                throw std::runtime_error("--max-steps requires a positive integer");
        } else if (arg.rfind("--", 0) != 0 && !has_entry) { entry = arg; has_entry = true; }
        else throw std::runtime_error("invalid VM argument: " + arg);
    }
    const yukari::Resources resources(argv[2]);
    yukari::Vm vm([&](const std::string& name) { return yukari::parse_script(resources.read(name)); });
    if (trace) vm.set_trace([](const yukari::VmLocation& l, const std::string& operation) {
        std::cerr << l.script << ":word=0x" << std::hex << l.word << std::dec << "  " << operation << '\n';
    });
    vm.start(entry);
    const auto status = vm.run(limit);
    std::cout << "VM " << (status == yukari::VmStatus::Halted ? "halted" : status == yukari::VmStatus::Waiting ? "waiting" : "step-limit")
              << " steps=" << vm.steps() << " result=" << yukari::format_value(vm.last_result()) << '\n';
    return status == yukari::VmStatus::Halted ? 0 : 3;
}
int validate_translation_patch(const std::filesystem::path& directory,const std::filesystem::path& path){
    const yukari::Resources resources(directory);const yukari::TranslationPatch patch(path);size_t scripts=0,matched=0;
    for(const auto& resource:resources.entries()){
        std::string name=resource.entry.name;for(char& c:name)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
        if(name.size()<4||name.compare(name.size()-4,4,".yks"))continue;
        auto script=yukari::parse_script(resources.read(resource.entry.name));
        matched+=patch.verify(script,resource.entry.name);
        if(patch.encoding()){
            auto translated=patch.apply(std::move(script),resource.entry.name);
            for(const auto& element:translated.elements)if(element.type==5&&!element.source_text.empty()){
                try{yukari::encode_original_text(element.text,patch.encoding()->c_str());}
                catch(const std::exception& error){throw std::runtime_error("translation_patch "+resource.entry.name+" offset="+std::to_string(element.b)+" encoding="+*patch.encoding()+": "+error.what());}
            }
        }
        ++scripts;
    }
    if(matched!=patch.size())throw std::runtime_error("translation_patch contains "+std::to_string(patch.size()-matched)+" unmatched entries");
    std::cout<<"translation_patch valid scripts="<<scripts<<" entries="<<matched<<'\n';return 0;
}
}
int main(int argc, char** argv) {
    try {
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help")) { usage(); return 0; }
        const std::string command = argv[1];
        if (command == "play" && argc >= 3) {
            return yukari::play_game(argc, argv);
        } else if (command == "dump-text" && argc == 4) {
            yukari::dump_game_text(argv[2],argv[3],std::cout);
        } else if (command == "validate-translation-patch" && argc == 4) {
            return validate_translation_patch(argv[2],argv[3]);
        } else if (command == "vm" && argc >= 3) {
            return run_vm(argc, argv);
        } else if (command == "list" && argc == 3) {
            const yukari::Archive archive(argv[2]);
            for (const auto& e : archive.entries()) std::cout << e.offset << '\t' << e.size << '\t' << e.name << '\n';
        } else if (command == "extract" && argc == 5) {
            yukari::write_file(argv[4], yukari::Archive(argv[2]).read(argv[3]));
        } else if (command == "script" && argc == 3) {
            yukari::disassemble(yukari::parse_script(yukari::read_file(argv[2])), std::cout);
        } else if (command == "script-entry" && argc == 4) {
            yukari::disassemble(yukari::parse_script(yukari::Archive(argv[2]).read(argv[3])), std::cout);
        } else if (command == "graphic" && argc == 3) {
            graphic(yukari::read_file(argv[2]));
        } else if (command == "game-script" && argc == 4) {
            yukari::disassemble(yukari::parse_script(yukari::Resources(argv[2]).read(argv[3])), std::cout);
        } else if (command == "game-view" && (argc == 4 || (argc == 5 && std::string(argv[4]) == "--once"))) {
            yukari::preview(yukari::Resources(argv[2]).read(argv[3]), argv[3], argc == 5 ? 1 : 0);
        } else if (command == "view" && (argc == 3 || (argc == 4 && std::string(argv[3]) == "--once"))) {
            yukari::preview(yukari::read_file(argv[2]), argv[2], argc == 4 ? 1 : 0);
        } else if (command == "view-entry" && (argc == 4 || (argc == 5 && std::string(argv[4]) == "--once"))) {
            yukari::preview(yukari::Archive(argv[2]).read(argv[3]), argv[3], argc == 5 ? 1 : 0);
        } else { usage(); return 2; }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "yukari: " << e.what() << '\n';
        return 1;
    }
}

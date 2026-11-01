// Read-only static call inventory. No executable, patch or dialogue is read/output.
// Build: c++ -std=c++17 -Iinclude tools/audit_compatibility.cpp src/formats.cpp src/resource_source.cpp -liconv -o /tmp/yukari-audit
#include "yukari/formats.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace yukari;
struct Count { uint64_t count{}; uint32_t first{}; };
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("usage: audit_compatibility ORIGINAL_GAME_DIRECTORY");
        // Resources uses the original engine's optional data06..09 mount order.
        // Keep this audit strictly within the requested five-package profile.
        for(int package=6;package<=9;++package){
            char name[32];std::snprintf(name,sizeof(name),"data%02d.ykc",package);
            if(std::filesystem::exists(std::filesystem::path(argv[1])/name))
                throw std::runtime_error("audit requires original data01..05 profile without optional data06..09");
        }
        Resources resources(argv[1]);
        for(int package=1;package<=5;++package){
            char name[32];std::snprintf(name,sizeof(name),"data%02d.ykc",package);
            Archive archive(std::filesystem::path(argv[1])/name);
            for(const auto& e:archive.entries()){
                std::string lower=e.name;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return std::tolower(c);});
                if(lower.size()<4||lower.substr(lower.size()-4)!=".yks")continue;
                const auto script=parse_script(resources.read(e.name));
                std::map<std::string,Count> calls;
                for(const auto& ins:script.instructions){
                    const auto& element=script.elements.at(ins.element);
                    if(element.type!=0)continue;
                    auto& c=calls[element.text];if(!c.count)c.first=ins.word_offset;++c.count;
                }
                std::cout<<"SCRIPT\t"<<name<<'\t'<<e.name<<'\t'<<e.offset<<'\t'<<e.size<<'\t'<<script.instructions.size()<<'\n';
                for(const auto& [fn,c]:calls)std::cout<<"CALL\t"<<name<<'\t'<<e.name<<'\t'<<fn<<'\t'<<c.count<<'\t'<<c.first<<'\n';
            }
        }
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

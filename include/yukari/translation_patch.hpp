#pragma once
#include "yukari/formats.hpp"
#include <fstream>
#include <sstream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>

namespace yukari {
// UTF-8 strings encoded as hex keep tabs, newlines and script controls lossless.
class TranslationPatch {
    struct Entry { std::string source,target; };
    std::map<std::pair<std::string,uint32_t>,Entry> entries;
    std::optional<std::string> target_encoding;
    static std::string key(std::string s){for(char& c:s){if(c=='/')c='\\';if(c>='A'&&c<='Z')c+=32;}return s;}
    static std::string unhex(const std::string& s){
        if(s.size()%2)throw std::runtime_error("invalid translation_patch hex");
        auto digit=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("invalid translation_patch hex");};
        std::string out;for(size_t i=0;i<s.size();i+=2)out+=char(digit(s[i])*16+digit(s[i+1]));return out;
    }
public:
    explicit TranslationPatch(const std::filesystem::path& path={}){
        if(path.empty())return;
        std::ifstream file(path);std::string line;
        if(!std::getline(file,line)||line!="YUKARI_TRANSLATION_PATCH_1")throw std::runtime_error("invalid translation_patch header");
        while(std::getline(file,line)){
            if(line.rfind("@encoding\t",0)==0){
                if(target_encoding)throw std::runtime_error("duplicate translation_patch encoding");
                const auto value=line.substr(10);
                if(value!="CP932"&&value!="CP936")throw std::runtime_error("invalid translation_patch encoding");
                target_encoding=value;continue;
            }
            std::istringstream row(line);std::string script,offset,source,target,extra;
            if(!std::getline(row,script,'\t')||!std::getline(row,offset,'\t')||!std::getline(row,source,'\t')||!std::getline(row,target,'\t')||std::getline(row,extra,'\t'))throw std::runtime_error("invalid translation_patch row");
            size_t used=0;auto at=std::stoul(offset,&used,16);if(used!=offset.size()||at>UINT32_MAX)throw std::runtime_error("invalid translation_patch offset");
            if(!entries.emplace(std::make_pair(key(unhex(script)),uint32_t(at)),Entry{unhex(source),unhex(target)}).second)throw std::runtime_error("duplicate translation_patch entry");
        }
        if(!file.eof())throw std::runtime_error("cannot read translation_patch");
    }
    size_t size()const{return entries.size();}
    const std::optional<std::string>& encoding()const{return target_encoding;}
    size_t verify(const Script& script,const std::string& name)const{
        std::set<uint32_t> matched;
        for(const auto& element:script.elements)if(element.type==5){auto it=entries.find({key(name),element.b});if(it!=entries.end()){
            if(element.text!=it->second.source)throw std::runtime_error("translation_patch source mismatch: "+name);
            matched.insert(element.b);
        }}return matched.size();
    }
    Script apply(Script script,const std::string& name)const{
        verify(script,name);
        for(auto& element:script.elements)if(element.type==5){auto it=entries.find({key(name),element.b});if(it!=entries.end()){
            element.source_text=element.text;
            element.text=it->second.target;
        }}return script;
    }
};

inline std::filesystem::path find_translation_patch(const std::filesystem::path& game_directory,const std::filesystem::path& explicit_path={}) {
    if(!explicit_path.empty())return explicit_path;
    const auto candidate=game_directory/"translation_patch";
    return std::filesystem::is_regular_file(candidate)?candidate:std::filesystem::path{};
}
}

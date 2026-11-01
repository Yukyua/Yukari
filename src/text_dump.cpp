#include "yukari/text_dump.hpp"
#include "yukari/formats.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace yukari {
namespace {
std::string lower(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return s;}
bool suffix(const std::string& s,const std::string& tail){return s.size()>=tail.size()&&s.compare(s.size()-tail.size(),tail.size(),tail)==0;}
std::string hex(uint64_t n,unsigned width){std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(width)<<n;return out.str();}
std::string quoted(const std::string& s){
    std::string out="\"";for(unsigned char c:s){switch(c){
    case '"':out+="\\\"";break;case '\\':out+="\\\\";break;
    case '\n':out+="\\n";break;case '\r':out+="\\r";break;case '\t':out+="\\t";break;
    default:if(c<32)out+="\\u00"+hex(c,2);else out+=char(c);
    }}return out+'"';
}
void csv_row(std::ostream& out,const std::vector<std::string>& cells){
    for(size_t i=0;i<cells.size();++i){if(i)out<<',';out<<'"';for(char c:cells[i]){if(c=='"')out<<'"';out<<c;}out<<'"';}out<<'\n';
}
std::string kind(const std::string& function,const std::string& text){
    if(function.rfind("StrOut",0)==0)return "dialogue";
    if(function=="NameOut")return "name";
    if(function=="Case"||function=="Case2"||function=="IndicateString"||function=="IndicateStringC")return "choice";
    if(function=="Index")return "chapter";
    if(function=="WindowNameSet"||function=="PopMenu"||function=="MessageBox"||function=="StrIn")return "ui";
    if(function=="DefaultFontSet")return "font";
    const auto normalized=lower(text);
    if(suffix(normalized,".yks"))return "script";
    for(const char* extension:{".ykg",".png",".bmp",".ogg",".wav",".mp3",".mid",".avi",".mpg"})
        if(suffix(normalized,extension))return "resource";
    if(function=="=")return "expression";
    return "other";
}
struct Use {uint32_t word,argument,element;std::string function;};
struct Literal {std::vector<uint32_t> elements;std::vector<Use> uses;std::set<std::string> kinds;};
std::string numbers(const std::vector<uint32_t>& values){std::ostringstream out;out<<'[';for(size_t i=0;i<values.size();++i){if(i)out<<',';out<<values[i];}out<<']';return out.str();}
std::string uses_json(const std::vector<Use>& uses){
    std::ostringstream out;out<<'[';for(size_t i=0;i<uses.size();++i){const auto& u=uses[i];if(i)out<<',';
        out<<"{\"word\":"<<u.word<<",\"argument\":"<<u.argument<<",\"element\":"<<u.element<<",\"function\":"<<quoted(u.function)<<'}';
    }out<<']';return out.str();
}
std::string kinds_json(const std::set<std::string>& values){std::string out="[";for(const auto& value:values){if(out.size()>1)out+=',';out+=quoted(value);}return out+']';}
uint64_t digest(const Bytes& bytes){uint64_t hash=14695981039346656037ULL;for(auto b:bytes)hash=(hash^b)*1099511628211ULL;return hash;}
bool within(const std::filesystem::path& path,const std::filesystem::path& directory){
    auto p=path.begin(),d=directory.begin();for(;d!=directory.end();++d,++p)if(p==path.end()||*p!=*d)return false;return true;
}
struct NewDirectory {
    std::filesystem::path path;bool complete=false;
    explicit NewDirectory(std::filesystem::path p):path(std::move(p)){
        if(std::filesystem::exists(path))throw std::runtime_error("text output directory already exists: "+path.string());
        std::filesystem::create_directories(path.parent_path());
        if(!std::filesystem::create_directory(path))throw std::runtime_error("cannot create text output directory");
    }
    ~NewDirectory(){if(!complete){std::error_code error;std::filesystem::remove_all(path,error);}}
};
}
void dump_game_text(const std::filesystem::path& directory,const std::filesystem::path& output,std::ostream& log){
    const auto game=std::filesystem::weakly_canonical(std::filesystem::absolute(directory));
    const auto destination=std::filesystem::weakly_canonical(std::filesystem::absolute(output));
    if(within(destination,game))throw std::runtime_error("text output must be outside the source game directory");
    Resources resources(game);auto entries=resources.entries();
    entries.erase(std::remove_if(entries.begin(),entries.end(),[](const auto& e){return !suffix(lower(e.entry.name),".yks");}),entries.end());
    std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return lower(a.entry.name)<lower(b.entry.name);});
    if(entries.empty())throw std::runtime_error("no YKS scripts found in original resources");
    NewDirectory owner(destination);
    std::ofstream json(destination/"texts.jsonl",std::ios::binary),csv(destination/"translation.csv",std::ios::binary),manifest(destination/"manifest.json",std::ios::binary);
    if(!json||!csv||!manifest)throw std::runtime_error("cannot create text export files");
    csv<<"\xef\xbb\xbf";
    csv_row(csv,{"id","archive","script","data_offset","elements","kinds","uses","source","translation","source_hex"});
    manifest<<"{\"format\":\"yukari-text-dump\",\"version\":1,\"encoding\":\"CP932\",\"hash_algorithm\":\"fnv1a64\",\"resource_policy\":\"original-effective\",\"scripts\":[";
    uint64_t strings=0,occurrences=0;size_t scripts=0;
    for(const auto& resource:entries){const auto& name=resource.entry.name;
        try{
            const auto bytes=resources.read(name);const auto script=parse_script(bytes);const auto hash=hex(digest(bytes),16);
            std::map<uint32_t,Literal> literals;
            for(size_t i=0;i<script.elements.size();++i)if(script.elements[i].type==5)literals[script.elements[i].b].elements.push_back(uint32_t(i));
            for(const auto& ins:script.instructions){const auto& function=script.elements[ins.element];if(function.type!=0)continue;
                for(size_t i=0;i<ins.arguments.size();++i){const auto index=ins.arguments[i];const auto& e=script.elements[index];if(e.type!=5)continue;
                    auto& item=literals.at(e.b);item.uses.push_back({ins.word_offset,uint32_t(i),index,function.text});item.kinds.insert(kind(function.text,e.text));
                }
            }
            uint64_t script_uses=0;
            for(auto& [offset,item]:literals){
                const auto& text=script.elements[item.elements.front()].text;
                auto first=script.data.begin()+offset,last=std::find(first,script.data.end(),uint8_t(0));
                std::string raw;raw.reserve(size_t(last-first)*2);constexpr char digits[]="0123456789abcdef";for(auto i=first;i!=last;++i){raw+=digits[*i>>4];raw+=digits[*i&15];}
                if(item.kinds.empty())item.kinds.insert("unused");
                const auto id=lower(name)+"#str:"+hex(offset,8),elements=numbers(item.elements),uses=uses_json(item.uses);
                const uint64_t file_offset=uint64_t(u32(bytes,32))+offset;
                json<<"{\"id\":"<<quoted(id)<<",\"archive\":"<<quoted(resource.archive)<<",\"script\":"<<quoted(name)
                    <<",\"script_hash\":"<<quoted(hash)<<",\"data_offset\":"<<offset<<",\"file_offset\":"<<file_offset
                    <<",\"archive_offset\":"<<uint64_t(resource.entry.offset)+file_offset<<",\"elements\":"<<elements
                    <<",\"kinds\":"<<kinds_json(item.kinds)<<",\"uses\":"<<uses<<",\"source\":"<<quoted(text)
                    <<",\"translation\":\"\",\"source_hex\":"<<quoted(raw)<<"}\n";
                std::string categories;for(const auto& k:item.kinds){if(!categories.empty())categories+='|';categories+=k;}
                csv_row(csv,{id,resource.archive,name,std::to_string(offset),elements,categories,uses,text,"",raw});
                script_uses+=item.uses.size();
            }
            if(scripts)manifest<<',';
            manifest<<"{\"archive\":"<<quoted(resource.archive)<<",\"script\":"<<quoted(name)<<",\"bytes\":"<<bytes.size()
                <<",\"hash\":"<<quoted(hash)<<",\"encrypted\":"<<(script.encrypted?"true":"false")<<",\"strings\":"<<literals.size()<<",\"uses\":"<<script_uses<<'}';
            ++scripts;strings+=literals.size();occurrences+=script_uses;
        }catch(const std::exception& e){throw std::runtime_error("dumping "+resource.archive+":"+name+": "+e.what());}
    }
    manifest<<"],\"script_count\":"<<scripts<<",\"string_count\":"<<strings<<",\"use_count\":"<<occurrences<<"}\n";
    json.close();csv.close();manifest.close();if(!json||!csv||!manifest)throw std::runtime_error("cannot finish text export");
    std::ofstream readme(destination/"README.txt",std::ios::binary);
    readme<<"Yukari static text dump v1 (original effective YKC resources, CP932 -> UTF-8)\n"
        <<"texts.jsonl: one record per type-5 string data offset; UTF-8, no BOM.\n"
        <<"translation.csv: same records; UTF-8 BOM; fill the translation column.\n"
        <<"Repeated text at different offsets remains separate. Shared offsets list every element/use.\n"
        <<"Uses contain instruction word, zero-based argument index, element and function.\n"
        <<"Keep source text, line breaks, Ruby/control codes and IDs unchanged. Categories are hints only.\n"
        <<"source_hex is decrypted CP932 without the terminating NUL; file offsets are inside the YKS.\n"
        <<"manifest.json lists source hashes, script inventory and counts (FNV-1a is not cryptographic).\n"
        <<"Includes unused literals, technical strings and resource paths. Review kinds before translating.\n"
        <<"Does not execute scripts, resolve dynamic variables, OCR image text or import translations.\n";
    readme.close();if(!readme)throw std::runtime_error("cannot finish text export README");
    owner.complete=true;
    log<<"DUMP_TEXT scripts="<<scripts<<" strings="<<strings<<" uses="<<occurrences<<" output="<<destination.string()<<'\n';
}
}

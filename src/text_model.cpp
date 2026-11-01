#include "yukari/text_model.hpp"
#include "yukari/original_scene.hpp"
#include <algorithm>
#include <cstdlib>
#include <cerrno>
#include <exception>
#include <climits>
#include <stdexcept>
namespace yukari {
namespace {
int32_t integer(const Value& v){if(auto p=std::get_if<int32_t>(&v))return *p;throw std::runtime_error("text instruction requires integer");}
const std::string& string(const Value& v){if(auto p=std::get_if<std::string>(&v))return *p;throw std::runtime_error("text instruction requires string");}
void minimum(const std::vector<Value>& a,size_t n){if(a.size()<n)throw std::runtime_error("text instruction missing required arguments");}
int32_t subtract(int32_t a,int32_t b){uint32_t bits=uint32_t(a)-uint32_t(b);int32_t n;std::memcpy(&n,&bits,4);return n;}
std::string nul_prefix(std::string s){auto n=s.find('\0');if(n!=std::string::npos)s.resize(n);return s;}
}
TextModel::TextModel(TextModelHooks h,std::string e):hooks_(std::move(h)),encoding_(std::move(e)){}
void TextModel::set_encoding(std::string encoding){
    if(encoding==encoding_)return;
    TextModel candidate=*this;
    auto convert=[&](std::string& value){
        if(!value.empty())value=encode_original_text(decode(value,encoding_.c_str()),encoding.c_str());
    };
    convert(candidate.rules_.kinsoku_pairs);convert(candidate.rules_.word_run_pairs);
    convert(candidate.rules_.suppress_after_rollback);convert(candidate.rules_.indent_space);
    for(auto& value:candidate.rules_.hanging_prefixes)convert(value);
    for(auto& [id,layer]:candidate.layers_){
        (void)id;convert(layer.pending_output);
        for(auto& record:layer.records)if(record.kind==3)convert(record.encoded);
    }
    candidate.encoding_=std::move(encoding);
    *this=std::move(candidate);
}
TextLayerState& TextModel::layer(int32_t id){return layers_[id];}
void TextModel::metrics(TextLayerState& l){auto f=fonts_.find(l.font);if(f==fonts_.end())throw std::runtime_error("text layout needs configured font metrics");l.flow.cell_width=f->second.cell_width;l.flow.line_height=f->second.line_height;}
void TextModel::glyph(int32_t id,const std::string& bytes,bool dbcs,int32_t w,int32_t h){
    if(bytes.size()!=(dbcs?2u:1u)||bytes[0]=='\0')throw std::runtime_error("glyph requires one source-codepage character");
    auto& l=layers_.at(id);metrics(l);
    l.records.push_back({3,l.flow.x,l.flow.y,w,h,l.font,0,bytes,l.style_token});l.flow.glyph(dbcs,w);
}
void TextModel::line_feed(int32_t id){auto& l=layers_.at(id);metrics(l);if(l.flow.lf())l.records.push_back({7,l.flow.x,l.flow.y,0,0,l.font,0,{},l.style_token});}
void TextModel::graphic_font(int32_t id,int32_t image,int32_t w,int32_t h){
    auto& l=layers_.at(id);metrics(l);l.records.push_back({5,l.flow.x,l.flow.y,w,h,l.font,image,{},l.style_token});
    auto& f=l.flow;const auto dx=std::max(f.cell_width,w),dy=std::max(f.line_height,h);
    f.x=TextFlow::add(f.x,dx);f.anchor_x=f.x;f.anchor_y=f.y;
    if((f.top==0&&f.bottom==0&&f.left==0&&f.right==0)||TextFlow::add(f.x,dx)<f.right)return;
    f.x=f.left;f.y=TextFlow::add(f.y,dy);if(f.bottom<=TextFlow::add(f.y,dy))f.overflow=true;
}
size_t TextModel::output_step(int32_t id,const std::string& input){
    auto bytes=nul_prefix(input);if(bytes.empty())return 0;
    auto& l=layers_.at(id);
    struct PendingGuard {TextLayerState& l;bool existed;int exceptions=std::uncaught_exceptions();~PendingGuard(){if(!existed&&std::uncaught_exceptions()>exceptions){l.output_prepared=false;l.rollback_active=false;l.pending_output.clear();}}} pending_guard{l,l.output_prepared};
    if(!hooks_.dbcs_lead||!hooks_.measure)throw std::runtime_error("text output requires codepage and glyph metrics providers");
    auto span=[&](size_t at){const size_t n=hooks_.dbcs_lead(uint8_t(bytes.at(at)))?2:1;if(at+n>bytes.size())throw std::runtime_error("truncated source-codepage glyph");return n;};
    auto pair_in=[&](const std::string& list,size_t at){if(at+2>bytes.size())return false;for(size_t i=0;i+1<list.size();i+=2)if(bytes.compare(at,2,list,i,2)==0)return true;return false;};
    if(l.output_prepared){if(l.pending_output!=bytes)throw std::runtime_error("text page continuation must retry the same pending source");}
    else {
        l.rollback_active=false;metrics(l);
        if(l.flow.anchor_x!=l.flow.x&&hooks_.dbcs_lead(uint8_t(bytes[0]))&&pair_in(rules_.kinsoku_pairs,0)&&!pair_in(rules_.kinsoku_pairs,2)){
            l.flow.x=l.flow.anchor_x;l.flow.y=l.flow.anchor_y;l.flow.overflow=false;l.rollback_active=true;
        }
        if(rules_.protect_word_runs&&l.flow.x!=l.flow.left&&bytes.size()>5&&bytes.size()<255&&
           !pair_in(rules_.word_run_pairs,0)&&pair_in(rules_.word_run_pairs,2)&&pair_in(rules_.word_run_pairs,4)){
            size_t end=2;while(pair_in(rules_.word_run_pairs,end))end+=2;
            auto extent=measure(l.font,bytes.substr(0,end));if(l.flow.right<=TextFlow::add(l.flow.x,extent.first))line_feed(id);
        }
        if(!l.rollback_active&&l.flow.x==l.flow.left){
            auto prefix=std::find_if(rules_.hanging_prefixes.begin(),rules_.hanging_prefixes.end(),[&](const std::string& p){return bytes.compare(0,p.size(),p)==0;});
            if(l.flow.y==l.flow.top){
                if(prefix!=rules_.hanging_prefixes.end())l.hanging_indent=int32_t((uint32_t(prefix->size())*uint32_t(l.flow.cell_width))>>1);
            }else if(prefix==rules_.hanging_prefixes.end()&&l.flow.cell_width!=0){
                const auto count=int64_t(l.hanging_indent)/int64_t(l.flow.cell_width);
                if(count>65536)throw std::runtime_error("hanging indent exceeds safe native profile");
                if(count>0&&rules_.indent_space.empty())throw std::runtime_error("hanging indent requires configured native space glyph");
                for(int32_t k=0;k<count;++k){const auto& ch=rules_.indent_space;const bool dbcs=hooks_.dbcs_lead(uint8_t(ch[0]));auto size=(hooks_.glyph_size?hooks_.glyph_size:hooks_.measure)(l.font,ch);glyph(id,ch,dbcs,size.first,size.second);}
            }
        }
        l.output_prepared=true;l.pending_output=bytes;
    }
    if(l.flow.overflow)return 0;
    auto complete=[&](size_t consumed){
        if(l.rollback_active&&consumed<bytes.size()&&hooks_.dbcs_lead(uint8_t(bytes[consumed]))){
            size_t table=0;
            while(consumed+1<bytes.size()){
                while(table+1<rules_.suppress_after_rollback.size()&&bytes.compare(consumed,2,rules_.suppress_after_rollback,table,2)!=0)table+=2;
                if(table+1>=rules_.suppress_after_rollback.size())break;consumed+=2;
            }
        }
        l.output_prepared=false;l.rollback_active=false;l.pending_output.clear();return consumed;
    };
    if(bytes.rfind("@gf(",0)==0||bytes.rfind("@of(",0)==0){
        const bool original=bytes[1]=='o';
        size_t end=4;while(end<bytes.size()&&bytes[end]!=')'&&bytes[end]!='\r'&&bytes[end]!='\n')end+=span(end);
        if(end==bytes.size()||bytes[end]!=')'||end>=256)throw std::runtime_error("malformed graphic-font control");
        errno=0;auto n=std::strtol(bytes.substr(4,end-4).c_str(),nullptr,0);
        if(errno==ERANGE||n<INT_MIN||n>INT_MAX)throw std::runtime_error("graphic-font control id overflow");
        if(n>=0){auto& provider=original?hooks_.original_font_size:hooks_.graphic_font_size;if(!provider)throw std::runtime_error("inline font needs actual image metrics provider");auto size=provider(int32_t(n));graphic_font(id,int32_t(n),size.first,size.second);if(original)l.records.back().kind=4;}return complete(end+1);
    }
    if(bytes.rfind("@r(",0)==0||bytes.rfind("@m(",0)==0){
        const bool ruby=bytes[1]=='r';size_t end=3;
        while(end<bytes.size()&&bytes[end]!=')'&&bytes[end]!='\r'&&bytes[end]!='\n')end+=span(end);
        if(end==bytes.size()||bytes[end]!=')'||end>=256)throw std::runtime_error("malformed ruby/keep-together control");
        auto body=bytes.substr(3,end-3);const auto comma=body.find(',');
        if(ruby&&comma==std::string::npos)throw std::runtime_error("ruby control needs count,text");
        errno=0;const auto count=std::strtol(body.c_str(),nullptr,0);
        if(errno==ERANGE||count<INT_MIN||count>INT_MAX)throw std::runtime_error("text control count overflow");
        if(!ruby){
            std::string upcoming;size_t at=end+1;
            // Original434af0 repeatedly tests the FIRST upcoming byte's DBCS
            // class, rather than each current byte. Preserve this native quirk.
            const size_t width=at<bytes.size()&&hooks_.dbcs_lead(uint8_t(bytes[at]))?2:1;
            for(long k=0;k<count&&at<bytes.size();++k){if(at+width>bytes.size())throw std::runtime_error("truncated @m source run");upcoming+=bytes.substr(at,width);at+=width;if(upcoming.size()>=256)throw std::runtime_error("@m source run exceeds native buffer");}
            auto size=measure(l.font,upcoming);if(l.flow.right<=TextFlow::add(l.flow.x,size.first))line_feed(id);return complete(end+1);
        }
        const auto ruby_text=body.substr(comma+1);if(ruby_text.empty())throw std::runtime_error("empty ruby is outside verified safe native profile");
        const auto size=measure(ruby_font_,ruby_text);metrics(l);const auto cell=l.flow.cell_width?l.flow.cell_width:l.flow.line_height;
        const auto saved=l.flow;const auto font=l.font;
        const uint32_t product=uint32_t(cell)*uint32_t(int32_t(count));int32_t signed_product;std::memcpy(&signed_product,&product,4);
        l.flow.x=TextFlow::add(l.flow.x,TextFlow::add(-(size.first/2),-(signed_product/2)));l.flow.y=subtract(l.flow.y,size.second);
        l.font=ruby_font_;
        try{for(size_t at=0;at<ruby_text.size();){const size_t n=hooks_.dbcs_lead(uint8_t(ruby_text[at]))?2:1;if(at+n>ruby_text.size())throw std::runtime_error("truncated ruby glyph");auto ch=ruby_text.substr(at,n);auto metric=(hooks_.glyph_size?hooks_.glyph_size:hooks_.measure)(ruby_font_,ch);glyph(id,ch,n==2,metric.first,metric.second);at+=n;}}
        catch(...){l.font=font;l.flow.x=saved.x;l.flow.y=saved.y;l.flow.anchor_x=saved.anchor_x;l.flow.anchor_y=saved.anchor_y;l.flow.cell_width=saved.cell_width;l.flow.line_height=saved.line_height;throw;}
        l.font=font;l.flow.x=saved.x;l.flow.y=saved.y;l.flow.anchor_x=saved.anchor_x;l.flow.anchor_y=saved.anchor_y;l.flow.cell_width=saved.cell_width;l.flow.line_height=saved.line_height;return complete(end+1);
    }
    if(bytes.rfind("@r",0)==0||bytes.rfind("@m",0)==0||bytes.rfind("@of",0)==0)
        throw std::runtime_error("unverified/malformed original text control");
    auto n=span(0);auto character=bytes.substr(0,n);auto size=(hooks_.glyph_size?hooks_.glyph_size:hooks_.measure)(l.font,character);glyph(id,character,n==2,size.first,size.second);return complete(n);
}
std::pair<int32_t,int32_t> TextModel::measure(int32_t font,const std::string& input)const{
    const auto f=fonts_.find(font);if(f==fonts_.end())throw std::runtime_error("text measure requires configured font");
    auto bytes=nul_prefix(input);if(bytes.empty())return {0,0};
    if(!hooks_.dbcs_lead)throw std::runtime_error("text measure requires codepage provider");
    std::string filtered;
    for(size_t at=0;at<bytes.size();){
        // 405ad0 strips @m/@r through the closing parenthesis, but then
        // includes that closing ')' in the extent string (native quirk).
        if(bytes.compare(at,2,"@m")==0||bytes.compare(at,2,"@r")==0){at+=2;if(at<bytes.size()&&bytes[at]=='('){while(at<bytes.size()&&bytes[at]!=')'){auto n=hooks_.dbcs_lead(uint8_t(bytes[at]))?2u:1u;at+=n;}}if(at>=bytes.size())throw std::runtime_error("unterminated measured text control");}
        const size_t n=hooks_.dbcs_lead(uint8_t(bytes[at]))?2:1;if(at+n>bytes.size())throw std::runtime_error("truncated measured glyph");filtered+=bytes.substr(at,n);at+=n;
    }
    if(f->second.cell_width>0){uint32_t bits=uint32_t(f->second.cell_width)*uint32_t(filtered.size());int32_t width;std::memcpy(&width,&bits,4);return {width/2,f->second.line_height};}
    if(!hooks_.measure)throw std::runtime_error("proportional text needs actual platform extent provider");return hooks_.measure(font,filtered);
}
void TextModel::name_out(const std::string& utf8,bool initial){
    auto found=layers_.find(name_window_);if(found==layers_.end())throw std::runtime_error("NameOut name layer not found");
    auto bytes=nul_prefix(encode_original_text(utf8,encoding_.c_str()));auto& flow=found->second.flow;
    if(initial){const auto extent=measure(found->second.font,bytes);const auto dx=subtract(subtract(flow.right,flow.x),extent.first)/2;flow.x=TextFlow::add(flow.x,dx);flow.anchor_x=flow.x;}
    for(size_t at=0;at<bytes.size();){const auto n=output_step(name_window_,bytes.substr(at));if(n==0)throw std::runtime_error("NameOut requires runtime page continuation through output_step");at+=n;}
}
void TextModel::reset_cursor(int32_t id){auto& l=layers_.at(id);l.flow.x=l.flow.anchor_x=l.flow.left;l.flow.y=l.flow.anchor_y=l.flow.top;l.hanging_indent=0;}
void TextModel::clear(int32_t id){auto& l=layers_.at(id);reset_cursor(id);l.flow.overflow=false;l.records.erase(std::remove_if(l.records.begin(),l.records.end(),[](const TextRecord& r){return r.kind==3||r.kind==4||r.kind==5||r.kind==7;}),l.records.end());}
std::string TextModel::str_in(int32_t id)const{
    auto found=layers_.find(id);if(found==layers_.end())return {};
    std::string encoded;
    for(const auto& r:found->second.records){if(r.kind==3)encoded+=nul_prefix(r.encoded);else if(r.kind==4||r.kind==5)encoded+=(r.kind==4?"@o(":"@g(")+std::to_string(r.image_id)+")";}
    return decode(encoded,encoding_.c_str());
}
std::optional<Value> TextModel::call(const std::string& n,const std::vector<Value>& a){
    if(n=="DefaultFontSet"){
        if(a.size()!=4&&a.size()!=5&&a.size()!=6&&a.size()!=7)return Value{};
        TextFont f;const auto id=integer(a[0]);f.family=nul_prefix(string(a[1]));f.height=integer(a[2]);
        const bool wide=a.size()==5||a.size()==7;f.width=wide?integer(a[3]):0;f.weight=integer(a[wide?4:3]);
        f.cell_width=f.height;f.line_height=f.width?f.width:f.height;
        if(a.size()>=6){f.cell_width=integer(a[wide?5:4]);f.line_height=integer(a[wide?6:5]);}
        fonts_[id]=std::move(f);return Value{};
    }
    if(n=="RubyFontSet"){if(!a.empty())ruby_font_=integer(a[0]);return Value{ruby_font_};}
    if(n=="FontSet"){minimum(a,2);auto l=layers_.find(integer(a[0]));if(l!=layers_.end())l->second.font=integer(a[1]);return Value{};}
    if(n=="NameWindowSet"){
        minimum(a,1);name_window_=integer(a[0]);auto l=layers_.find(name_window_);
        if(l!=layers_.end()&&a.size()>=5){auto& f=l->second.flow;f.left=integer(a[1]);f.top=integer(a[2]);f.right=integer(a[3]);f.bottom=integer(a[4]);reset_cursor(name_window_);}return Value{};
    }
    if(n=="NameOut"){minimum(a,1);name_out(string(a[0]));return Value{};}
    if(n=="StrIn"){if(a.empty())return Value{};return Value{str_in(integer(a[0]))};}
    if(n=="TextBoxSet"){
        if(a.size()<4)return Value{};
        if(!hooks_.create_box||!hooks_.delete_box)throw std::runtime_error("TextBoxSet requires edit-control UI hooks");
        TextBoxSpec s{integer(a[0]),integer(a[1]),integer(a[2]),integer(a[3]),a.size()>4?integer(a[4]):0};
        if(box_exists_){hooks_.delete_box();box_exists_=false;}hooks_.create_box(s);box_exists_=true;box_spec_=s;box_value_.clear();return Value{};
    }
    if(n=="TextBoxDelete"){if(box_exists_){if(!hooks_.delete_box)throw std::runtime_error("TextBoxDelete missing UI hook");hooks_.delete_box();box_exists_=false;box_spec_={};box_value_.clear();}return Value{};}
    if(n=="TextBoxStringGet"){if(!box_exists_||!hooks_.get_box)throw std::runtime_error("TextBoxStringGet requires live UI control");box_value_=nul_prefix(hooks_.get_box());return Value{box_value_};}
    if(n=="TextBoxStringSet"){
        if(a.empty()||std::holds_alternative<std::monostate>(a[0]))return Value{};
        if(!box_exists_||!hooks_.set_box)throw std::runtime_error("TextBoxStringSet requires live UI control");
        auto text=nul_prefix(string(a[0]));auto bytes=encode_original_text(text,encoding_.c_str());hooks_.set_box(bytes.empty()?std::string{}:text);box_value_=bytes.empty()?std::string{}:text;return Value{};
    }
    if(n=="OriginalFontSet"||n=="GraphicFontSet"){
        minimum(a,1);auto& hook=n=="OriginalFontSet"?hooks_.register_original_font:hooks_.register_graphic_font;
        if(!hook)throw std::runtime_error(n+" requires native font registration provider");auto path=nul_prefix(string(a[0]));hook(path);(n=="OriginalFontSet"?original_font_resource_:graphic_font_resource_)=std::move(path);return Value{};
    }
    return std::nullopt;
}
namespace {
constexpr size_t text_limit=64*1024*1024;
struct TextWriter {
 Bytes bytes;
 void n(uint64_t v,unsigned count=4){if(bytes.size()>text_limit-count)throw std::runtime_error("text snapshot exceeds limit");for(unsigned i=0;i<count;++i)bytes.push_back(uint8_t(v>>(8*i)));}
 void s(const std::string& v){if(v.size()>1024*1024||bytes.size()>text_limit-4-v.size())throw std::runtime_error("text snapshot string exceeds limit");n(v.size());bytes.insert(bytes.end(),v.begin(),v.end());}
};
struct TextReader {
 const Bytes& bytes;size_t at=0;
 uint64_t n(unsigned count=4){require_range(bytes.size(),at,count,"text snapshot number");uint64_t v=0;for(unsigned i=0;i<count;++i)v|=uint64_t(bytes[at++])<<(8*i);return v;}
 int32_t i(){auto bits=uint32_t(n());int32_t v;std::memcpy(&v,&bits,4);return v;}
 std::string s(){auto size=n();if(size>1024*1024)throw std::runtime_error("text snapshot string exceeds limit");require_range(bytes.size(),at,size,"text snapshot string");std::string v(bytes.begin()+at,bytes.begin()+at+size);at+=size;return v;}
 uint32_t count(uint32_t max){auto v=n();if(v>max||v>bytes.size()-at)throw std::runtime_error("invalid text snapshot count");return uint32_t(v);}
};
uint64_t text_hash(const Bytes& b,size_t end){uint64_t h=14695981039346656037ULL;for(size_t i=0;i<end;++i)h=(h^b[i])*1099511628211ULL;return h;}
}
Bytes TextModel::snapshot()const{
 TextWriter w;w.n(0x54584b59);w.n(4);w.s(encoding_);w.n(uint32_t(name_window_));w.n(uint32_t(ruby_font_));w.n(box_exists_?1:0);for(auto n:{box_spec_.value_28,box_spec_.value_2c,box_spec_.value_24,box_spec_.font_height,box_spec_.style})w.n(uint32_t(n));if(box_exists_&&!hooks_.get_box)throw std::runtime_error("text snapshot requires live TextBox read provider");auto box_value=box_exists_?nul_prefix(hooks_.get_box()):std::string{};decode(box_value,"UTF-8");w.s(box_value);w.s(original_font_resource_);w.s(graphic_font_resource_);
 w.s(rules_.kinsoku_pairs);w.s(rules_.word_run_pairs);w.s(rules_.suppress_after_rollback);w.s(rules_.indent_space);w.n(rules_.protect_word_runs?1:0);if(rules_.hanging_prefixes.size()>65536)throw std::runtime_error("text prefix count exceeds limit");w.n(rules_.hanging_prefixes.size());for(const auto& p:rules_.hanging_prefixes)w.s(p);
 if(fonts_.size()>65536||layers_.size()>65536)throw std::runtime_error("text snapshot font/layer limit");
 w.n(fonts_.size());for(const auto& pair:fonts_){w.n(uint32_t(pair.first));const auto& f=pair.second;w.s(f.family);for(auto n:{f.height,f.width,f.weight,f.cell_width,f.line_height})w.n(uint32_t(n));}
 w.n(layers_.size());for(const auto& pair:layers_){w.n(uint32_t(pair.first));const auto& l=pair.second;const auto& f=l.flow;for(auto n:{f.x,f.y,f.anchor_x,f.anchor_y,f.left,f.top,f.right,f.bottom,f.cell_width,f.line_height})w.n(uint32_t(n));w.n(f.overflow?1:0);w.n(uint32_t(l.font));w.n(uint32_t(l.hanging_indent));w.n(l.style_token,8);w.n(l.output_prepared?1:0);w.n(l.rollback_active?1:0);w.s(l.pending_output);
 if(l.records.size()>1000000)throw std::runtime_error("text snapshot record limit");w.n(l.records.size());for(const auto& r:l.records){for(auto n:{r.kind,r.x,r.y,r.width,r.height,r.font,r.image_id})w.n(uint32_t(n));w.s(r.encoded);w.n(r.style_token,8);w.n(r.history_recorded?1:0);}}
 const auto hash=text_hash(w.bytes,w.bytes.size());w.n(hash,8);return w.bytes;
}
void TextModel::restore(const Bytes& bytes){
 if(bytes.size()<16||bytes.size()>text_limit)throw std::runtime_error("invalid text snapshot size");
 TextReader hash_reader{bytes,bytes.size()-8};if(hash_reader.n(8)!=text_hash(bytes,bytes.size()-8))throw std::runtime_error("text snapshot checksum mismatch");
 TextReader r{bytes};if(r.n()!=0x54584b59)throw std::runtime_error("unsupported text snapshot");const auto version=r.n();if(version!=3&&version!=4)throw std::runtime_error("unsupported text snapshot");
 auto encoding=r.s();if(encoding.empty()||encoding.find('\0')!=std::string::npos)throw std::runtime_error("invalid text snapshot encoding");TextModel candidate(hooks_,encoding);candidate.name_window_=r.i();candidate.ruby_font_=r.i();auto box=r.n();if(box>1)throw std::runtime_error("invalid text snapshot boolean");candidate.box_exists_=box!=0;if(version>=4){candidate.box_spec_.value_28=r.i();candidate.box_spec_.value_2c=r.i();candidate.box_spec_.value_24=r.i();candidate.box_spec_.font_height=r.i();candidate.box_spec_.style=r.i();candidate.box_value_=r.s();if(candidate.box_value_.find('\0')!=std::string::npos)throw std::runtime_error("invalid TextBox snapshot value");}else if(box)throw std::runtime_error("legacy text snapshot has active TextBox but omits its geometry/value");candidate.original_font_resource_=r.s();candidate.graphic_font_resource_=r.s();
 candidate.rules_.kinsoku_pairs=r.s();candidate.rules_.word_run_pairs=r.s();candidate.rules_.suppress_after_rollback=r.s();candidate.rules_.indent_space=r.s();auto protect=r.n();if(protect>1)throw std::runtime_error("invalid text rules boolean");candidate.rules_.protect_word_runs=protect!=0;for(auto count=r.count(65536);count--;)candidate.rules_.hanging_prefixes.push_back(r.s());
 for(auto count=r.count(65536);count--;){auto id=r.i();TextFont f;f.family=r.s();f.height=r.i();f.width=r.i();f.weight=r.i();f.cell_width=r.i();f.line_height=r.i();if(!candidate.fonts_.emplace(id,std::move(f)).second)throw std::runtime_error("duplicate text snapshot font");}
 for(auto count=r.count(65536);count--;){auto id=r.i();TextLayerState l;auto& f=l.flow;f.x=r.i();f.y=r.i();f.anchor_x=r.i();f.anchor_y=r.i();f.left=r.i();f.top=r.i();f.right=r.i();f.bottom=r.i();f.cell_width=r.i();f.line_height=r.i();auto overflow=r.n();if(overflow>1)throw std::runtime_error("invalid text overflow boolean");f.overflow=overflow!=0;l.font=r.i();l.hanging_indent=r.i();l.style_token=r.n(8);auto prepared=r.n(),rollback=r.n();if(prepared>1||rollback>1)throw std::runtime_error("invalid text continuation boolean");l.output_prepared=prepared!=0;l.rollback_active=rollback!=0;l.pending_output=r.s();if(l.output_prepared!=!l.pending_output.empty()||(!l.output_prepared&&l.rollback_active))throw std::runtime_error("invalid text pending output");
 for(auto records=r.count(1000000);records--;){TextRecord rec;rec.kind=r.i();rec.x=r.i();rec.y=r.i();rec.width=r.i();rec.height=r.i();rec.font=r.i();rec.image_id=r.i();rec.encoded=r.s();rec.style_token=r.n(8);auto recorded=r.n();if(recorded>1)throw std::runtime_error("invalid text history marker");rec.history_recorded=recorded!=0;l.records.push_back(std::move(rec));}
 if(!candidate.layers_.emplace(id,std::move(l)).second)throw std::runtime_error("duplicate text snapshot layer");}
 if(r.at!=bytes.size()-8)throw std::runtime_error("trailing text snapshot data");
 if(candidate.box_exists_&&(!hooks_.create_box||!hooks_.delete_box||!hooks_.set_box||!hooks_.get_box))throw std::runtime_error("restoring TextBox requires complete live UI providers");
 if(box_exists_&&!hooks_.delete_box)throw std::runtime_error("restoring text snapshot cannot destroy existing TextBox without UI provider");
 const auto old_value=box_exists_&&hooks_.get_box?nul_prefix(hooks_.get_box()):box_value_;
 bool touched=false;
 try{
  if(candidate.box_exists_){decode(candidate.box_value_,"UTF-8");touched=true;hooks_.create_box(candidate.box_spec_);hooks_.set_box(candidate.box_value_);}
  else if(box_exists_){touched=true;hooks_.delete_box();}
 }catch(const std::exception& failure){
  const std::string reason=failure.what();
  if(touched){try{if(box_exists_){hooks_.create_box(box_spec_);hooks_.set_box(old_value);}else if(hooks_.delete_box)hooks_.delete_box();}catch(const std::exception& rollback){throw std::runtime_error("TextBox restore failed: "+reason+"; old UI rollback also failed: "+rollback.what());}}
  throw;
 }
 *this=std::move(candidate);
}

}

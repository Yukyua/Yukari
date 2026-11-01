#include "yukari/save_preview.hpp"
#include "yukari/original_scene.hpp"
#include <cstdio>
#include <stdexcept>
namespace yukari {
namespace {
int integer(const Value& value){if(auto p=std::get_if<int32_t>(&value))return *p;throw std::runtime_error("save preview setting is not an integer");}
int setting(const SettingsState& s,const std::string& name,int fallback){auto p=s.values.find(name);return p==s.values.end()?fallback:integer(p->second);}
std::string string_setting(const SettingsState& s,const std::string& name,const std::string& fallback){auto p=s.values.find(name);if(p==s.values.end())return fallback;if(auto value=std::get_if<std::string>(&p->second))return *value;throw std::runtime_error("save preview setting is not a string");}
struct Font {int font=9,effect=2;uint32_t color=0x303030;};
Font font(const SettingsState& s,const std::string& name,Font fallback,bool inherit){
    auto p=s.records.find(name);if(p==s.records.end())return fallback;
    const auto& record=p->second;
    if(!record.empty()){int n=integer(record[0]);if(!inherit||n>=0)fallback.font=n;}
    if(record.size()>1){int n=integer(record[1]);if(!inherit||n>=0)fallback.effect=n;}
    if(record.size()>2){uint32_t n=uint32_t(integer(record[2]));if(!inherit||n!=0xffffffff)fallback.color=n;}
    return fallback;
}
std::vector<int> date_fields(const SettingsState& s,const SavePreviewDate& date){
    std::vector<int> codes={1,3,4,12,13};auto p=s.records.find("SaveDataDisplaySetToDateFields");
    if(p!=s.records.end()){codes.clear();if(p->second.size()>16)throw std::runtime_error("native date field table exceeds 16 entries");for(const auto& value:p->second)codes.push_back(integer(value));}
    std::vector<int> fields;for(int code:codes){if(!code)break;switch(code){case 1:fields.push_back(date.year);break;case 2:fields.push_back(date.year%100);break;case 3:fields.push_back(date.month);break;case 4:fields.push_back(date.day);break;case 11:fields.push_back(date.hour%12);break;case 12:fields.push_back(date.hour);break;case 13:fields.push_back(date.minute);break;case 14:fields.push_back(date.second);break;default:throw std::runtime_error("native date field selects uninitialized original stack data");}}
    return fields;
}
std::string date_format(const std::string& format,const std::vector<int>& fields){
    // Original uses sprintf with integers, not strftime. Parse conversions so
    // accidental pointer/%n formats cannot escape the integer field contract.
    if(format.size()>255)throw std::runtime_error("native date format exceeds 255 bytes");
    std::string result;size_t field=0;
    for(size_t at=0;at<format.size();){
        if(format[at]!='%'){result+=format[at++];continue;}
        const size_t start=at++;if(at<format.size()&&format[at]=='%'){result+='%';++at;continue;}
        while(at<format.size()&&std::string("-+ #0").find(format[at])!=std::string::npos)++at;
        unsigned width=0;while(at<format.size()&&format[at]>='0'&&format[at]<='9'){width=width*10+unsigned(format[at++]-'0');if(width>255)throw std::runtime_error("native date width exceeds buffer");}
        if(at<format.size()&&format[at]=='.'){++at;unsigned precision=0;while(at<format.size()&&format[at]>='0'&&format[at]<='9'){precision=precision*10+unsigned(format[at++]-'0');if(precision>255)throw std::runtime_error("native date precision exceeds buffer");}}
        std::string conversion=format.substr(start,at-start);
        if(at<format.size()&&format[at]=='l')++at; // Windows long remains32-bit.
        if(at>=format.size()||std::string("diuoxX").find(format[at])==std::string::npos)throw std::runtime_error("unsupported non-integer native date conversion");
        const char type=format[at++];conversion+=type;if(field>=fields.size())throw std::runtime_error("native date format reads absent field");
        char buffer[512];int count=(type=='d'||type=='i')?std::snprintf(buffer,sizeof buffer,conversion.c_str(),fields[field]):std::snprintf(buffer,sizeof buffer,conversion.c_str(),unsigned(fields[field]));++field;
        if(count<0||size_t(count)>=sizeof buffer||result.size()+size_t(count)>255)throw std::runtime_error("native formatted date exceeds buffer");result.append(buffer,size_t(count));
    }
    if(result.size()>255)throw std::runtime_error("native formatted date exceeds buffer");return result;
}
}
SavePreviewLayout save_preview_layout(const SettingsState& s,const SavePreviewMetadata& m,int x,int y,const SavePreviewOptions& options){
    SavePreviewLayout out;out.parent_x=x;out.parent_y=y;if(!m.exists)return out;
    out.success=true;const auto standard=font(s,"SaveDataDisplaySetToFont",{},false);
    if(setting(s,"SaveDataDisplaySetToGraphicEnabled",1)){
        const int w=setting(s,"SaveDataDisplaySetToGraphicW",80),h=setting(s,"SaveDataDisplaySetToGraphicH",60);
        if(w<=0||h<=0||w>16384||h>16384)throw std::runtime_error("invalid native save preview bitmap dimensions");
        // Native33330 failure leaves an empty DIB instance; it does not
        // suppress valid comment/date fields or turn success into failure.
        if(!m.thumbnail.empty())out.thumbnail=SavePreviewBitmap{m.thumbnail,setting(s,"SaveDataDisplaySetToGraphicX",5),setting(s,"SaveDataDisplaySetToGraphicY",5),w,h};
    }
    auto text=[&](SavePreviewText::Kind kind,std::string value,Font style,const std::string& key,int default_x,int default_y,std::optional<SavePreviewRect> rect={}){
        if(value.empty())return;out.text.push_back({kind,std::move(value),style.font,style.effect,style.color,setting(s,key+"X",default_x),setting(s,key+"Y",default_y),rect});
    };
    if(setting(s,"SaveDataDisplaySetToDateEnabled",1)&&m.date.valid)text(SavePreviewText::Date,date_format(string_setting(s,"SaveDataDisplaySetToDateFormat","%4.4d/%2.2d/%2.2d %2.2d:%2.2d"),date_fields(s,m.date)),standard,"SaveDataDisplaySetToDate",140,5);
    if(setting(s,"SaveDataDisplaySetToComment1Enabled",1)){
        std::string comment=m.comment1;
        if(options.comment1_skip_bytes){auto encoded=encode_original_text(comment,options.encoding.c_str());if(options.comment1_skip_bytes>encoded.size())throw std::runtime_error("native comment1 prefix exceeds metadata bytes");comment=decode(encoded.substr(options.comment1_skip_bytes),options.encoding.c_str());}
        text(SavePreviewText::Comment1,std::move(comment),font(s,"SaveDataDisplaySetToComment1Font",standard,true),"SaveDataDisplaySetToComment1",140,25);
    }
    if(setting(s,"SaveDataDisplaySetToComment2Enabled",1)){
        std::optional<SavePreviewRect> rect;auto p=s.records.find("SaveDataDisplaySetToComment2Rect");if(p!=s.records.end()){
            if(p->second.size()!=4)throw std::runtime_error("invalid native comment2 rectangle");SavePreviewRect r{integer(p->second[0]),integer(p->second[1]),integer(p->second[2]),integer(p->second[3])};if(!r.empty())rect=r;
        }
        text(SavePreviewText::Comment2,m.comment2,font(s,"SaveDataDisplaySetToComment2Font",standard,true),"SaveDataDisplaySetToComment2",140,45,rect);
    }
    return out;
}
void draw_save_preview(const SavePreviewLayout& layout,const SavePreviewDrawHooks& hooks){
    if(!layout.success)return;
    if(layout.thumbnail&&!hooks.bitmap)throw std::runtime_error("save preview requires bitmap draw hook");
    if(!layout.text.empty()&&!hooks.text)throw std::runtime_error("save preview requires native text draw hook");
    if(layout.thumbnail)hooks.bitmap(*layout.thumbnail,layout.parent_x,layout.parent_y);
    for(const auto& text:layout.text)hooks.text(text,layout.parent_x,layout.parent_y);
}
}

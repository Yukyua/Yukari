#include "yukari/engine_config.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>
namespace yukari {
int32_t engine_config_integer(const std::string& s){
 size_t at=0;while(at<s.size()&&(s[at]==' '||s[at]=='\t'||s[at]=='\v'||s[at]=='\f'||s[at]=='\r'||s[at]=='\n'))++at;
 bool negative=false;if(at<s.size()&&(s[at]=='+'||s[at]=='-'))negative=s[at++]=='-';
 auto digit=[](unsigned char c){if(c>='0'&&c<='9')return int(c-'0');if(c>='a'&&c<='f')return int(c-'a'+10);if(c>='A'&&c<='F')return int(c-'A'+10);return 99;};
 unsigned base=10;if(at<s.size()&&s[at]=='0'){base=8;if(at+2<s.size()&&(s[at+1]=='x'||s[at+1]=='X')&&digit(uint8_t(s[at+2]))<16){base=16;at+=2;}}
 const uint64_t limit=negative?2147483648ULL:2147483647ULL;uint64_t value=0;
 while(at<s.size()){auto n=digit(uint8_t(s[at]));if(unsigned(n)>=base)break;value=value>(limit-unsigned(n))/base?limit:value*base+unsigned(n);++at;}
 if(negative){if(value==2147483648ULL)return INT32_MIN;return -int32_t(value);}return int32_t(value);
}
namespace {
bool lead(uint8_t c,const std::string& encoding){if(encoding=="CP936"||encoding=="GBK")return c>=0x81&&c<=0xfe;return encoding=="CP932"||encoding=="SHIFT-JIS"||encoding=="SHIFT_JIS"||encoding=="SJIS"?(c>=0x81&&c<=0x9f)||(c>=0xe0&&c<=0xfc):false;}
void apply(EngineConfig& c,EngineConfigEntry& e){
 auto n=[&](size_t at=0){return engine_config_integer(e.values.at(at));};
 auto scalar=[&](std::optional<int32_t>& target){if(!e.values.empty())target=n();};
 auto boolean=[&](std::optional<bool>& target,bool empty_resets=false){if(!e.values.empty())target=n()!=0;else if(empty_resets)target=false;};
 auto point=[&](std::optional<EngineConfigPoint>& target){if(e.values.size()>=2)target=EngineConfigPoint{n(),n(1)};};
 auto list=[&](std::vector<int32_t>& target){for(const auto& token:e.values)target.push_back(engine_config_integer(token));};
 e.recognized=true;
 if(e.key=="TransitionWaitType")scalar(c.transition_wait_type);
 else if(e.key=="FontBorderThickSize")scalar(c.font_border_thick_size);
 else if(e.key=="FontShadowThickSize")scalar(c.font_shadow_thick_size);
 else if(e.key=="SkipScrollSpeed"){if(!e.values.empty()&&n()>0)c.skip_scroll_speed=n();}
 else if(e.key=="ModeExclusive")boolean(c.mode_exclusive);
 else if(e.key=="WindowSizeVariable")boolean(c.window_size_variable);
 else if(e.key=="SoundVolumeType")boolean(c.sound_volume_type,true);
 else if(e.key=="WindowSize")point(c.window_size);
 else if(e.key=="HistorySoundIconPositionSet")point(c.history_sound_icon_position);
 else if(e.key=="HistorySoundIconPositionType")scalar(c.history_sound_icon_position_type);
 else if(e.key=="HistoryLineMargin")scalar(c.history_line_margin);
 else if(e.key=="HistoryRowsMargin")scalar(c.history_rows_margin);
 else if(e.key=="NomalCursor"){if(!e.values.empty())c.normal_cursor=e.values[0];}
 else if(e.key=="SaveDataDirectory"){if(!e.values.empty()){auto path=e.values[0];if(path.empty()||path.back()!='\\')path+='\\';c.save_data_directory=std::move(path);}}
 else if(e.key=="SaveDataIntoKeyWaitIconNumber")boolean(c.save_data_into_key_wait_icon_number,true);
 else if(e.key=="SaveDataIntoSelectPointData"||e.key=="SelectPointSaveDataEnable")boolean(c.save_data_into_select_point_data,true);
 else if(e.key=="SaveDataIntoHistoryData")boolean(c.save_data_into_history_data,true);
 else if(e.key=="SaveDataWithoutScriptData")boolean(c.save_data_without_script_data,true);
 else if(e.key=="SpriteWithoutGraphicAttribute")list(c.sprite_without_graphic_attribute);
 else if(e.key=="SaveDataWithoutGraphicTransmittance")list(c.save_data_without_graphic_transmittance);
 else if(e.key=="SaveDataWithoutGraphicLayer")list(c.save_data_without_graphic_layer);
 else e.recognized=false;
}
}
EngineConfig parse_engine_config(const Bytes& bytes,const char* codepage){
 if(!codepage||!*codepage)throw std::runtime_error("default.ini requires an explicit encoding");
 if(bytes.size()>16*1024*1024)throw std::runtime_error("default.ini exceeds parser size limit");
 EngineConfig result;std::string encoding=codepage;uint32_t line_number=0;
 size_t source_end=0;while(source_end<bytes.size()&&bytes[source_end])++source_end;
 for(size_t at=0;at<source_end;){const auto begin=at;++line_number;while(at<source_end&&bytes[at]!='\r'&&bytes[at]!='\n')++at;const auto end=at;if(at<source_end&&bytes[at]=='\r')++at;if(at<source_end&&bytes[at]=='\n')++at;
  if(end-begin>1023)throw std::runtime_error("default.ini line exceeds native safe buffer at line "+std::to_string(line_number));
  std::string line(bytes.begin()+begin,bytes.begin()+end);
  for(size_t i=0;i<line.size();){uint8_t c=uint8_t(line[i]);if(c==' '||c=='\t'||c==','){line[i++]=0;continue;}if(c=='/'&&i+1<line.size()&&line[i+1]=='/'){std::fill(line.begin()+i,line.end(),0);break;}if(lead(c,encoding)){if(i+1>=line.size())throw std::runtime_error("truncated default.ini codepage character");i+=2;}else ++i;}
  auto equals=line.find('=');if(equals==std::string::npos)continue;
  size_t key_begin=0;while(key_begin<equals&&!line[key_begin])++key_begin;if(key_begin==equals)continue;
  size_t key_end=key_begin;while(key_end<equals&&line[key_end])++key_end;
  bool invalid_key=false;for(size_t i=key_end;i<equals;++i)if(line[i])invalid_key=true;if(invalid_key)continue;
  EngineConfigEntry entry;entry.key=decode(line.substr(key_begin,key_end-key_begin),codepage);entry.line=line_number;entry.byte_offset=begin;
  for(size_t i=equals+1;i<line.size();){while(i<line.size()&&!line[i])++i;if(i==line.size())break;auto start=i;while(i<line.size()&&line[i])++i;if(entry.values.size()>=16)throw std::runtime_error("default.ini line exceeds native safe token count");entry.values.push_back(decode(line.substr(start,i-start),codepage));}
  apply(result,entry);result.entries.push_back(std::move(entry));
 }
 return result;
}
}

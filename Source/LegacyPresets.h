#pragma once
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <cstdint>
#include "RecoveredFilters.h"
namespace camelphat {
struct LegacyPreset {std::string name;std::array<float,53> values{};bool oldBitCrush=true;std::string bank;};
inline bool decodeLegacy(const void* input,size_t size,std::vector<LegacyPreset>& presets){
 const auto* data=static_cast<const unsigned char*>(input);if(size<60||std::memcmp(data,"CcnK",4)||std::memcmp(data+16,"CaP3",4))return false;
 bool bank=!std::memcmp(data+8,"FBCh",4),single=!std::memcmp(data+8,"FPCh",4);if(!bank&&!single)return false;
 size_t position=bank?160:60;if(size<position)return false;unsigned count=bank?(unsigned(data[24])<<24)|(unsigned(data[25])<<16)|(unsigned(data[26])<<8)|unsigned(data[27]):1;if(count==0||count>1024)return false;
 auto u32=[&](uint32_t&v){if(position+4>size)return false;v=uint32_t(data[position])|(uint32_t(data[position+1])<<8)|(uint32_t(data[position+2])<<16)|(uint32_t(data[position+3])<<24);position+=4;return true;};
 auto string=[&](std::string&v){size_t start=position;while(position<size&&data[position]!=0&&position-start<4096)++position;if(position>=size||position-start>=4096)return false;v.assign(reinterpret_cast<const char*>(data+start),position-start);++position;return true;};
 std::vector<LegacyPreset> result;result.reserve(count);
 for(unsigned k=0;k<count;++k){uint32_t word=0;if(!u32(word))return false;float version;std::memcpy(&version,&word,4);if(!std::isfinite(version)||version<0||version>3.5f)return false;LegacyPreset p;if(!string(p.name)||!u32(word)||word>53)return false;unsigned parameters=word;
  for(unsigned i=0;i<parameters;++i){if(!u32(word))return false;std::memcpy(&p.values[i],&word,4);if(!std::isfinite(p.values[i])||p.values[i]<0||p.values[i]>1.001f)return false;}
  if(version<=camelphat_recovered::floatBits(0x3dcccccd))for(int id:{36,42}){int index=std::min(10,int((1.f-p.values[size_t(id)])*11.f));p.values[size_t(id)]=std::max(0.f,float(double(3*index+1)/-33.+1.)-camelphat_recovered::floatBits(0x38d1b717));}
  if(version<=camelphat_recovered::floatBits(0x3f051eb8)){p.values[46]=1;p.values[47]=1;}
  if(version>=3 && position+15<size && !std::memcmp(data+position,"<compatibility>",15)){std::string marker,mode;if(!string(marker)||!string(mode))return false;p.oldBitCrush=mode.find("useOldBitCrush 0")==std::string::npos;}
  result.push_back(std::move(p));
 }
 presets=std::move(result);return true;
}
inline std::vector<unsigned char> encodeLegacy(const std::vector<LegacyPreset>& presets,bool bank){
 if(presets.empty()||(!bank&&presets.size()!=1))return {};
 std::vector<unsigned char> payload;
 auto little=[&](uint32_t word){for(int i=0;i<4;++i)payload.push_back(static_cast<unsigned char>(word>>(i*8)));};
 auto string=[&](const std::string& value){payload.insert(payload.end(),value.begin(),value.end());payload.push_back(0);};
 for(auto& p:presets){little(0x40400000);string(p.name);little(53);for(float v:p.values){uint32_t bits;std::memcpy(&bits,&v,4);little(bits);}string("<compatibility>");string(p.oldBitCrush?"useOldBitCrush 1":"useOldBitCrush 0");}
 std::vector<unsigned char> file(bank?160:60,0);
 auto big=[&](size_t offset,uint32_t word){for(int i=0;i<4;++i)file[offset+size_t(i)]=static_cast<unsigned char>(word>>(24-i*8));};
 std::memcpy(file.data(),"CcnK",4);big(4,uint32_t(file.size()+payload.size()-8));std::memcpy(file.data()+8,bank?"FBCh":"FPCh",4);big(12,1);std::memcpy(file.data()+16,"CaP3",4);big(20,1);big(24,uint32_t(presets.size()));
 if(!bank)std::memcpy(file.data()+28,presets[0].name.data(),std::min(size_t(27),presets[0].name.size()));
 big(bank?156:56,uint32_t(payload.size()));file.insert(file.end(),payload.begin(),payload.end());return file;
}
}

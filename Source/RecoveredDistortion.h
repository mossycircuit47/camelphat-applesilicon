#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
namespace camelphat_recovered {
inline float bitsFloat(uint32_t bits) noexcept { float f; std::memcpy(&f,&bits,4); return f; }
// Recovered from original exported stereo processors at 0xeee0, 0xf030, 0xf270.
// Preserve single-precision operation order; compile with -ffp-contract=off.
struct Distortion {
 static float mechanical(float sample,float amount) noexcept {
  float drive=amount*10.f; drive=drive+1.f;
  float slope=.25f/(drive-.75f); float x=sample*drive;
  if(x>.75f) { x=x-.75f; x=x*slope; return x+.75f; }
  if(x<-.75f) { x=-.75f-x; x=x*slope; x=x+.75f; return -x; }
  return x;
 }
 static float tube(float sample,float amount) noexcept {
  float drive=4.f*amount; drive=drive+1.f;
  float threshold=bitsFloat(0x3f19999a), span=drive-threshold;
  float x=sample*drive;
  if(x>threshold || x< -threshold) {
   bool neg=x<0.f;
   float v=neg? -threshold-x:x-threshold;
   v=v/span; v=1.f-v; v=v*bitsFloat(0x3f3504f7);
   v=v*v; v=.5f-v; v=v*bitsFloat(0x3f4ccccc); v=v+threshold;
   return neg?-v:v;
  }
  return x;
 }
 static float bitReduction(float sample,float amount,bool truncateMode) noexcept {
  float v=1.f-amount; v=v*15.f; v=v+1.f;
  int bits=static_cast<int>(v); float steps=static_cast<float>(1u<<bits);
  float inverse=1.f/steps; float x=sample*steps; x=x+.5f;
  x=truncateMode?static_cast<float>(static_cast<int>(x)):std::floor(x);
  return x*inverse;
 }
 void processMech(float* l,float* r,int n,float amount) noexcept {for(int i=0;i<n;++i)l[i]=mechanical(l[i],amount);for(int i=0;i<n;++i)r[i]=mechanical(r[i],amount);}
 void processTube(float* l,float* r,int n,float amount) noexcept {for(int i=0;i<n;++i)l[i]=tube(l[i],amount);for(int i=0;i<n;++i)r[i]=tube(r[i],amount);}
 void processBitRed(float* l,float* r,int n,float amount,bool mode) noexcept {for(int i=0;i<n;++i)l[i]=bitReduction(l[i],amount,mode);for(int i=0;i<n;++i)r[i]=bitReduction(r[i],amount,mode);}
};
}

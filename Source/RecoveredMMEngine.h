#pragma once
#include "RecoveredTables.h"
#include "RecoveredMMFilter.h"
#include "RecoveredAnalog.h"
#include "RecoveredSVF.h"
#include <memory>
#include <type_traits>
namespace camelphat_recovered {
struct FilterBackend {
 virtual ~FilterBackend()=default;
 virtual void rate(float)=0;virtual void mode(int)=0;virtual void cutoff(float)=0;virtual void resonance(float)=0;virtual void drive(float)=0;virtual void clear()=0;virtual void process(float*,float*,int)=0;
};
template<class T,class=void>struct HasStereo3:std::false_type{};
template<class T>struct HasStereo3<T,std::void_t<decltype(std::declval<T&>().process2((float*)nullptr,(float*)nullptr,0))>>:std::true_type{};
template<class T>struct FilterPair:FilterBackend {
 T l,r;
 void rate(float v)override{l.setSampleRate(v);r.setSampleRate(v);}
 void mode(int v)override{l.setMode(v);r.setMode(v);}
 void cutoff(float v)override{l.setDestCutoff(v);r.setDestCutoff(v);}
 void resonance(float v)override{l.setDestResonance(v);r.setDestResonance(v);}
 void drive(float v)override{l.setDestDrive(v);r.setDestDrive(v);}
 void clear()override{l.clearHistory();r.clearHistory();}
 void process(float* a,float* b,int n)override{if constexpr(HasStereo3<T>::value)l.process2(a,b,n);else{l.process(a,n);r.process(b,n);}}
};
class MMEngine {
public:
 std::array<std::unique_ptr<FilterBackend>,16> backends;
 EnvelopeFollower envelope,limiter;
 float sampleRate=44100,cutoffValue=-1,resonanceValue=-1,envValue=-1,mixValue=-1,attackValue=-1,releaseValue=-1,maxCutoff=127,limiterGain=1,limiterRelease=0;
 int type=-1,factor=1,postMode=0;FilterBackend* active=nullptr;
 template<class T>void add(int i){backends[size_t(i)]=std::make_unique<FilterPair<T>>();}
 MMEngine(){
  add<CAFilterBiquad>(0);add<CAFilterBiquadBP>(1);add<CAFilterBiquadLPFat>(2);add<CAFilterBiquadBPFat>(3);add<CAFilterBiquadFat>(4);add<CAFilterComb>(5);add<CAFilterCombNeg>(6);add<CAFilterRingMod>(7);
  add<SVFAFilter>(8);add<MS20AFilter>(9);add<OTAAFilter>(10);add<MoogAFilter>(11);add<CAFilterTube>(12);add<CAFilterMech>(13);add<CAFilterBitRed>(14);add<CAFilterBiquadPeaking>(15);
  limiter.attack=.0075f;limiter.release=.05f;
 }
 static float hertz(float cutoff,float env,float envelopeValue){
  float value=env*envelopeValue;value=value*cutoff;float dry=(1.f-env)*cutoff;value=value+dry;
  float hz;
  if(value>=1)hz=16000.f;
  else if(value<=0)hz=80.f;
  else{float oneMinus=1.f-value;double log=std::log10(double(oneMinus))*doubleBits(0xbfd0000000000000ULL);
   if(log>1)hz=16000;else if(log<doubleBits(0x3e7ad7f2a0000000ULL))hz=80;else hz=float(log*doubleBits(0x40cf180000000000ULL)+80.);}
  return hz*floatBits(0x3b14f209);
 }
 static float midi(float hz){float x=std::log(hz);x=x*(12.f/std::log(2.f));x=x+69.f;return std::max(0.f,x);}
 void reset(float sr){sampleRate=sr;type=-1;cutoffValue=resonanceValue=envValue=mixValue=attackValue=releaseValue=-1;limiterGain=1;limiterRelease=0;
  limiter.setSampleRate(sr);envelope.setSampleRate(sr);limiter.clear();envelope.clear();maxCutoff=midi((sr*.5f)*floatBits(0x3b14f209))-floatBits(0x3c23d70a);
  // Reserve delay buffers outside the audio callback; later mode changes retain capacity.
  for(auto& b:backends)b->rate(sr*2.f);
 }
 void setType(int t,float cut,float res){type=t;const auto* def=camelphat::filterDefinitions.data()+size_t(t)*12;
  int family=def[0],mode=def[1];factor=def[2];postMode=def[3];active=backends[size_t(family==0&&mode==4?15:family)].get();
  envelope.clear();limiter.clear();envelope.setSampleRate(float(factor)*sampleRate);envelope.setSampleRate(float(factor)*sampleRate);limiterGain=1;limiterRelease=0;
  active->rate(float(factor)*sampleRate);active->mode(mode);active->cutoff(cut);active->resonance(res);active->drive(1);active->clear();
 }
 void process(int t,float cutoff,float resonance,float env,float mix,float attack,float release,float* left,float* right,int n){
#ifdef CAMELPHAT_MM_CAPTURE
  CAMELPHAT_MM_CAPTURE(t,cutoff,resonance,env,mix,attack,release,left,right,n);
#endif
  float mixStep=(mix-mixValue)/float(n);
  if(attack!=attackValue){attackValue=attack;envelope.attack=attack;envelope.attackStep=attack==0?0:1.f/(attack*envelope.sampleRate);}
  if(release!=releaseValue){releaseValue=release;envelope.release=release;envelope.releaseStep=release==0?0:1.f/(release*envelope.sampleRate);}
  float dryL[64],dryR[64],upL[128],upR[128];std::copy_n(left,n,dryL);std::copy_n(right,n,dryR);
  bool change=cutoffValue!=cutoff||type!=t;float cutoffStep=0;
  if(change){float startCut=std::min(maxCutoff,midi(hertz(cutoffValue,envValue,envelope.envelope)));if(type!=t)setType(t,startCut,resonanceValue);cutoffStep=(cutoff-cutoffValue)/float(n*factor);}
  if(envValue!=0 || resonanceValue!=resonance || envValue!=env){resonanceValue=resonance;envValue=env;float cut=midi(hertz(cutoffValue,envValue,envelope.envelope));active->resonance(resonanceValue);active->cutoff(cut);}
  float* l=left;float* r=right;int count=n;
  if(factor!=1){count=n*2;for(int i=0;i<n;++i){upL[2*i]=upL[2*i+1]=left[i];upR[2*i]=upR[2*i+1]=right[i];}l=upL;r=upR;}
  for(int offset=0;offset<count;offset+=16){int block=std::min(16,count-offset);for(int i=0;i<block;++i){float x=(l[offset+i]+r[offset+i])*.5f;x=x*.5f;envelope.process(x);}active->process(l+offset,r+offset,block);
   if(change){cutoffValue=cutoffValue+float(block)*cutoffStep;active->cutoff(midi(hertz(cutoffValue,envValue,envelope.envelope)));}}
  cutoffValue=cutoff;resonanceValue=resonance;
  if(factor!=1)for(int i=0;i<n;++i){left[i]=(upL[2*i]+upL[2*i+1])*.5f;right[i]=(upR[2*i]+upR[2*i+1])*.5f;}
  if(postMode==1){for(int i=0;i<n;++i){float v=limiter.process(std::max(std::fabs(left[i]),std::fabs(right[i])));if(v>1){float candidate=1.f/v;if(limiterGain>candidate){limiterGain=candidate;limiterRelease=(1.f-candidate)/(7.5f*sampleRate);}}if(limiterGain<1){left[i]*=limiterGain;right[i]*=limiterGain;limiterGain+=limiterRelease;}}}
  else if(postMode==2){for(int i=0;i<n;++i){left[i]=softPost(left[i],resonanceValue);right[i]=softPost(right[i],resonanceValue);}}
  for(int i=0;i<n;++i){left[i]=(1.f-mixValue)*dryL[i]+mixValue*left[i];right[i]=(1.f-mixValue)*dryR[i]+mixValue*right[i];mixValue+=mixStep;}mixValue=mix;
 }
 static float softPost(float x,float r){float drive=r*6.f+1.f;if(x>.7f||x<-.7f){bool neg=x<0;float v=neg?-x:x;v=(v-.7f)/(drive-.7f);v=(1.f-v)*floatBits(0x3f3504f7);v=.5f-v*v;v=v*floatBits(0x3f19999a);v=v+.7f;return neg?-v:v;}return x;}
};
}

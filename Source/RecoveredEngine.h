#pragma once
#include "RecoveredParameter.h"
#include "RecoveredDistortion.h"
#include "RecoveredModulation.h"
#include "RecoveredMMEngine.h"
#include "RecoveredLfoRouting.h"
#include <array>
namespace camelphat_recovered {
class Engine {
public:
 struct ControlEvent {int sample=0,target=-1;float value=0;};
 std::array<Parameter,53> parameters;std::array<float,53> normalized{};
 BPFilter band;Distortion distortion;recovered::Exciter exciter;recovered::MBass magic;recovered::Flanger flanger;CACompressor compressor;MMEngine filter;
 LfoRouting routing;float sampleRate=44100,bpm=120;double beats=0;
 float bandRejectMix=0,masterGain=0,masterMix=0,meters[2]={};bool oldBitCrush=false;
 static float checkedValue(int id,float value){switch(id){case 0:case 7:case 12:case 20:case 23:case 26:return value>.5f?1.f:0.f;default:return value;}}
 void prepare(float sr,const std::array<float,53>& values){sampleRate=sr;normalized=values;
  for(size_t i=0;i<53;++i){normalized[i]=checkedValue(int(i),values[i]);parameters[i].configure(camelphat::kParameterSpecs[i],sr);parameters[i].set(normalized[i]);parameters[i].reset();}
  band.setSampleRate(sr);exciter.prepare(sr);magic.frequency=60;magic.prepare(sr);flanger.prepare(sr);compressor.setSampleRate(sr);compressor.clear();filter.reset(sr);
  bandRejectMix=masterGain=masterMix=0;meters[0]=meters[1]=0;
  routing=LfoRouting{};routing.setSampleRate(sr);routing.configure(normalized);
 }
 void set(int id,float v){v=checkedValue(id,v);normalized[size_t(id)]=v;parameters[size_t(id)].set(v);if(id>=32&&id<=43){
  updateLfos();int lfo=id>=38?1:0;
  if(id==32||id==33||id==38||id==39)routing.updateMapping(lfo);
  if(id==36||id==37||id==42||id==43)routing.updateRate(lfo);
 }}
 void updateLfos(){routing.configure(normalized);}
 void time(float tempo,double position,bool playing=true,bool force=false){
  float hostBeat=std::max(0.0f,float(position));
  if(tempo==0.0f)return;
  const double tolerance=doubleBits(0x3f50624de0000000ULL);
  if(force||tempo!=bpm||(playing&&std::fabs(double(hostBeat)-beats)>tolerance)){
   bpm=tempo;beats=double(hostBeat);routing.setTimeInfo(tempo,beats);
  }
 }
 float modulated(int id,float raw)const{return routing.owner[size_t(id)]>=0?routing.modulated(id,raw):raw;}
 float advance(int id,int n){auto& p=parameters[size_t(id)];p.advance(n);return modulated(id,p.current);}
 void process(const float* inL,const float* inR,float* outL,float* outR,int n,const ControlEvent* events=nullptr,int eventCount=0){
  if(parameters[29].stored<.5f){std::copy_n(inL,n,outL);std::copy_n(inR,n,outR);meters[0]=meters[1]=0;return;}
  beats+=double(float(n)*bpm)/(double(sampleRate)*60.);
  routing.bpm=bpm;routing.beat=float(beats);
  for(int id:{14,32,33,34,35,36,37,38,39,40,41,42,43})parameters[size_t(id)].advance(n);
  bool distortionOn=advance(7,n)>.5f,bandOn=advance(0,n)>.5f;
  bool bitOn=false,mechOn=false,tubeOn=false,exciterOn=false;
  if(distortionOn){bitOn=advance(8,n)>0;mechOn=advance(9,n)>0;tubeOn=advance(10,n)>0;exciterOn=advance(11,n)>0;}
  bool filterOn=advance(12,n)>.5f,flangerOn=advance(20,n)>.5f,magicOn=advance(23,n)>.5f,compressorOn=advance(26,n)>.5f;
  magic.clip=parameters[47].stored>.5f;compressor.setEnvelopeMode(parameters[46].stored<.5f);
  int eventIndex=0;
  for(int offset=0;offset<n;offset+=64){int block=std::min(64,n-offset);
   if(eventIndex<eventCount&&events[eventIndex].sample<=offset+64){auto& event=events[eventIndex++];if(event.target>=0&&event.target<53){int target=event.target;if(target==44||target==45){int index=int(parameters[size_t(target)].stored);if(index>=0&&index<27)target=camelphat::morphTargets[size_t(index)];else target=-1;}if(target>=0)set(target,event.value);}}
   float l[64],r[64],rejectL[64],rejectR[64];std::copy_n(inL+offset,block,l);std::copy_n(inR+offset,block,r);
   if(bandOn){int low=std::max(0,std::min(284,int(advance(1,block)*284.f))),high=std::max(0,std::min(284,int(advance(2,block)*284.f)));float qlow=advance(3,block),qhigh=advance(4,block);band.process(l,r,rejectL,rejectR,block,camelphat::bandPassFrequencies[size_t(low)],camelphat::bandPassFrequencies[size_t(high)],qlow,qhigh);}
   if(exciterOn)exciter.process(l,r,block,advance(11,block));
   if(bitOn)distortion.processBitRed(l,r,block,advance(8,block),oldBitCrush);
   if(mechOn)distortion.processMech(l,r,block,advance(9,block));
   if(filterOn){int preset=std::min(33,int(parameters[14].stored));int type=camelphat::filterIndexMap[size_t(preset)];float mix=advance(13,block),attack=advance(15,block),release=advance(16,block),env=advance(17,block),cutoff=advance(18,block),resonance=advance(19,block);filter.process(type,cutoff,resonance,env,mix,attack,release,l,r,block);}
   if(flangerOn){float amount=advance(21,block),rate=advance(22,block);flanger.process(l,r,block,amount,rate,0);}
   if(tubeOn)distortion.processTube(l,r,block,advance(10,block));
   if(magicOn){float amount=advance(24,block),hz=advance(25,block);magic.process(l,r,block,amount,hz);}
   if(compressorOn){float amount=std::min(floatBits(0x3f7d70a4),advance(27,block)),release=advance(28,block)*floatBits(0x3a83126f);compressor.process(l,r,block,amount,release);}
   if(bandOn){float step=(advance(5,block)-bandRejectMix)/float(block);for(int i=0;i<block;++i){l[i]=l[i]+bandRejectMix*rejectL[i];r[i]=r[i]+bandRejectMix*rejectR[i];bandRejectMix+=step;}}
   float mixStep=(advance(30,block)-masterMix)/float(block);float targetGain=float(std::pow(double(advance(31,block)),2.736999988555908));float gainStep=(targetGain-masterGain)/float(block);
   for(int i=0;i<block;++i){float wet=masterMix*l[i],dry=(1.f-masterMix)*inL[offset+i];outL[offset+i]=(wet+dry)*masterGain;wet=masterMix*r[i];dry=(1.f-masterMix)*inR[offset+i];outR[offset+i]=(wet+dry)*masterGain;masterGain+=gainStep;masterMix+=mixStep;}
   float clamp=floatBits(0x3f7ff972);for(int i=0;i<block;++i){outL[offset+i]=std::max(-clamp,std::min(clamp,outL[offset+i]));outR[offset+i]=std::max(-clamp,std::min(clamp,outR[offset+i]));}
   meters[0]=std::max(0.f,meters[0]-(2.f/sampleRate)*float(block));meters[1]=std::max(0.f,meters[1]-(2.f/sampleRate)*float(block));for(int i=0;i<block;i+=8){meters[0]=std::max(meters[0],std::fabs(outL[offset+i]));meters[1]=std::max(meters[1],std::fabs(outR[offset+i]));}
   routing.advance(block);
  }
 }
};
}

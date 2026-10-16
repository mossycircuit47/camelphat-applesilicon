#pragma once
#include "RecoveredModulation.h"
#include "RecoveredTables.h"
#include <array>
namespace camelphat_recovered {
inline int lfoRateIndex(float normalized){return std::min(32,int((1.0f-normalized)*33.0f));}
inline float lfoRateHz(float normalized,bool sync,float bpm,bool logarithmicFree=false){
    if(sync){float beats=4.0f*camelphat::rateBars[size_t(lfoRateIndex(normalized))];return float(1.0/(double(beats)*(60.0/double(bpm))));}
    if(!logarithmicFree){float low=floatBits(0x3CA3D70A);float span=8.0f-low;return span*normalized+low;}
    float position=(1.0f-normalized)*33.0f;int i=std::min(32,int(position)),j=std::min(32,i+1);float fraction=position-float(i);
    float beats=((1.0f-fraction)*camelphat::rateBars[size_t(i)])*4.0f;beats+=(fraction*camelphat::rateBars[size_t(j)])*4.0f;
    return float(1.0/(double(beats)*0.5));
}
inline float lfoPhaseAt(float normalized,bool sync,float beat){
    if(!sync)return -1.0f;
    float beats=4.0f*camelphat::rateBars[size_t(lfoRateIndex(normalized))];float position=beat/beats;
    float phase=position-float(int(position));if(phase>=1.0f)phase-=1.0f;return phase;
}
inline int lfoTarget(float normalized){int index=int(normalized*100.0f+0.5f);return index>=0&&index<23?int(camelphat::modulationTargets[size_t(index)]):-1;}
inline int lfoShape(float normalized){return std::min(9,int(normalized*100.0f+0.5f));}
inline int lfoOwnerForTarget(const std::array<float,53>&p,int parameter){
    for(int l=0;l<2;++l){int offset=l*6;if(int(p[size_t(32+offset)]+0.5f)>0&&lfoTarget(p[size_t(33+offset)])==parameter)return l;}return -1;
}
// Stateful mapping follows updateLfoMapping: turning LFO1 off does not automatically remap LFO2.
// Configure receives normalized VST3 parameters; modulation values are added after main parameter smoothing.
struct LfoRouting {
    recovered::Lfo lfo[2];std::array<float,53> parameters{};std::array<int,53> owner{};
    float sampleRate=44100,bpm=120,beat=0;bool initialized=false,logarithmicFree=false;
    LfoRouting(){owner.fill(-1);for(auto&l:lfo){l.sampleRate=sampleRate;l.computePhaseIncrement();}}
    void setSampleRate(float sr){sampleRate=sr;for(auto&l:lfo){l.sampleRate=sr;l.computePhaseIncrement();}}
    bool enabled(int l)const{return int(parameters[size_t(32+l*6)]+0.5f)>0;}
    bool synced(int l)const{return int(parameters[size_t(37+l*6)]+0.5f)>0;}
    void resetSyncedPhases(){for(int l=0;l<2;++l)if(synced(l)){float phase=lfoPhaseAt(parameters[size_t(36+l*6)],true,beat);if(phase<0||phase>=1)phase=0;lfo[l].phase=double(phase);}}
    void updateRate(int l){lfo[l].rate=lfoRateHz(parameters[size_t(36+l*6)],synced(l),bpm,logarithmicFree);lfo[l].computePhaseIncrement();resetSyncedPhases();}
    void updateMapping(int l){for(auto&o:owner)if(o==l)o=-1;int target=lfoTarget(parameters[size_t(33+l*6)]);if(target>=0&&enabled(l)&&(owner[size_t(target)]<0||l==0))owner[size_t(target)]=l;resetSyncedPhases();}
    void configure(const std::array<float,53>&normalized){auto previous=parameters;parameters=normalized;
        for(int l=0;l<2;++l){int offset=l*6;lfo[l].waveform=uint8_t(lfoShape(parameters[size_t(34+offset)]));lfo[l].depth=parameters[size_t(35+offset)];
            if(!initialized||parameters[size_t(36+offset)]!=previous[size_t(36+offset)]||parameters[size_t(37+offset)]!=previous[size_t(37+offset)])updateRate(l);
            if(!initialized||parameters[size_t(32+offset)]!=previous[size_t(32+offset)]||parameters[size_t(33+offset)]!=previous[size_t(33+offset)])updateMapping(l);
        }initialized=true;
    }
    void setTimeInfo(float newBpm,double newBeat){float roundedBeat=float(newBeat);if(newBpm!=bpm||roundedBeat!=beat){bpm=newBpm;beat=roundedBeat;updateRate(0);updateRate(1);}}
    float valueForTarget(int target)const{int index=owner[size_t(target)];return index>=0?lfo[index].getValue():0.0f;}
    float modulated(int target,float raw)const{return std::max(0.0f,std::min(1.0f,raw+valueForTarget(target)));}
    // Original process advances both LFOs after each chunk, including disabled/unmapped LFOs.
    void advance(int frames){lfo[0].advance(frames);lfo[1].advance(frames);}
};
}

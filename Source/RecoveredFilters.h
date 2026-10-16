#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

// Transcribed from CamelPhat 3.50 Intel: CC10-D832, C590-CBFF,
// and 25640-2576F in analysis/CamelPhat.asm. Compile with FP contraction off.
namespace camelphat_recovered {
inline float floatBits(std::uint32_t bits) { float v; std::memcpy(&v,&bits,4); return v; }
inline double doubleBits(std::uint64_t bits) { double v; std::memcpy(&v,&bits,8); return v; }
inline float removeTiny(float v) {
    const double d=v;
    return d>doubleBits(0xbcd203af9ee75616ULL) && d<doubleBits(0x3cd203af9ee75616ULL) ? 0.0f : v;
}

struct Biquad {
    float b0=0,b1=0,b2=0,a1=0,a2=0,y1=0,y2=0,x1=0,x2=0;
    void set(float sampleRate,float frequency,float resonance,bool highPass,bool qMode) {
        const double tau=doubleBits(0x401921fb54442d18ULL);
        const double omega=highPass ? (double(frequency)*tau)/double(sampleRate)
                                    : (double(frequency)/double(sampleRate))*tau;
        const double s=std::sin(omega), c=std::cos(omega);
        const double alpha=qMode ? s/(double(resonance)+double(resonance))
            : s*std::sinh(((double(resonance)*doubleBits(0x3fd62e42fefa39efULL))*omega)/s);
        const double den=alpha+1.0, numerator=highPass ? c+1.0 : 1.0-c;
        b0=removeTiny(float((numerator*0.5)/den));
        b1=removeTiny(float((highPass ? -numerator : numerator)/den));
        b2=b0;
        a1=removeTiny(float((c*-2.0)/den)); a2=removeTiny(float((1.0-alpha)/den));
    }
    void copyCoefficients(const Biquad& b) { b0=b.b0;b1=b.b1;b2=b.b2;a1=b.a1;a2=b.a2; }
    void blockStart() { const float t=removeTiny(y1); if(t==0.0f) y1=y2=0.0f; }
    float process(float x) {
        float y=x*b0; y=y+b1*x1; y=y+b2*x2; y=y-y1*a1; y=y-a2*y2;
        y2=y1;y1=y;x2=x1;x1=x;return y;
    }
};

struct BPFilter {
    float highFrequency=0,lowFrequency=0,highResonance=0,lowResonance=0,sampleRate=44100;
    Biquad filters[8]; // HP L/R, HP L/R, LP L/R, LP L/R
    void setupFilters() {
        float hq=1.0f-highResonance; hq=hq+floatBits(0x3f19999a);
        filters[0].set(sampleRate,highFrequency,hq,true,false);
        for(int i=1;i<4;++i) filters[i].copyCoefficients(filters[0]);
        float lq=1.5f*lowResonance; lq=lq+0.25f;
        filters[4].set(sampleRate,lowFrequency,lq,false,true);
        for(int i=5;i<8;++i) filters[i].copyCoefficients(filters[4]);
    }
    void setSampleRate(float rate) {sampleRate=rate;setupFilters();}
    // The original clear regenerates coefficients, and deliberately retains delay state.
    void clear() {setupFilters();}
    void clear(float h,float l,float hr,float lr) {highFrequency=h;lowFrequency=l;highResonance=hr;lowResonance=lr;setupFilters();}
    void process(float* left,float* right,float* rejectedLeft,float* rejectedRight,int count,float h,float l,float hr,float lr) {
        if(h!=highFrequency||l!=lowFrequency||hr!=highResonance||lr!=lowResonance) clear(h,l,hr,lr);
        for(auto& f:filters) f.blockStart();
        for(int channel=0;channel<2;++channel) {
            float* buffer=channel ? right:left;float* rejected=channel ? rejectedRight:rejectedLeft;
            for(int i=0;i<count;++i) {const float input=buffer[i];float y=input;
                for(int stage=0;stage<4;++stage)y=filters[2*stage+channel].process(y);
                buffer[i]=y;rejected[i]=input-y;
            }
        }
    }
};

struct EnvelopeFollower {
    float sampleRate=44100,attack=floatBits(0x3cf5c28f),release=floatBits(0x3da3d70a),envelope=0,attackStep=0,releaseStep=0;
    EnvelopeFollower() {setSampleRate(sampleRate);}
    void setSampleRate(float rate) {sampleRate=rate;attackStep=attack==0?0:1.0f/(attack*rate);releaseStep=release==0?0:1.0f/(release*rate);envelope=0;}
    void clear(){envelope=0;}
    float process(float input) {
        const float decayed=envelope-releaseStep;
        float delta=0;
        if(input>decayed)delta=input-decayed;
        else if(-decayed>input)delta=-input-decayed;
        envelope=std::max(0.0f,std::min(attackStep,delta)+decayed);return envelope;
    }
};

struct CACompressor {
    float amount=0,envelopeMode=0,release=0,sampleRate=44100,gain=1,gainRelease=0;
    EnvelopeFollower follower;
    void setSampleRate(float rate){sampleRate=rate;follower.setSampleRate(rate);}
    void clear(){gain=1;gainRelease=0;follower.clear();}
    void clear(float a,float r){amount=a;release=r;clear();}
    void setEnvelopeMode(bool mode){envelopeMode=mode?1.0f:0.0f;}
    void process(float* left,float* right,int count,float targetAmount,float targetRelease) {
        const float multiplier=1.0f/(1.0f-amount);
        const float releaseSamples=release*sampleRate;
        const float amountStep=(targetAmount-amount)/float(count);
        release=targetRelease;
        const float threshold=floatBits(0x3e9eb852),kneeRange=multiplier-threshold;
        for(int i=0;i<count;++i){
            const float peak=std::max(std::fabs(left[i]),std::fabs(right[i]));
            const float envelope=envelopeMode==0 ? std::fabs(peak) : follower.process(peak);
            float l=left[i]*multiplier,r=right[i]*multiplier;
            const float driven=envelope*multiplier;
            if(driven>threshold){
                const float normal=(driven-threshold)/kneeRange;
                float v=(1.0f-normal)*floatBits(0x3f3504f7);v=v*v;
                v=0.5f-v;v=v*floatBits(0x3fb0a3d7);v=v+threshold;
                const float candidate=v/driven;
                if(gain>candidate){gain=candidate;gainRelease=(1.0f-candidate)/releaseSamples;}
            }
            if(gain<1.0f){l=l*gain;r=r*gain;gain=gain+gainRelease;}
            left[i]=l;right[i]=r;amount=amount+amountStep;
        }
        amount=targetAmount;
    }
};
}

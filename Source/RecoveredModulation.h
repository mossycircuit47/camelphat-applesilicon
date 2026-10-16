#pragma once
#include "RecoveredLookupTables.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace recovered {
inline float floatBits(uint32_t x) { float f; std::memcpy(&f,&x,4); return f; }
inline double doubleBits(uint64_t x) { double d; std::memcpy(&d,&x,8); return d; }
// Translation of Chorus::processFlange (0xE070), Chorus::process (0xDD00).
// Compile with floating-point contraction disabled to preserve SSE scalar ordering.
class Flanger {
public:
    float baseDelay = floatBits(0x3A83126F), depth = floatBits(0x3B03126F);
    float stereoOffset = 0.0f;
    void prepare(float sr) { sampleRate=sr; ring.assign(std::max(1,int(35.0f*sr/1000.0f+0.5f)),0.0f); clear(); }
    void clear() { std::fill(ring.begin(),ring.end(),0.0f); phase=0; length=std::max(1,int((baseDelay+depth)*sampleRate+0.5f)); write=0; read=int(depth*sampleRate+0.5f); }
    void process(float* left,float* right,int n,float amount,float rate,float feedback) {
        if(sampleRate==0 || ring.empty()) return;
        const float increment=(rate*200.0f)/sampleRate, wet=amount*0.5f, dry=1.0f-wet;
        for(int i=0;i<n;++i) {
            phase+=increment; if(phase>=200.0f) phase-=200.0f;
            const float a=tap(phase);
            float rp=stereoOffset*50.0f+phase; if(rp>=200.0f)rp-=200.0f;
            const float b=tap(rp), l=left[i],r=right[i];
            float value=(l+r)*0.5f+((a+b)*feedback)*0.5f;
            constexpr double tiny=1.0e-15;
            if(double(value)>-tiny && double(value)<tiny) value=0.0f;
            ring[size_t(write)]=value;
            left[i]=l*dry+a*wet; right[i]=r*dry+b*wet;
            if(++write>=length)write=0; if(++read>=length)read=0;
        }
    }
private:
    float sampleRate=0,phase=0; int length=1,write=0,read=0; std::vector<float> ring;
    static const std::array<float,201>& sine() { return camelphat_recovered::originalSineLut; }
    float tap(float p) const {
        int k=int(p); float f=p-float(k); const auto& s=sine();
        float d=((s[size_t(k+1)]-s[size_t(k)])*f+s[size_t(k)])*0.5f;
        d+=0.5f; d*=depth; d*=sampleRate;
        int di=int(d); f=d-float(di); int a=length+read-di; while(a>=length)a-=length;
        int b=a+1; while(b>=length)b-=length;
        return f*ring[size_t(a)]+(1.0f-f)*ring[size_t(b)];
    }
};

// CALfo phase is double; lookup, depth, target and slew are float.
class Lfo {
public:
    uint8_t waveform=0; float rate=1,depth=0,sampleRate=1;
    double phase=0,phaseIncrement=0; float value=0,target=0,slew=0,previousRandom=0,nextRandom=0;
    void computePhaseIncrement(){phaseIncrement=double(rate)/double(sampleRate);slew=1.0f/(floatBits(0x3C23D70A)*sampleRate);}
    float getValue() const {return value;}
    void advance(int frames) {
        double old=phase; phase+=double(frames)*phaseIncrement; if(phase>=1.0)phase-=1.0;
        bool halfBoundary=(old<0.5 && phase>=0.5)||old>phase;
        if(waveform==5) {if(halfBoundary)target=randomValue();}
        else if(waveform==6) {
            if(halfBoundary){previousRandom=nextRandom;nextRandom=randomValue();}
            float t=float(phase)*2.0f; if(t>=1.0f)t-=1.0f;
            target=(1.0f-t)*previousRandom+t*nextRandom;
        } else {
            const auto& a=table(); float x=float(phase)*100.0f; int k=int(x);float f=x-float(k);
            const size_t index=size_t(waveform)*101+size_t(k);
            target=((1.0f-f)*a[index]+f*a[index+1]-0.5f)*depth;
        }
        float step=float(frames)*slew;
        if(target>value){value+=step;if(value>target)value=target;}else value=std::max(target,value-step);
    }
private:
    float randomValue() const {return (float(std::rand())*floatBits(0x30000000)-0.5f)*depth;}
    static const std::array<float,1010>& table() {return camelphat_recovered::originalLfoLut;}
};
}

#include "RecoveredFilters.h"
namespace recovered {
using MagicBiquad=camelphat_recovered::Biquad;
using MagicFollower=camelphat_recovered::EnvelopeFollower;
inline void peakFilter(MagicBiquad& b,float sr,float hz) {
    const double omega=(double(hz)*doubleBits(0x401921fb54442d18ULL))/double(sr);
    const double s=std::sin(omega);
    const double alpha=std::sinh((doubleBits(0x3fd62e42fefa39efULL)*omega)/s)*s;
    const double A=std::sqrt(std::pow(10.0,double(10.0f/20.0f)));
    const double p=alpha*A,q=alpha/A,den=q+1.0;
    b.b0=camelphat_recovered::removeTiny(float((p+1.0)/den));
    b.b1=camelphat_recovered::removeTiny(float((std::cos(omega)*-2.0)/den));
    b.b2=camelphat_recovered::removeTiny(float((1.0-p)/den));b.a1=b.b1;
    b.a2=camelphat_recovered::removeTiny(float((1.0-q)/den));
}
struct Exciter {
    MagicBiquad filter[2];MagicFollower follower;float sampleRate=44100;
    Exciter(){follower.attack=floatBits(0x3C23D70A);follower.release=floatBits(0x3DCCCCCD);}
    void prepare(float sr){sampleRate=sr;follower.setSampleRate(sr);clear();}
    void clear(){follower.clear();for(auto& f:filter){f=MagicBiquad{};f.set(sampleRate,7000,1,true,false);}}
    void process(float*l,float*r,int n,float amount){
        for(auto&f:filter)f.blockStart();
        for(int i=0;i<n;++i){float a=l[i],b=r[i],h=filter[0].process(a),j=filter[1].process(b);
            float e=(h+j)*300.0f;e*=0.5f;
            float gain=10.0f*follower.process(e);gain=std::min(gain,3.0f);
            float x=gain*h+a,y=gain*j+b;
            x=std::max(floatBits(0xBF7FBE77),std::min(floatBits(0x3F7FBE77),x));
            y=std::max(floatBits(0xBF7FBE77),std::min(floatBits(0x3F7FBE77),y));
            l[i]=x*amount+(1.0f-amount)*a;r[i]=y*amount+(1.0f-amount)*b;
        }
    }
};
struct MBass {
    MagicBiquad high[2],peak[2];MagicFollower follower;float sampleRate=44100,frequency=0,gain=1,gainRelease=0;bool clip=false;
    MBass(){follower.attack=floatBits(0x3BF5C28F);follower.release=floatBits(0x3D4CCCCD);follower.setSampleRate(44100);}
    // Original setSampleRate does not update this follower's rate; it stays at its constructor's 44100 Hz.
    void prepare(float sr){sampleRate=sr;clear();}
    void clear(){for(auto&f:high){f=MagicBiquad{};f.set(sampleRate,40,.5f,true,false);}for(auto&f:peak){f=MagicBiquad{};peakFilter(f,sampleRate,frequency);}gain=1;gainRelease=0;}
    void setFrequency(float hz){frequency=hz;for(auto&f:peak)peakFilter(f,sampleRate,hz);}
    void process(float*l,float*r,int n,float amount,float hz){
        if(hz!=frequency)for(auto&f:peak)peakFilter(f,sampleRate,hz); // original intentionally does not store requested frequency here
        for(auto&f:high)f.blockStart();for(auto&f:peak)f.blockStart();
        for(int i=0;i<n;++i){float a=l[i],b=r[i],x=high[0].process(peak[0].process(a)),y=high[1].process(peak[1].process(b));
            if(clip){x=std::max(-1.0f,std::min(1.0f,x));y=std::max(-1.0f,std::min(1.0f,y));}
            else {float envelope=follower.process(std::max(std::fabs(y),std::fabs(x)));
                if(envelope>1){float target=1.0f/envelope;if(gain>target){gain=target;gainRelease=(1.0f-target)/(7.5f*sampleRate);}}
                if(gain<1){x*=gain;y*=gain;gain+=gainRelease;}
            }
            l[i]=x*amount+a*(1.0f-amount);r[i]=y*amount+b*(1.0f-amount);
        }
    }
};
}

#pragma once
#include "RecoveredFilters.h"
#include <array>
#include <vector>

namespace camelphat_recovered {
// 51 dispatch entries, 48-byte original records at unk_616C0. Labels precede
// each record in the binary. Fourth column is MMFilter's internal flag.
struct MMFilterDescriptor {int family,mode,oversample,flag;};
inline constexpr MMFilterDescriptor mmFilterDescriptors[51]={
    {0,0,1,1},{2,0,1,2},{1,0,1,1},{3,0,1,2},{0,2,1,1},{4,2,1,2},
    {0,6,1,0},{0,4,1,1},{0,3,1,1},{5,0,1,1},{6,0,1,1},{7,0,2,1},
    {12,0,1,0},{13,0,1,0},{14,0,1,0},
    {8,0,2,0},{9,0,1,0},{10,3,2,0},{11,3,2,0},{10,0,2,0},{11,0,2,0},
    {8,1,2,0},{9,1,1,0},{10,4,2,0},{11,4,2,0},{10,1,2,0},{11,1,2,0},
    {8,2,2,0},{9,2,1,0},{10,5,2,0},{11,5,2,0},{10,2,2,0},{11,2,2,0},
    {8,0,1,0},{9,0,1,0},{10,3,1,0},{11,3,1,0},{10,0,1,0},{11,0,1,0},
    {8,1,1,0},{9,1,1,0},{10,4,1,0},{11,4,1,0},{10,1,1,0},{11,1,1,0},
    {8,2,1,0},{9,2,1,0},{10,5,1,0},{11,5,1,0},{10,2,1,0},{11,2,1,0}
};
inline float removeSubnormal(float v){std::uint32_t bits;std::memcpy(&bits,&v,4);return (bits&0x7f800000)==0 && (bits&0x7fffffff)!=0 ? 0.0f:v;}

// Exact CAFilterBiquad family: 4BB90-4CDF5. This family is selected by
// MMFilter table model 0 (all modes except mode 4, which selects LPFat).
struct CAFilterBiquad {
    float sampleRate=44100,cutoff=17000,resonance=0,targetCutoff=17000,targetResonance=0;
    int mode=0;unsigned phatMode=0;bool dirty=true;
    Biquad filter;
    void clearHistory(){cutoff=targetCutoff;resonance=targetResonance;dirty=true;filter.x1=filter.x2=filter.y1=filter.y2=0;}
    void setSampleRate(float rate){sampleRate=rate;clearHistory();}
    void setDestCutoff(float midi){float exponent=midi-69.f;exponent=exponent*floatBits(0x3daaaaab);targetCutoff=std::pow(2.0f,exponent)*440.f;dirty=true;}
    void setDestResonance(float r){targetResonance=r;dirty=true;}
    void setDestDrive(float){} // Original exported implementation is empty.
    void setMode(int m){mode=m;dirty=true;}
    void setPhatMode(unsigned m){phatMode=m;}
    void updateFilter(float frequency,float r){
        float q=r*6.0f;q=q+1.0f;
        float bandwidth=q;
        if(mode==0 || mode==2){
            if(mode==2 && phatMode==3)q=q*3.0f;
            // HP uses multiply before divide; LP uses divide before multiply.
            const double tau=doubleBits(0x401921fb54442d18ULL);
            const double omega=mode==2 ? (double(frequency)*tau)/double(sampleRate) : (double(frequency)/double(sampleRate))*tau;
            const double c=std::cos(omega),s=std::sin(omega),alpha=s/(double(q)+double(q)),den=alpha+1.;
            const double n=mode==2 ? c+1. : 1.-c;
            filter.b0=float((n*.5)/den);filter.b1=float((mode==2 ? -n:n)/den);filter.b2=filter.b0;
            filter.a1=float((c*-2.)/den);filter.a2=float((1.-alpha)/den);
        }else if(mode>=1&&mode<=6){
            if(mode==1 && phatMode==4)bandwidth=bandwidth*3.f;
            if(mode==3)bandwidth=bandwidth-floatBits(0x3f7d70a4);
            float skirtGain=1;
            if(mode==6){float bw=r*200.f;bw=std::max(2.f,bw);bandwidth=bw/frequency;skirtGain=r+r;}
            const double tau=doubleBits(0x401921fb54442d18ULL);
            const double omega=(mode==3||mode==5) ? (double(frequency)/double(sampleRate))*tau
                : (double(frequency)*tau)/double(sampleRate);
            const double s=std::sin(omega),c=std::cos(omega);
            const double alpha=s*std::sinh(((double(bandwidth)*doubleBits(0x3fd62e42fefa39efULL))*omega)/s);
            const double den=alpha+1.;
            if(mode==1||mode==6){filter.b0=float((double(skirtGain)*alpha)/den);filter.b1=float(0./den);filter.b2=float((double(-skirtGain)*alpha)/den);filter.a1=float((c*-2.)/den);filter.a2=float((1.-alpha)/den);}
            if(mode==3){filter.b0=float(1./den);filter.b1=float((c*-2.)/den);filter.b2=filter.b0;filter.a1=filter.b1;filter.a2=float((1.-alpha)/den);}
            if(mode==5){filter.b0=float((1.-alpha)/den);filter.b1=float((c*-2.)/den);filter.b2=float(den/den);filter.a1=filter.b1;filter.a2=filter.b0;}
            if(mode==4){
                float gain=q;if(phatMode==1||phatMode==2)gain=gain*3.f;else if(phatMode==4)gain=gain*4.f;
                const double a=std::sqrt(double(std::pow(10.f,gain/20.f)));
                const double eqAlpha=s*std::sinh(((1.*doubleBits(0x3fd62e42fefa39efULL))*omega)/s);
                const double boosted=eqAlpha*a,damped=eqAlpha/a,eqDen=damped+1.;
                filter.b0=float((boosted+1.)/eqDen);filter.b1=float((c*-2.)/eqDen);filter.b2=float((1.-boosted)/eqDen);filter.a1=filter.b1;filter.a2=float((1.-damped)/eqDen);dirty=false;
            }
        }
        filter.b0=removeSubnormal(filter.b0);filter.b1=removeSubnormal(filter.b1);filter.b2=removeSubnormal(filter.b2);filter.a1=removeSubnormal(filter.a1);filter.a2=removeSubnormal(filter.a2);
    }
    void process(float* buffer,int count){
        if(dirty){dirty=false;updateFilter(targetCutoff,targetResonance);cutoff=targetCutoff;resonance=targetResonance;}
        for(int i=0;i<count;++i)buffer[i]=filter.process(buffer[i]);
        std::uint32_t bits;std::memcpy(&bits,&filter.y1,4);
        if(((bits&0x7f800000)==0 && (bits&0x7fffffff)!=0)||filter.y1>1000.f||filter.y1< -1000.f||filter.y2>1000.f||filter.y2< -1000.f)filter.y1=filter.y2=0;
    }
};

// The four wrapper families use the same coefficient generators and recurrence;
// their fixed phat mode and serial second section are set in MMFilter's ctor.
struct CAFilterBiquadBP {
    CAFilterBiquad first,second;
    explicit CAFilterBiquadBP(bool fat=false){first.setMode(1);second.setMode(4);first.setPhatMode(fat?4:1);second.setPhatMode(fat?4:1);first.clearHistory();second.clearHistory();}
    void clearHistory(){first.clearHistory();second.clearHistory();}
    void setSampleRate(float r){first.setSampleRate(r);second.setSampleRate(r);}
    void setDestCutoff(float c){first.setDestCutoff(c);second.setDestCutoff(c);}
    void setDestResonance(float r){first.setDestResonance(r);second.setDestResonance(r);}
    void setDestDrive(float){}
    void setMode(int){}
    void setPhatMode(unsigned){}
    void process(float* b,int n){first.process(b,n);second.process(b,n);}
};
struct CAFilterBiquadBPFat:CAFilterBiquadBP { CAFilterBiquadBPFat():CAFilterBiquadBP(true){} };
struct CAFilterBiquadFat {
    CAFilterBiquad section;
    CAFilterBiquadFat(){section.setMode(1);section.setPhatMode(3);section.clearHistory();}
    void clearHistory(){section.clearHistory();}
    void setSampleRate(float r){section.setSampleRate(r);}
    void setDestCutoff(float c){section.setDestCutoff(c);}
    void setDestResonance(float r){section.setDestResonance(r);}
    void setDestDrive(float){}
    void setMode(int m){section.setMode(m);}
    void setPhatMode(unsigned){}
    void process(float* b,int n){section.process(b,n);}
};
struct CAFilterBiquadPeaking:CAFilterBiquadFat {
    CAFilterBiquadPeaking(){section.setMode(4);section.setPhatMode(2);section.clearHistory();}
    void setMode(int){}
};
struct CAFilterTube {
    float cutoff=0,mix=0,drive=0,sampleRate=44100;
    void clearHistory(){}
    void setSampleRate(float rate){sampleRate=rate;clearHistory();}
    void setDestCutoff(float midi){cutoff=(midi-floatBits(0x421df281))/floatBits(0x42b773da);}
    void setDestResonance(float){mix=1;}
    void setDestDrive(float value){drive=value+.5f;}
    void setMode(int){} void setPhatMode(unsigned){}
    void process(float* b,int n){
        float boost=cutoff*4.f;boost=boost+1.f;const float trim=drive/(cutoff+1.f);
        const float knee=floatBits(0x3f19999a),range=boost-knee;
        for(int i=0;i<n;++i){const float input=b[i];float x=input*boost;
            if(x>knee){float normal=(x-knee)/range;float v=(1.f-normal)*floatBits(0x3f3504f7);v=v*v;x=.5f-v;x=x*floatBits(0x3f4ccccc);x=x+knee;}
            else if(x< -knee){float normal=(-knee-x)/range;float v=(1.f-normal)*floatBits(0x3f3504f7);v=v*v;x=.5f-v;x=x*floatBits(0x3f4ccccc);x=x+knee;x=-x;}
            float dry=input*(1.f-mix);float wet=mix*trim;wet=wet*x;b[i]=dry+wet;
        }
    }
};
struct CAFilterMech:CAFilterTube {
    void process(float* b,int n){
        float boost=cutoff*10.f;boost=boost+1.f;const float slope=.25f/(boost-.75f),trim=drive/(cutoff+1.f);
        for(int i=0;i<n;++i){const float input=b[i];float x=input*boost;
            if(x>.75f){x=x-.75f;x=x*slope;x=x+.75f;}
            else if(x< -.75f){x=-.75f-x;x=x*slope;x=x+.75f;x=-x;}
            float dry=input*(1.f-mix);float wet=mix*trim;wet=wet*x;b[i]=dry+wet;
        }
    }
};
struct CAFilterBitRed:CAFilterTube {
    int bits=0;
    void setDestCutoff(float midi){float v=midi-floatBits(0x421df281);v=v/floatBits(0xc2b773da);v=v+1.f;v=v*15.f;v=v+1.f;bits=int(v);}
    void process(float* b,int n){const float scale=float(std::int32_t(std::uint32_t(1)<<(bits&31))),inverse=1.f/scale;
        for(int i=0;i<n;++i){const float input=b[i];float dry=(1.f-mix)*input;float wet=mix*drive;float v=input*scale;v=v+.5f;v=std::floor(v);v=v*inverse;wet=wet*v;b[i]=dry+wet;}
    }
};
struct CAFilterRingMod {
    float sampleRate=44100,phase=0,step=0,mix=0,subMix=0;
    std::array<float,201> table{};
    CAFilterRingMod(){
        static constexpr std::uint32_t originalSineBits[201]={0x00000000,0x3d00a892,0x3d809851,0x3dc0bbdc,0x3e00575b,0x3e20305c,0x3e3fe0e3,0x3e5f60f1,0x3e7ea891,0x3e8ed7ec,0x3e9e377a,0x3ead6f0e,0x3ebc7ad2,0x3ecb56f8,0x3ed9ffbe,0x3ee87172,0x3ef6a86c,0x3f025089,0x3f092bf2,0x3f0fe4b2,0x3f167918,0x3f1ce77a,0x3f232e38,0x3f294bbc,0x3f2f3e7b,0x3f3504f3,0x3f3a9db0,0x3f400747,0x3f45405b,0x3f4a4799,0x3f4f1bbd,0x3f53bb8d,0x3f5825e0,0x3f5c5996,0x3f6055a2,0x3f641901,0x3f67a2bf,0x3f6af1f8,0x3f6e05d5,0x3f70dd90,0x3f737871,0x3f75d5cf,0x3f77f511,0x3f79d5ae,0x3f7b772d,0x3f7cd925,0x3f7dfb3b,0x3f7edd26,0x3f7f7eae,0x3f7fdfa9,0x3f800000,0x3f7fdfa9,0x3f7f7eae,0x3f7edd26,0x3f7dfb3b,0x3f7cd925,0x3f7b772d,0x3f79d5ae,0x3f77f511,0x3f75d5ce,0x3f737870,0x3f70dd90,0x3f6e05d5,0x3f6af1f8,0x3f67a2be,0x3f641900,0x3f6055a3,0x3f5c5998,0x3f5825e1,0x3f53bb8e,0x3f4f1bbd,0x3f4a479a,0x3f45405b,0x3f400748,0x3f3a9db0,0x3f3504f3,0x3f2f3e7a,0x3f294bbc,0x3f232e37,0x3f1ce779,0x3f167917,0x3f0fe4b1,0x3f092bf1,0x3f025088,0x3ef6a86e,0x3ee87174,0x3ed9ffc1,0x3ecb56fa,0x3ebc7ad4,0x3ead6f10,0x3e9e377b,0x3e8ed7ed,0x3e7ea891,0x3e5f60f1,0x3e3fe0e2,0x3e20305a,0x3e005759,0x3dc0bbd5,0x3d809849,0x3d00a87d,0xb3bbbd2e,0xbd00a8ac,0xbd809860,0xbdc0bbcd,0xbe005754,0xbe203055,0xbe3fe0de,0xbe5f60ed,0xbe7ea88d,0xbe8ed7eb,0xbe9e3779,0xbead6f0e,0xbebc7ad2,0xbecb56f8,0xbed9ffbf,0xbee87173,0xbef6a86c,0xbf02508b,0xbf092bf3,0xbf0fe4b4,0xbf16791a,0xbf1ce77c,0xbf232e37,0xbf294bbb,0xbf2f3e7a,0xbf3504f2,0xbf3a9daf,0xbf400747,0xbf45405d,0xbf4a479c,0xbf4f1bbf,0xbf53bb8b,0xbf5825de,0xbf5c5995,0xbf6055a1,0xbf641900,0xbf67a2be,0xbf6af1f7,0xbf6e05d4,0xbf70dd8f,0xbf737870,0xbf75d5ce,0xbf77f511,0xbf79d5ae,0xbf7b772d,0xbf7cd924,0xbf7dfb3b,0xbf7edd26,0xbf7f7eae,0xbf7fdfa9,0xbf800000,0xbf7fdfa9,0xbf7f7eae,0xbf7edd26,0xbf7dfb3b,0xbf7cd924,0xbf7b772d,0xbf79d5ae,0xbf77f510,0xbf75d5ce,0xbf737870,0xbf70dd8f,0xbf6e05d4,0xbf6af1f6,0xbf67a2be,0xbf6418ff,0xbf6055a1,0xbf5c5995,0xbf5825e2,0xbf53bb90,0xbf4f1bbf,0xbf4a479b,0xbf45405d,0xbf400749,0xbf3a9db2,0xbf3504f5,0xbf2f3e7c,0xbf294bbe,0xbf232e39,0xbf1ce77b,0xbf167919,0xbf0fe4b3,0xbf092bf3,0xbf02508a,0xbef6a86c,0xbee87172,0xbed9ffbe,0xbecb56f8,0xbebc7ad2,0xbead6f0d,0xbe9e3778,0xbe8ed7ea,0xbe7ea88b,0xbe5f60ec,0xbe3fe0dd,0xbe203054,0xbe005753,0xbdc0bbca,0xbd80983d,0xbd00a866,0x343bbd2e};
        for(int i=0;i<=200;++i)table[i]=floatBits(originalSineBits[i]);
    }
    void clearHistory(){phase=0;}
    void setSampleRate(float r){sampleRate=r;}
    void setDestCutoff(float midi){float exponent=(midi-69.f)*floatBits(0x3daaaaab);float hz=std::pow(2.f,exponent)*440.f;hz=hz*200.f;step=hz/sampleRate;}
    void setDestResonance(float){mix=0;} void setDestDrive(float){subMix=0;}
    void setMode(int){} void setPhatMode(unsigned){}
    float interpolate(float p){int index=int(p);float v=table[index+1]-table[index];v=v*(p-float(index));return v+table[index];}
    void process(float* b,int n){for(int i=0;i<n;++i){phase=phase+step;if(phase>=200.f)phase=0;
        float a=(1.f-subMix)*interpolate(phase),s=subMix*interpolate(phase*.25f);a=a+s;a=a*(1.f-mix);a=mix+a;b[i]=b[i]*a;
    }}
};
struct CAFilterComb {
    float sampleRate=44100,delay=0,feedback=0,fadeStep=0,fade=0,dcCoefficient=floatBits(0x3f7fd150),dcInput=0,dcOutput=0;
    int read=0,newRead=0,write=0;bool changing=false,changed=false;int transition=0;bool negative=false;
    std::vector<float> buffer;
    explicit CAFilterComb(bool neg=false):negative(neg){setSampleRate(sampleRate);}
    void clearHistory(){} // Original wrapper retains the comb history.
    void setSampleRate(float r){sampleRate=r;buffer.assign(std::size_t(int(r+.5f)),0.f);fadeStep=1.f/(floatBits(0x3c23d70a)*r);read=newRead=0;write=int(delay*r+.5f);transition=0;fade=0;changed=changing=false;if(negative){dcCoefficient=(floatBits(0xc1fb53d2)/r)+1.f;dcInput=dcOutput=0;}}
    void setDestCutoff(float midi){float exponent=(midi-69.f)*floatBits(0x3daaaaab);float hz=std::pow(2.f,exponent)*440.f;float next=1.f/hz;if(next!=delay){delay=next;transition=1;changed=true;}}
    void setDestResonance(float r){feedback=r*floatBits(negative?0xbf7d70a4:0x3f7d70a4);}
    void setDestDrive(float){} void setMode(int){} void setPhatMode(unsigned){}
    void increment(int& index){if(++index>=int(buffer.size()))index=0;}
    void process(float* b,int n){
        for(int i=0;i<n;++i){float out;
            if(transition==0){out=buffer[read];float v=out*feedback;buffer[write]=b[i]+v;increment(read);increment(write);}
            else{
                if(changed&&!changing){changed=false;changing=true;newRead=write-int(delay*sampleRate+.5f);if(newRead<0)newRead+=int(buffer.size());fade=0;}
                out=(1.f-fade)*buffer[read];float other=fade*buffer[newRead];out=out+other;float v=out*feedback;buffer[write]=b[i]+v;
                increment(read);increment(newRead);increment(write);fade=fade+fadeStep;
                if(fade>=1.f){changing=false;read=newRead;fade=0;if(!changed)transition=0;}
            }
            b[i]=out;
        }
        if(n<=0)return;
        float previous=b[0];float v=previous-dcInput;float t=dcCoefficient*dcOutput;v=v+t;v=v+floatBits(0x219392ef);v=v-floatBits(0x219392ef);b[0]=v;dcInput=b[n-1];
        for(int i=1;i<n;++i){float next=b[i];v=next-previous;t=dcCoefficient*b[i-1];v=v+t;v=v+floatBits(0x219392ef);v=v-floatBits(0x219392ef);b[i]=v;previous=next;}
        dcOutput=b[n-1];
    }
};
struct CAFilterCombNeg:CAFilterComb{CAFilterCombNeg():CAFilterComb(true){}}
;
struct CAFilterBiquadLPFat {
    float sampleRate=44100,targetCutoff=17000,targetResonance=0,cutoff=17000,resonance=0,gain=1;
    bool dirty=true;
    std::array<float,4> history{};
    std::array<float,8> coefficients{};
    void clearHistory(){history.fill(0);cutoff=targetCutoff;resonance=targetResonance;dirty=true;}
    void setSampleRate(float r){sampleRate=r;clearHistory();}
    void setDestCutoff(float midi){float v=(midi-69.f)*floatBits(0x3daaaaab);v=std::pow(2.f,v)*440.f;targetCutoff=std::max(40.f,v);dirty=true;}
    void setDestResonance(float r){targetResonance=r*6.f+1.f;dirty=true;}
    void setDestDrive(float){} void setMode(int){} void setPhatMode(unsigned){}
    void setup(float rate,float frequency,float q){
        const double fs=rate,fc=frequency,res=q;double total=1.;
        const double constants[2]={doubleBits(0x3fe87de2ef4e0115ULL),doubleBits(0x3ffd906bbdbe3c10ULL)};
        for(int j=0;j<2;++j){
            double d1=constants[j]/res;
            double omega=fc*doubleBits(0x400921fb54442d18ULL);omega=omega/fs;
            double warp=std::tan(omega);warp=warp*(fs+fs);
            double warpSquared=warp*warp;
            double n2=0./warpSquared,n1=0./warp,d2=1./warpSquared;d1=d1/warp;
            double numerator4=4.*n2;numerator4=numerator4*fs;numerator4=numerator4*fs;
            double num=n1+n1;num=num*fs;num=num+numerator4;num=num+1.;
            double denominator4=4.*d2;denominator4=denominator4*fs;denominator4=denominator4*fs;
            double den=d1+d1;den=den*fs;den=den+denominator4;den=den+1.;
            total=total*(num/den);
            double a=-8.*d2;a=a*fs;a=a*fs;a=a+2.;a=a/den;
            double d=d1*-2.;d=d*fs;double a2=denominator4+d;a2=1.+a2;a2=a2/den;
            double b=-8.*n2;b=b*fs;b=b*fs;b=b+2.;b=b/num;
            double nn=n1*-2.;nn=nn*fs;double b2=numerator4+nn;b2=1.+b2;b2=b2/num;
            coefficients[j*4]=removeTiny(float(a));coefficients[j*4+1]=removeTiny(float(a2));coefficients[j*4+2]=removeTiny(float(b));coefficients[j*4+3]=removeTiny(float(b2));
        }
        gain=removeTiny(float(total));
    }
    void process(float* buffer,int n){
        if(dirty){dirty=false;setup(sampleRate,targetCutoff,targetResonance);cutoff=targetCutoff;resonance=targetResonance;}
        for(int i=0;i<n;++i){float v=buffer[i]*gain;
            for(int j=0;j<2;++j){float t=history[j*2]*coefficients[j*4];float state=v-t;t=history[j*2+1]*coefficients[j*4+1];state=state-t;
                float out=history[j*2]*coefficients[j*4+2];out=out+state;t=history[j*2+1]*coefficients[j*4+3];out=out+t;
                history[j*2+1]=history[j*2];history[j*2]=state;v=out;
            }buffer[i]=v;
        }
        for(float& v:history)v=removeTiny(v);
    }
};
}
